#!/usr/bin/env python3
"""Publish the exact source only after current macOS + immutable Android verification."""
import argparse, hashlib, json, os, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'log-download-public'))
from publish_source import publish, SOURCE, TREE

p = argparse.ArgumentParser()
for name in ('source', 'proof', 'origin', 'output'): p.add_argument('--' + name, type=Path, required=True)
a = p.parse_args(); origin = json.loads(a.origin.read_text())
assert origin['outcome'] == 'passed' and origin['source_commit'] == SOURCE and origin['source_tree'] == TREE
assert origin['review_commit'] == os.environ['GITHUB_SHA'] and origin['macos']['fresh_complete_group'] and origin['macos']['gate_count'] == 8
assert origin['android']['conclusion'] == 'success' and origin['android']['reused_unchanged_full_build'] and not origin['android']['rebuilt_in_retry']
assert not origin['original_mac_failure']['counted_as_pass'] and origin['no_product_source_changes']
assert not origin['original_defaults_failure']['counted_as_pass'] and origin['original_defaults_failure']['external_metadata_adapter_explicit']
receipt = publish(a.source, a.proof, a.output)
receipt['required_successful_jobs'] = ['current-run complete macos group', 'verified original Android job 116225902823']
receipt['validation_origin_sha256'] = hashlib.sha256(a.origin.read_bytes()).hexdigest()
receipt['validation_origins'] = origin
a.output.write_text(json.dumps(receipt, indent=2) + '\n')
