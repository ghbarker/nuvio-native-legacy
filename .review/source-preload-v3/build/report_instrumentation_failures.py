#!/usr/bin/env python3
"""Preserve exact runtime diagnostics even when the Gradle test task fails."""
from pathlib import Path
import xml.etree.ElementTree as ET
root = Path('.review/source-preload-v3/android/probe/build/outputs/androidTest-results')
reports = list(root.rglob('*.xml'))
print('Instrumentation result reports:', len(reports))
for report in reports:
    for case in ET.parse(report).getroot().iter('testcase'):
        for kind in ('failure', 'error', 'skipped'):
            message = case.find(kind)
            if message is not None:
                print(str(report), case.attrib, kind, message.attrib)
                print(message.text or '')
