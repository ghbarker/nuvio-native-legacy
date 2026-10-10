#!/usr/bin/env python3
"""Prove the native folder comparison rejects its former expanded-width basis.

Only the fixture environment selects the old measurement. Production and
fixture source bytes stay unchanged; positive captures run independently.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import os
import re
import subprocess
import sys
import tempfile

CASE = '2340x1080@1.5'
CAPTURE_LOG = 'homelayouts/2340x1080-ui1.5/capture.log'
ASSERTION = 'fabsf(poster.w-pw*escalaPoster)<.15f&&fabsf(poster.h-posterVis)<.15f'
OUTPUT = 'build/home-folder-native-negative'
NUMBER = r'(-?\d+(?:\.\d+)?)'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def one_measurement(pattern, log, name):
    matches = re.findall(pattern, log)
    require(len(matches) == 1, f'Expected exactly one flushed {name} measurement')
    values = tuple(float(x) for x in matches[0])
    require(all(math.isfinite(x) for x in values), f'Invalid {name} measurement')
    return values


def validate_failure(output, returncode):
    output = Path(output)
    require(returncode == 1, f'Expected capture driver status 1, got {returncode}')
    results = json.loads((output / 'results.json').read_text(encoding='utf-8'))
    require(isinstance(results, list) and len(results) == 1,
            'Expected exactly one Home folders landscape UI150 result')
    result = results[0]
    require(result.get('fixture') == 'homelayouts' and result.get('case') == CASE
            and result.get('status') == 'capture failed' and result.get('log') == CAPTURE_LOG,
            'Expected the specific Home folders capture failure, not a build or another case')
    error = result.get('error', '')
    require(isinstance(error, str) and '<Signals.SIGABRT: 6>' in error
            and 'homelayouts-shot' in error
            and not re.search(r'timeout|timed out', error, re.I),
            'Expected the Home native SIGABRT assertion, not a timeout or other failure')
    log = (output / CAPTURE_LOG).read_text(encoding='utf-8', errors='replace')
    assertion = (r'homelayouts_shot\.c:\d+: pastasComparar: Assertion [\x60\x27\x22]'
                 + re.escape(ASSERTION) + r'[\x60\x27\x22] failed')
    require(re.search(assertion, log) is not None,
            'Native capture did not fail on the unchanged adjacent ordinary-poster assertion')
    require(len(re.findall(r'Assertion .* failed', log)) == 1,
            'Expected only the intended adjacent-poster assertion failure')
    idle = one_measurement(r'\[shot\] idle expanded reference: focused '+NUMBER+'x'+NUMBER
                           +r', expected width '+NUMBER, log, 'idle expanded reference')
    reference = one_measurement(r'\[shot\] ordinary reference: focused '+NUMBER+'x'+NUMBER
                                +r', unfocused column1 '+NUMBER+'x'+NUMBER
                                +r', row '+NUMBER+r', UI150\b', log, 'named unfocused column1')
    adjacent = one_measurement(r'\[shot\] adjacent measured: folder '+NUMBER+','+NUMBER+' '+NUMBER+'x'+NUMBER
                               +r', poster '+NUMBER+','+NUMBER+' '+NUMBER+'x'+NUMBER
                               +r', expected poster '+NUMBER+'x'+NUMBER, log, 'adjacent targets')
    fw, fh, expected_open = idle
    rw, rh, closed_w, closed_h, row = reference
    fx, fy, folder_w, folder_h, px, py, poster_w, poster_h, expected_w, expected_h = adjacent
    require(min(fw, fh, closed_w, closed_h, folder_w, folder_h, poster_w, poster_h) > 0,
            'Expected visible nonempty measured artwork targets')
    require(abs(fw-fh*16/9) < .15 and abs(fw-expected_open) < .15
            and abs(fw-rw) < .15 and abs(fh-rh) < .15 and rw > closed_w+.15,
            'The original focused-width error must be reproduced with actual idle expansion active')
    require(row >= 0 and row.is_integer() and abs(rh-closed_h) < .15,
            'Unfocused reference must come from the named ordinary row with unchanged artwork height')
    require(abs(folder_h-closed_h) < .15 and abs(folder_w-folder_h*360/203) < .15,
            'The first landscape folder height/aspect must have passed before the poster-width abort')
    require(abs(poster_w-closed_w) < .15 and abs(expected_w-rw) < .15
            and expected_w > poster_w+.15 and abs(poster_h-expected_h) < .15,
            'Only the expanded-width expectation may fail; actual unfocused width and visible height must match')
    require(poster_h >= closed_h*.7 and poster_h <= closed_h+.15,
            'Visible poster height must retain the fixture minimum 70 percent and cannot exceed the full art')
    require(fx >= 0 and fy >= 132 and fx+folder_w <= 2340+.15 and fy+folder_h <= 1080+.15
            and px >= 0 and py >= 132 and px+poster_w <= 2340+.15 and py+poster_h < fy,
            'Measured adjacent targets must remain within the actual landscape viewport')
    return dict(capture_result=result, idle_expanded_reference=idle,
                named_unfocused_reference=reference, adjacent_targets=adjacent,
                folder_height_aspect_passed=True, exact_old_width_failure_verified=True)


def run_negative(root):
    root = Path(root).resolve()
    files = ['src/home.c', 'tests/homelayouts_shot.c']
    before = {name: (root / name).read_bytes() for name in files}
    fixture = before[files[1]]
    require(fixture.count(b'assert(fabsf(alvo.w-w)<.15f&&fabsf(alvo.h-h)<.15f);') == 2,
            'Both unchanged folder geometry assertions must precede the negative abort')
    require(b'if(getenv("NV_FOLDER_OLD_REFERENCE"))pw=normal.rect.w/zoom/escalaPoster;' in fixture,
            'Expected the narrow fixture-only old-reference control')
    output = root / OUTPUT
    output.mkdir(parents=True, exist_ok=False)  # Reject stale or mixed evidence.
    command = [sys.executable, 'tests/phone_shots.py', '--fixtures', 'homelayouts',
               '--home-variant', 'folders', '--cases', CASE, '--output', OUTPUT]
    receipt = dict(case=CASE, command=command, control='NV_FOLDER_OLD_REFERENCE=1',
                   source_sha256_before={k: hashlib.sha256(v).hexdigest() for k, v in before.items()},
                   expected_assertion=ASSERTION, independent_positive_captures_required=True,
                   exact_old_width_failure_verified=False)
    try:
        with (output / 'driver.log').open('w', encoding='utf-8') as stream:
            print('Command:', repr(command), file=stream, flush=True)
            environment = dict(os.environ, NV_FOLDER_OLD_REFERENCE='1',
                               NUVIO_SHOT_FONTE='0', NUVIO_SHOT_EN='0')
            process = subprocess.run(command, cwd=root, env=environment,
                                     stdout=stream, stderr=subprocess.STDOUT)
        receipt['capture_driver_returncode'] = process.returncode
        receipt.update(validate_failure(output, process.returncode))
    finally:
        after = {name: (root / name).read_bytes() for name in files}
        receipt['source_sha256_after'] = {k: hashlib.sha256(v).hexdigest() for k, v in after.items()}
        receipt['source_unchanged_byte_for_byte'] = before == after
        (output / 'negative-control.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
        require(before == after, 'Home/fixture source bytes changed during the environment-only control')
    print('PASS: actual expanded native reference rejects the old width basis on the exact '
          'poster assertion; measured folder height/aspect pass, source bytes unchanged. '
          'Independent positive captures remain required.')


def self_test():
    result = dict(fixture='homelayouts', case=CASE, status='capture failed', log=CAPTURE_LOG,
                  error="Command ['build/home-folder-native-negative/homelayouts-shot', 'prefix', '1'] died with <Signals.SIGABRT: 6>.")
    log = ("[shot] idle expanded reference: focused 693.333x390.000, expected width 693.333\n"
           "[shot] ordinary reference: focused 693.333x390.000, unfocused column1 260.000x390.000, row 3, UI150\n"
           "[shot] adjacent measured: folder 104.000,666.000 691.626x390.000, poster 104.000,150.000 260.000x390.000, expected poster 693.333x390.000\n"
           "homelayouts-shot: tests/homelayouts_shot.c:309: pastasComparar: Assertion `"+ASSERTION+"' failed.\n")
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory);capture = output / CAPTURE_LOG;capture.parent.mkdir(parents=True)
        def attempt(results, text, code, accepted):
            (output / 'results.json').write_text(json.dumps(results), encoding='utf-8');capture.write_text(text, encoding='utf-8')
            try:
                validate_failure(output, code)
            except Exception:
                require(not accepted, 'Validator rejected expected native failure')
            else:
                require(accepted, 'Validator accepted invalid failure evidence')
        attempt([result], log, 1, True)
        invalid = [([],log,1),([result,result],log,1),([result],log,0),([result],log,-6)]
        for key,value in [('fixture','vertudo'),('case','1080x2340@1.5'),('status','build failed'),('log','other.log'),
                          ('error','SIGABRT timeout'),('error','died with <Signals.SIGSEGV: 11>')]:
            invalid.append(([dict(result,**{key:value})],log,1))
        for old,new in [('unfocused column1','unfocused column0'),('UI150','UI100'),
                        ('expected width 693.333','expected width 900.000'),('691.626x390.000','360.000x203.000'),
                        ('expected poster 693.333x390.000','expected poster 260.000x390.000'),
                        ('260.000x390.000','260.000x300.000'),(ASSERTION,'unrelated_assertion')]:
            invalid.append(([result],log.replace(old,new),1))
        invalid.append(([result],log+log,1))
        invalid.append(([result],re.sub(r'\[shot\] adjacent measured:[^\n]+\n','',log),1))
        for height in [100,500]:
            invalid.append(([result],log.replace(
                'poster 104.000,150.000 260.000x390.000, expected poster 693.333x390.000',
                f'poster 104.000,150.000 260.000x{height:.3f}, expected poster 693.333x{height:.3f}'),1))
        for values in invalid:attempt(*values,False)
        # The native runner must refuse an existing output tree before it can
        # launch a driver or reuse any previous capture evidence.
        stale = output / 'stale-checkout'; (stale / 'src').mkdir(parents=True); (stale / 'tests').mkdir()
        actual_root = Path(__file__).resolve().parents[1]
        for name in ['src/home.c', 'tests/homelayouts_shot.c']:
            (stale / name).write_bytes((actual_root / name).read_bytes())
        (stale / OUTPUT).mkdir(parents=True)
        try:
            run_negative(stale)
        except FileExistsError:
            pass
        else:
            require(False, 'Native runner accepted stale output evidence')
    print(f'home_folder_native_negative: valid exact native evidence accepted; {len(invalid)+1} stale/wrong-case/build/status/signal/measurement/assertion controls rejected')


if __name__ == '__main__':
    parser = argparse.ArgumentParser();parser.add_argument('--self-test', action='store_true');args = parser.parse_args()
    try:
        self_test() if args.self_test else run_negative(Path(__file__).resolve().parents[1])
    except Exception as error:
        print(f'FAIL: Home folder native negative control: {error}', file=sys.stderr)
        sys.exit(1)
