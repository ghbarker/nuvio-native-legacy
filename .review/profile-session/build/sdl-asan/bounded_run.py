#!/usr/bin/env python3
"""Run unchanged native commands with separate compile and execution limits."""
import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time


def capture(command, output, seconds=8):
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                timeout=seconds, text=True)
        output.write_text(result.stdout)
        return result.returncode
    except subprocess.TimeoutExpired as exc:
        raw = exc.stdout or b''
        output.write_text((raw.decode(errors='replace') if isinstance(raw, bytes) else raw)
                          + '\nDIAGNOSTIC COMMAND TIMEOUT\n')
        return 124
    except OSError as exc:
        output.write_text(str(exc) + '\n')
        return 127


def terminate(child):
    try:
        os.killpg(child.pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        child.wait(timeout=2)
    except subprocess.TimeoutExpired:
        os.killpg(child.pid, signal.SIGKILL)
        child.wait(timeout=2)


def run(command, output, runtime_seconds, compile_seconds=0, binary_pattern=None, env=None, cwd=None):
    output.mkdir(parents=True, exist_ok=True)
    events = []
    started = time.monotonic()
    binary_started = None if binary_pattern else started
    binary_pid = None
    sampled = False
    timed_out = False
    pattern = re.compile(binary_pattern) if binary_pattern else None
    log_path = output / 'command.log'
    with log_path.open('wb') as log:
        child = subprocess.Popen(command, cwd=cwd, env=env, stdout=log, stderr=subprocess.STDOUT,
                                 start_new_session=True)
        events.append({'event': 'started', 'pid': child.pid, 'command': command})
        try:
            while child.poll() is None:
                now = time.monotonic()
                if pattern:
                    table = subprocess.run(['ps', '-axo', 'pid=,pgid=,stat=,etime=,command='],
                                           capture_output=True, text=True, timeout=2, check=True).stdout
                    rows = []
                    for line in table.splitlines():
                        parts = line.split(None, 4)
                        if len(parts) != 5 or not parts[0].isdigit() or not parts[1].isdigit():
                            continue
                        if int(parts[1]) != child.pid:
                            continue
                        rows.append(line)
                        # Match the process executable, never an output filename
                        # appearing later in a compiler's argument list.
                        executable = parts[4].split(None, 1)[0]
                        if binary_started is None and pattern.search(executable):
                            binary_started, binary_pid = now, int(parts[0])
                            events.append({'event': 'binary_started', 'pid': binary_pid,
                                           'elapsed_seconds': round(now - started, 3)})
                    (output / 'processes.txt').write_text('\n'.join(rows) + '\n')
                elif binary_pid is None:
                    binary_pid = child.pid
                limit = runtime_seconds if binary_started is not None else compile_seconds
                phase_start = binary_started if binary_started is not None else started
                if now - phase_start >= limit:
                    timed_out = True
                    events.append({'event': 'timeout', 'phase': 'runtime' if binary_started is not None else 'compile',
                                   'elapsed_seconds': round(now - started, 3)})
                    break
                if binary_pid and not sampled and binary_started is not None and now - binary_started >= 2:
                    sampled = True
                    if Path('/usr/bin/sample').exists():
                        sample_limit = min(4, max(1, int(limit - (now - phase_start))))
                        rc = capture(['/usr/bin/sample', str(binary_pid), '1', '-file', str(output / 'native.sample')],
                                     output / 'sample-command.log', sample_limit)
                        events.append({'event': 'sample', 'returncode': rc})
                time.sleep(0.15)
        finally:
            if child.poll() is None:
                terminate(child)
    result = {'command': command, 'returncode': child.returncode, 'timed_out': timed_out,
              'runtime_limit_seconds': runtime_seconds, 'compile_limit_seconds': compile_seconds,
              'elapsed_seconds': round(time.monotonic() - started, 3), 'binary_pid': binary_pid,
              'outcome': 'TIMEOUT' if timed_out else ('PASS' if child.returncode == 0 else 'FAIL'),
              'events': events}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(log_path.read_text(errors='replace')[-20000:], flush=True)
    print(json.dumps(result, indent=2), flush=True)
    return 124 if timed_out else child.returncode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--runtime-seconds', type=int, default=120)
    parser.add_argument('--compile-seconds', type=int, default=300)
    parser.add_argument('--binary-pattern')
    parser.add_argument('--cwd', type=Path)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command or not 0 < args.runtime_seconds <= 120 or not 0 < args.compile_seconds <= 300:
        parser.error('Provide a command and limits within 120 runtime / 300 compile seconds')
    raise SystemExit(run(command, args.output.resolve(), args.runtime_seconds, args.compile_seconds,
                         args.binary_pattern, cwd=args.cwd))


if __name__ == '__main__':
    main()
