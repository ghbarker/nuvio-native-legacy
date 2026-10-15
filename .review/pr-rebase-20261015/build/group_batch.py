#!/usr/bin/env python3
"""Amortize native SDK setup across small suites while retaining each proof."""
import argparse
from pathlib import Path
import subprocess
import sys
p = argparse.ArgumentParser()
p.add_argument('--case', required=True)
p.add_argument('--groups', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
for group in a.groups.split(','):
    subprocess.run([sys.executable, str(Path(__file__).with_name('regressions.py')),
        '--case', a.case, '--group', group, '--output', str(a.output / group)], check=True)
