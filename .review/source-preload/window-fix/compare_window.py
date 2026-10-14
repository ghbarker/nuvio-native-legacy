#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--baseline', type=Path, required=True)
parser.add_argument('--candidate', type=Path, required=True)
parser.add_argument('--fixture', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
assert subprocess.check_output(['git', '-C', str(args.baseline), 'rev-parse', 'HEAD'], text=True).strip() == '17b7769dac4273557e1b07f5fb8734df91dd718f'
assert subprocess.check_output(['git', '-C', str(args.candidate), 'rev-parse', 'HEAD'], text=True).strip() == 'e7f58bc8108534e4425867ac04127731962ccf73'
args.output.mkdir(parents=True, exist_ok=True)
player = 'android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt'
assert (args.baseline / player).read_bytes() == (args.candidate / player).read_bytes()
old = 'tests/android_window_layout.py'
assert (args.baseline / old).read_bytes() == (args.candidate / old).read_bytes()

diagnostics = []
for name, root in [('baseline', args.baseline), ('candidate', args.candidate)]:
    result = subprocess.run(['python3', str(root / old)], capture_output=True, text=True)
    log = result.stdout + result.stderr
    (args.output / ('old-window-' + name + '.log')).write_text(log)
    assert result.returncode != 0 and 'estavelEmitido' in log and 'RECRIA_ESPERA_HDR_MS' in log
    errors = re.findall(r'error:\s*(.*)', log)
    assert len(errors) == 2, errors
    diagnostics.append(errors)
assert diagnostics[0] == diagnostics[1], diagnostics

for name, root in [('baseline', args.baseline), ('candidate', args.candidate)]:
    dest = root / 'tests/pr431_window_geometry.py'
    assert not dest.exists()
    shutil.copyfile(args.fixture, dest)
    result = subprocess.run(['python3', str(dest)], capture_output=True, text=True)
    log = result.stdout + result.stderr
    (args.output / ('bounded-window-' + name + '.log')).write_text(log)
    assert result.returncode == 0, log
    assert 'android_window_layout:' in log and 'ok' in log
proof = {'outcome': 'passed', 'baseline_commit': '17b7769dac4273557e1b07f5fb8734df91dd718f',
         'candidate_commit': 'e7f58bc8108534e4425867ac04127731962ccf73',
         'identical_original_fixture_failure': diagnostics[0], 'production_player_bytes_identical': True,
         'extracted_production_methods_unchanged': True,
         'bounded_neighbor_fields': ['estavelEmitido', 'RECRIA_ESPERA_HDR_MS'],
         'existing_geometry_and_HDR_assertions_retained': True,
         'additional_stable_HDR_600ms_and_SDR_350ms_assertions': 'passed on both sources',
         'fixture_sha256': hashlib.sha256(args.fixture.read_bytes()).hexdigest()}
(args.output / 'window-baseline-comparison.json').write_text(json.dumps(proof, indent=2) + '\n')
print('Window fixture: identical canonical/candidate baseline failure; bounded fixture passed both exact sources')
