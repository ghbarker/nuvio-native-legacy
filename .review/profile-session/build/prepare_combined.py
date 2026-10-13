#!/usr/bin/env python3
"""Freeze a reviewed upstream feature onto the retained test app, without editing it.

The input repo must already contain the complete tested retained metadata and watched overlays.
All output lives under --output. Ordinary files use git merge-file; settings
append-only tables receive a structural merge which keeps every existing ID.
Any other conflict requires an explicit reviewed file in --resolutions.
"""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

RETAINED_COMMIT = 'ef864de25a89d89c46620226b5d3da4afb228c3b'
UPSTREAM_BASE = '17bb330373273a3de284d7f2898a3a3e16f1fc3f'
BUILD_SCRIPTS = {'tools/android.sh', 'tools/arm.sh'}


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def git_optional(repo, ref, path):
    result = subprocess.run(['git', '-C', str(repo), 'show', ref + ':' + path], capture_output=True)
    if result.returncode:
        exists = subprocess.run(['git', '-C', str(repo), 'cat-file', '-e', ref], capture_output=True)
        if exists.returncode:
            raise RuntimeError('Missing git reference: ' + ref)
        return b''
    return result.stdout


def blob(data):
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def clean(text):
    # Preserve string literals while deleting comments.
    return re.sub(r'"(?:\\.|[^"\\])*"|/\*[\s\S]*?\*/|//[^\n]*',
                  lambda m: m[0] if m[0].startswith('"') else '', text)


def norm(text):
    return ''.join(re.findall(r'"(?:\\.|[^"\\])*"|\w+|[^\s]', clean(text)))


def split_entries(text):
    """Return initializer entries with source boundaries, ignoring nested commas."""
    cooked = re.sub(r'"(?:\\.|[^"\\])*"|/\*[\s\S]*?\*/|//[^\n]*',
                    lambda m: m[0] if m[0].startswith('"') else ' ' * len(m[0]), text)
    spans, start, depth, quoted, escape = [], 0, 0, False, False
    for i, char in enumerate(cooked):
        if quoted:
            if escape:
                escape = False
            elif char == '\\':
                escape = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char in '([{':
            depth += 1
        elif char in ')]}':
            depth -= 1
        elif char == ',' and depth == 0:
            if norm(text[start:i]):
                spans.append((start, i + 1, norm(text[start:i])))
            start = i + 1
    if norm(text[start:]):
        spans.append((start, len(text), norm(text[start:])))
    assert depth == 0 and not quoted
    return spans


def blocks(text):
    patterns = {
        'enum': r'typedef enum\s*\{(?P<body>[\s\S]*?)\}\s*OpcaoId;',
        'options': r'static const Opcao OPCOES\[AJ_N\]\s*=\s*\{(?P<body>[\s\S]*?)\n\};',
        'keys': r'static const char \*CHAVE\[\]\s*=\s*\{(?P<body>[\s\S]*?)\n\};',
    }
    result = {}
    for name, pattern in patterns.items():
        matches = list(re.finditer(pattern, text))
        assert len(matches) == 1, (name, len(matches))
        m = matches[0]
        result[name] = (m.start('body'), m.end('body'), m['body'])
    return result


def merge_file(current, base, feature, scratch, label):
    files = []
    for name, value in (('retained', current), ('base', base), ('feature', feature)):
        p = scratch / name
        p.write_bytes(value)
        files.append(str(p))
    result = subprocess.run(['git', 'merge-file', '--diff3', '-p',
                             '-L', 'retained', '-L', 'upstream', '-L', 'profile-session',
                             *files], capture_output=True)
    if result.returncode < 0 or result.returncode >= 128:
        raise RuntimeError(result.stderr.decode())
    return result.stdout, result.returncode


