#!/usr/bin/env python3
"""Remove only the task-specific private config directory below RUNNER_TEMP."""
from pathlib import Path
import argparse,os,shutil
p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,required=True);a=p.parse_args();base=Path(os.environ['RUNNER_TEMP']).resolve();d=a.directory.resolve();assert d.parent==base and d.name=='nuvio-apk20030-private';shutil.rmtree(d,ignore_errors=True)
