#!/usr/bin/env python3
"""Retain loopback HTTP fixture diagnostics before its temporary directory closes.

The product and test assertions remain unchanged. This only changes the CI
working-copy cleanup trap, copying a failed fixture's log to the review artifact.
"""
from pathlib import Path

for name in ('jellyfin', 'plex', 'emby'):
    path = Path('tests')/(name+'.sh')
    text = path.read_text()
    old = "trap '[ -z \"$srv\" ] || kill \"$srv\" 2>/dev/null || true; rm -rf \"$tmp\"' EXIT"
    new = '''profile_http_cleanup() {
  profile_http_status=$?
  if [ "$profile_http_status" -ne 0 ] && [ -f "$tmp/log" ] && [ -n "${NUVIO_PROFILE_RESULTS:-}" ]; then
    mkdir -p "$NUVIO_PROFILE_RESULTS"
    cp "$tmp/log" "$NUVIO_PROFILE_RESULTS/http-@NAME@-failure.log"
  fi
  [ -z "$srv" ] || kill "$srv" 2>/dev/null || true
  rm -rf "$tmp"
  return "$profile_http_status"
}
trap profile_http_cleanup EXIT'''.replace('@NAME@',name)
    if new in text:
        continue
    assert text.count(old)==1, path
    path.write_text(text.replace(old,new))
# The original Plex fixture stores TMPDIR plus a filename in only 40 bytes.
# Fix its buffer in this working copy, keeping the full intended path/assertion.
plex_fixture = Path('tests/plex.c')
text = plex_fixture.read_text()
old = '  { char nome[40];\n    FILE *f;\n    snprintf(nome, sizeof nome, "%s/plex-p3.txt", dirDados);'
new = old.replace('char nome[40]', 'char nome[700]')
if new not in text:
    assert text.count(old)==1, plex_fixture
    plex_fixture.write_text(text.replace(old,new))
print('Loopback HTTP fixture assertions preserved; failed logs retained as review artifacts')