def merge_settings(base_bytes, retained_bytes, feature_bytes, scratch):
    base, retained, feature = [d.decode() for d in (base_bytes, retained_bytes, feature_bytes)]
    bb, rb, fb = [blocks(t) for t in (base, retained, feature)]
    tails = {}
    replacements = []
    for name in bb:
        b = split_entries(bb[name][2]); r = split_entries(rb[name][2]); f = split_entries(fb[name][2])
        if name == 'enum':
            assert b[-1][2] == r[-1][2] == f[-1][2] == 'AJ_N'
            b = b[:-1]; r = r[:-1]; f = f[:-1]
        baseline = [e[2] for e in b]
        assert [e[2] for e in r[:len(b)]] == baseline, ('retained settings drift', name)
        assert [e[2] for e in f[:len(b)]] == baseline, ('feature rewrites existing settings', name)
        assert len(f) >= len(b), ('Feature must append settings', name)
        tails[name] = fb[name][2][f[len(b)][0]:f[-1][1]] if len(f) > len(b) else ''
        replacements.append((fb[name][0], fb[name][1], bb[name][2]))
    # Merge the behavior changes normally after removing append-only tables.
    stripped = feature
    for start, end, body in sorted(replacements, reverse=True):
        stripped = stripped[:start] + body + stripped[end:]
    merged_bytes, status = merge_file(retained_bytes, base_bytes, stripped.encode(), scratch, 'settings')
    merged = merged_bytes.decode()
    mb = blocks(merged)
    for name in sorted(mb, key=lambda n: mb[n][0], reverse=True):
        start, end, body = mb[name]
        entries = split_entries(body)
        if name == 'enum':
            assert entries[-1][2] == 'AJ_N'
            insert = entries[-1][0]
            body = body[:insert] + '\n' + tails[name] + body[insert:]
        else:
            body = body.rstrip() + '\n' + tails[name] + '\n'
        merged = merged[:start] + body + merged[end:]
    for name, block in blocks(merged).items():
        before = [e[2] for e in split_entries(rb[name][2])]
        after = [e[2] for e in split_entries(block[2])]
        if name == 'enum':
            before = before[:-1]; after = after[:-1]
        assert after[:len(before)] == before, ('Retained settings indices changed', name)
        assert len(after) == len(before) + len(split_entries(tails[name])), name
    return merged.encode(), status


