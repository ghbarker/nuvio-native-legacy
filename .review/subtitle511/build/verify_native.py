#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path
from verify_source import verify

p = argparse.ArgumentParser()
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
required = ['full-app-tv', 'full-app-mobile', 'legendasui-original', 'legendasui-original-asan',
            'legendasui_425-original', 'legendasui_425-original-asan', 'legmemoria-original',
            'legmemoria-sanitized-replay', 'faixasmkv-original', 'faixasmkv-sanitized-replay',
            'faixasmkv_casar-original', 'faixasmkv_casar-sanitized-replay', 'linguas-normal',
            'linguas-asan', 'subtitle-title-fixtures']
assert (a.output / 'passed-gates.txt').read_text().splitlines() == required
for mode in ['tv', 'mobile']:
    assert (a.output / ('app-' + mode)).stat().st_size > 0
fixture = json.loads((a.output / 'subtitle-titles/report.json').read_text())
assert fixture['passed'] and fixture['source_unchanged']
replays = []
for name in ['legmemoria', 'faixasmkv', 'faixasmkv_casar']:
    proof = json.loads((a.output / 'sanitized-originals' / (name + '-sanitized.json')).read_text())
    assert proof['exit_code'] == 0 and proof['original_source_arguments_and_assertions_unchanged']
    replays.append(proof)
proof = verify(Path.cwd())
proof.update(outcome='passed', gates=required, sanitizer_replays=replays,
             subtitle_fixture_report_sha256=hashlib.sha256((a.output / 'subtitle-titles/report.json').read_bytes()).hexdigest(),
             fresh_full_tv_mobile_builds=True, prior_passes_reused=False)
(a.output / 'native-verified.json').write_text(json.dumps(proof, indent=2) + '\n')
print('All 15 fresh native build and regression gates passed')
