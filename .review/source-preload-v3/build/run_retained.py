#!/usr/bin/env python3
"""Keep original reduced scripts; adapt only omitted APIs and canonical #205 expectations."""
import argparse
import hashlib
from pathlib import Path
import shlex
import subprocess
import uuid

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--name', required=True)
parser.add_argument('--adapter', type=Path, required=True)
args = parser.parse_args()
script = args.source / 'tests' / (args.name + '.sh')
before = script.read_bytes()
# These three omit perfis.c by design. Other scripts, including the actual
# full-source Settings and original ASan checks, run entirely unchanged.
if args.name not in {'fonte_ordem_addon', 'fontecache_vod', 'fontefalha_worker'}:
    subprocess.run(['bash', str(script)], check=True)
else:
    text = before.decode()
    token = 'tests/' + args.name + '.c'
    assert text.count(token) == 1, (args.name, text.count(token))
    replacement = token + ' ' + shlex.quote(str(args.adapter.resolve()))
    if args.name == 'fonte_ordem_addon':
        # The canonical streams fixture now names real language matches. Keep
        # every old input and link that actual product producer additively.
        replacement += ' src/linguas.c'
    temporary = script.with_name('review-retained-' + args.name + '-' + uuid.uuid4().hex + '.sh')
    review_fixture = None
    fixture_before = None
    fixture = script.with_suffix('.c')
    if args.name == 'fonte_ordem_addon':
        # Canonical #205 places known-negative entries last in EACH quality
        # group; the old #402 fixture expected them last in the whole list.
        # Retain every assertion and addon-mode array in this bounded copy.
        fixture_before = fixture.read_bytes()
        fixture_text = fixture_before.decode()
        changes = [
          ('mistaQ[] = {2,3,1,0}', 'mistaQ[] = {0,2,3,1}'),
          ('hintQ[] = {2,1,0}', 'hintQ[] = {0,2,1}'),
          ('modo = 0; conferir(consultada, 3); modo = 1; conferir(consultada, 3);',
           'const int consultadaQ[] = {0,2,1};\n  modo = 0; conferir(consultadaQ, 3); modo = 1; conferir(consultada, 3);')
        ]
        for old, new in changes:
            assert fixture_text.count(old) == 1, old
            fixture_text = fixture_text.replace(old, new)
        review_fixture = fixture.with_name('review-ordem205-' + uuid.uuid4().hex + '.c')
        review_fixture.write_text(fixture_text)
        print('Bounded review #205 quality-group expectations; all addon expectations and assertions retained:',
              'original_fixture_sha256=' + hashlib.sha256(fixture_before).hexdigest(),
              'effective_fixture_sha256=' + hashlib.sha256(fixture_text.encode()).hexdigest(), flush=True)
        replacement = replacement.replace(token, 'tests/' + review_fixture.name, 1)
    try:
        temporary.write_text(text.replace(token, replacement))
        subprocess.run(['bash', str(temporary)], check=True)
    finally:
        temporary.unlink(missing_ok=True)
        if review_fixture is not None:
            review_fixture.unlink(missing_ok=True)
            assert fixture.read_bytes() == fixture_before, 'Original reduced fixture changed'
assert script.read_bytes() == before, 'Original reduced test script changed'
print('Original reduced recipe preserved:', args.name, hashlib.sha256(before).hexdigest())
