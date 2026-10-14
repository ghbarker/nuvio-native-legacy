#!/usr/bin/env python3
"""Capture bounded native hang diagnostics; timeout is evidence, never a pass."""
import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--label', required=True)
    parser.add_argument('--deadline', type=int, default=300)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    original = args.source / 'tests/fontepref.c'
    text = original.read_text()
    marker = 'int main(void) {'
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '\n  fprintf(stderr, "[pref-diagnostic] ENTERED MAIN\\n"); fflush(stderr);', 1)
    marker = '  dados_iniciar(dir);'
    assert text.count(marker) == 1
    text = text.replace(marker, '  fprintf(stderr, "[pref-diagnostic] BEFORE dados_iniciar\\n"); fflush(stderr);\n' + marker +
                        '\n  fprintf(stderr, "[pref-diagnostic] AFTER dados_iniciar\\n"); fflush(stderr);', 1)
    original.write_text(text)
    environment = os.environ.copy()
    environment['SANITIZE'] = '1'
    environment['ASAN_OPTIONS'] = 'detect_leaks=0:verbosity=1'
    log_path = args.output / 'fontepref.log'
    events = []
    start = time.monotonic()
    sampled = False
    binary_seen = None
    binary_pid = None
    snapshots = 0
    with log_path.open('wb') as log:
        child = subprocess.Popen(['bash', 'tests/fontepref.sh'], cwd=args.source,
                                 env=environment, stdout=log, stderr=subprocess.STDOUT,
                                 start_new_session=True)
        events.append({'event': 'started', 'pid': child.pid, 'process_group': child.pid})
        next_snapshot = 0
        while child.poll() is None and time.monotonic() - start < args.deadline:
            elapsed = time.monotonic() - start
            process_table = subprocess.run(['ps', '-axo', 'pid=,pgid=,stat=,etime=,command='],
                                           capture_output=True, text=True, check=True).stdout
            rows = []
            for line in process_table.splitlines():
                parts = line.split(None, 4)
                if len(parts) != 5: continue
                pid, group, state, age, command = parts
                if not pid.isdigit() or not group.isdigit(): continue
                if int(group) == child.pid:
                    rows.append(line)
                    if re.match(r'^/tmp/nuvio-fontepref(?:\s|$)', command) and binary_seen is None:
                        binary_seen, binary_pid = elapsed, int(pid)
                        events.append({'event': 'binary_running', 'pid': binary_pid,
                                       'elapsed_seconds': round(elapsed, 2), 'state': state})
            if elapsed >= next_snapshot:
                (args.output / ('processes-%03d.txt' % snapshots)).write_text('\n'.join(rows) + '\n')
                snapshots += 1
                next_snapshot = elapsed + 15
            if binary_seen is not None and not sampled and elapsed - binary_seen >= 3:
                sampled = True
                sample = subprocess.run(['/usr/bin/sample', str(binary_pid), '2', '-file',
                                         str(args.output / 'pref.sample')],
                                        capture_output=True, text=True, timeout=20)
                (args.output / 'sample-command.log').write_text(sample.stdout + sample.stderr)
                events.append({'event': 'sample_completed', 'pid': binary_pid,
                               'returncode': sample.returncode, 'elapsed_seconds': round(elapsed, 2)})
            time.sleep(1)
        timed_out = child.poll() is None
        if timed_out:
            events.append({'event': 'diagnostic_timeout', 'elapsed_seconds': round(time.monotonic() - start, 2)})
            os.killpg(child.pid, signal.SIGTERM)
            try: child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.wait(timeout=5)
    # A successful parent may still have left a diagnostic child; terminate
    # only this isolated process group, never unrelated runner processes.
    try: os.killpg(child.pid, signal.SIGTERM)
    except ProcessLookupError: pass
    contents = log_path.read_text(errors='replace')
    report = {'label': args.label, 'deadline_seconds': args.deadline,
              'elapsed_seconds': round(time.monotonic() - start, 2), 'script_returncode': child.returncode,
              'binary_pid': binary_pid, 'binary_seen': binary_seen is not None, 'sample_attempted': sampled,
              'entered_main': '[pref-diagnostic] ENTERED MAIN' in contents,
              'before_dados_iniciar': '[pref-diagnostic] BEFORE dados_iniciar' in contents,
              'after_dados_iniciar': '[pref-diagnostic] AFTER dados_iniciar' in contents,
              'outcome': 'DIAGNOSTIC_TIMEOUT' if timed_out else ('TEST_EXIT_0' if child.returncode == 0 else 'TEST_ERROR'),
              'validation_pass_claimed': False, 'events': events}
    (args.output / 'diagnostic-result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
