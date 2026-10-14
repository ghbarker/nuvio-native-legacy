#!/usr/bin/env python3
"""Match actual C JNI lookups against the actual compiled NvPlayer class."""
import argparse
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--classes', type=Path, required=True)
parser.add_argument('--source-proof', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
klass = args.classes / 'space/nuvio/nativelegacy/NvPlayer.class'
assert klass.is_file(), klass
bridge = (args.source / 'src/video_android.c').read_text()
assert '"space.nuvio.nativelegacy.NvPlayer"' in bridge
lookups = re.findall(r'GetStaticMethodID\(env,\s*gCls,\s*"([^"]+)",\s*"([^"]+)"\)', bridge)
assert len(lookups) >= 19 and len({name for name, _ in lookups}) == len(lookups)
required = {'preparar', 'prepararProximo', 'prepararEstado', 'cancelarPreparo', 'abrirPreparada', 'pararMantendoPreparo'}
assert required <= {name for name, _ in lookups}
output = subprocess.check_output(['javap', '-s', '-classpath', str(args.classes), 'space.nuvio.nativelegacy.NvPlayer'], text=True)
methods = {}
name = None
for line in output.splitlines():
    if ' public ' in ' ' + line and ' static ' in line and '(' in line:
        match = re.search(r'([A-Za-z_$][A-Za-z0-9_$]*)\(', line)
        name = match.group(1) if match else None
    elif name and 'descriptor:' in line:
        methods.setdefault(name, set()).add(line.split('descriptor:', 1)[1].strip())
        name = None
for name, descriptor in lookups:
    assert descriptor in methods.get(name, set()), (name, descriptor, methods.get(name))
legacy = json.loads((Path(__file__).parent / 'legacy-jni-descriptors.json').read_text())
for entry in legacy['methods']:
    assert entry['descriptor'] in methods.get(entry['name'], set()), ('Missing original static overload', entry)
proof = json.loads(args.source_proof.read_text())
assert proof['verified'] and proof['phase'] == 'final'
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps({'outcome': 'passed', 'source_commit': proof['source_commit'],
                                 'review_commit': proof['build_commit'], 'case': proof['case'],
                                 'compiled_class': str(klass), 'actual_c_bridge': 'src/video_android.c',
                                 'static_method_descriptors': dict(lookups),
                                 'preload_bridge_methods': sorted(required),
                                 'legacy_v1_source': legacy['source_commit'],
                                 'legacy_static_method_descriptors': {entry['name']: entry['descriptor'] for entry in legacy['methods']}}, indent=2) + '\n')
print('Actual compiled JNI bridge descriptors PASS:', len(lookups), 'methods, including all six preload methods')