def assert_settings_preserved(retained, merged, defaults_before, defaults_after):
    before, after = blocks(retained.decode()), blocks(merged.decode())
    counts = {}
    for name in before:
        b = [e[2] for e in split_entries(before[name][2])]
        a = [e[2] for e in split_entries(after[name][2])]
        if name == 'enum':
            assert b[-1] == a[-1] == 'AJ_N'
            b, a = b[:-1], a[:-1]
        assert a[:len(b)] == b and len(a) >= len(b), name
        counts[name] = len(b)
    b = [e[2] for e in split_entries(defaults_before.decode())]
    a = [e[2] for e in split_entries(defaults_after.decode())]
    assert a[:len(b)] == b and len(a) >= len(b)
    assert counts['enum'] == counts['options'] == counts['keys'] == len(b), counts
    return {'retained_setting_count': len(b), 'all_existing_indices_preserved': True,
            'appended_setting_defaults': a[len(b):]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True, type=Path)
    parser.add_argument('--feature', required=True)
    parser.add_argument('--base', default=UPSTREAM_BASE)
    parser.add_argument('--retained-commit', default=RETAINED_COMMIT)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--resolutions', type=Path)
    parser.add_argument('--extra-overlay', type=Path, help='Reviewed preview-only src adaptations, excluded from the standalone feature')
    parser.add_argument('--snapshot', type=Path,
                        help='Read frozen local feature bytes when its remote git object is not local')
    parser.add_argument('--snapshot-plan', type=Path,
                        help='Exact standalone manifest proving each local feature blob')
    parser.add_argument('--snapshot-base-ref', default='HEAD',
                        help='Local partial upstream snapshot commit; metadata still pins true upstream SHA')
    args = parser.parse_args()
    assert re.fullmatch('[0-9a-f]{40}', args.base) and re.fullmatch('[0-9a-f]{40}', args.feature)
    args.output.mkdir(parents=True, exist_ok=True)
    feature_entries = {}
    if args.snapshot:
        assert args.snapshot_plan, '--snapshot requires --snapshot-plan'
        feature_plan = json.loads(args.snapshot_plan.read_text())
        assert feature_plan['base'] == args.base and feature_plan['feature_commit'] == args.feature
        feature_entries = {e['path']: e for e in feature_plan['files']}
        assert len(feature_entries) == len(feature_plan['files'])
        paths = sorted(feature_entries)
    else:
        paths = git(args.repo, 'diff', '--name-only', '-z', args.base, args.feature).decode().split('\0')
        paths = sorted(p for p in paths if p)
    assert paths and all((p.startswith('src/') or p in BUILD_SCRIPTS) and '..' not in Path(p).parts for p in paths)
    entries, outputs, conflicts = [], {}, []
    with tempfile.TemporaryDirectory(dir=args.output) as temp:
        scratch = Path(temp)
        for path in paths:
            if args.snapshot:
                base = git_optional(args.snapshot, args.snapshot_base_ref, path)
                feature = (args.snapshot / path).read_bytes()
                assert blob(feature) == feature_entries[path]['sha'], path
            else:
                base = git_optional(args.repo, args.base, path)
                feature = git(args.repo, 'show', args.feature + ':' + path)
            disk = args.repo / path
            retained = disk.read_bytes() if disk.is_file() else b''
            method = 'three-way'
            if path == 'src/ajustes.c':
                merged, status = merge_settings(base, retained, feature, scratch)
                method = 'structural-settings-append-and-three-way'
            elif path == 'src/ajustes_ux_padrao.inc':
                b, f = split_entries(base.decode()), split_entries(feature.decode())
                assert [e[2] for e in f[:len(b)]] == [e[2] for e in b] and len(f) >= len(b)
                merged = retained.rstrip() + b'\n' + (feature.decode()[f[len(b)][0]:].lstrip().encode() if len(f) > len(b) else b'')
                status, method = 0, 'append-after-retained-defaults'
            else:
                merged, status = merge_file(retained, base, feature, scratch, path)
            if status:
                for kind, data in (('base', base), ('retained', retained), ('feature', feature), ('conflict', merged)):
                    p = args.output / 'conflicts' / (path + '.' + kind)
                    p.parent.mkdir(parents=True, exist_ok=True)
                    p.write_bytes(data)
                resolved = args.resolutions / path if args.resolutions else None
                if resolved and resolved.is_file():
                    merged = resolved.read_bytes()
                    assert b'<<<<<<<' not in merged and b'>>>>>>>' not in merged
                    method = 'explicit-reviewed-conflict-resolution'
                else:
                    conflicts.append(path)
                    continue
            mode = '100755' if disk.exists() and disk.stat().st_mode & 0o111 else '100644'
            feature_mode = (feature_entries[path]['mode'] if args.snapshot else
                            git(args.repo, 'ls-tree', args.feature, '--', path).decode().split()[0])
            assert feature_mode == mode
            dest = args.output / 'merged' / path
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(merged)
            os.chmod(dest, 0o755 if mode == '100755' else 0o644)
            outputs[path] = merged
            entries.append({'path': path, 'sha': blob(merged), 'mode': mode,
                            'retained_sha': blob(retained), 'upstream_sha': blob(base),
                            'feature_sha': blob(feature), 'merge': method})
    if conflicts:
        raise SystemExit('Explicit reviewed resolutions required: ' + ', '.join(conflicts))
    preview_entries = []
    if args.extra_overlay:
        for disk in sorted((args.extra_overlay / 'src').rglob('*')):
            if not disk.is_file(): continue
            path = str(disk.relative_to(args.extra_overlay))
            assert path not in outputs, ('Use --resolutions for standalone paths', path)
            retained_path = args.repo / path
            retained = retained_path.read_bytes() if retained_path.is_file() else b''
            merged = disk.read_bytes()
            mode = '100755' if disk.stat().st_mode & 0o111 else '100644'
            dest = args.output / 'merged' / path; dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(merged); os.chmod(dest, 0o755 if mode == '100755' else 0o644)
            outputs[path] = merged
            preview_entries.append({'path': path, 'sha': blob(merged), 'mode': mode,
                                    'retained_sha': blob(retained), 'merge': 'reviewed-preview-only-scope-adaptation'})
    preservation = assert_settings_preserved(
        (args.repo / 'src/ajustes.c').read_bytes(), outputs['src/ajustes.c'],
        (args.repo / 'src/ajustes_ux_padrao.inc').read_bytes(), outputs['src/ajustes_ux_padrao.inc'])
    patches = []
    for entry in entries + preview_entries:
        p = entry['path']
        existed = (args.repo / p).is_file()
        before = (args.repo / p).read_text().splitlines(keepends=True) if existed else []
        after = outputs[p].decode().splitlines(keepends=True)
        patches.append('diff --git a/' + p + ' b/' + p + '\n')
        if not existed:
            patches.append('new file mode ' + entry['mode'] + '\n')
        patches.extend(difflib.unified_diff(before, after, fromfile='a/' + p if existed else '/dev/null', tofile='b/' + p))
    (args.output / 'combined.patch').write_text(''.join(patches))
    manifest = {'retained_commit': args.retained_commit, 'upstream_base': args.base,
                'feature_commit': args.feature, 'settings': preservation, 'files': entries,
                'preview_only_files': preview_entries}
    (args.output / 'combined-files.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'files': len(entries), 'settings': preservation, 'feature': args.feature}, indent=2))


if __name__ == '__main__':
    main()
