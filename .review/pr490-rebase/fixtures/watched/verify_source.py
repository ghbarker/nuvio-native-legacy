#!/usr/bin/env python3
"""Review-only source invariants for the PR #490 rebase; no services."""
import argparse
import json
import re
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--root", type=Path, default=Path.cwd())
args = parser.parse_args()
root = args.root
base = "e3e21a590120ca283bcf7a96df1813ff5041eb66"
old = "dbf67f6996404a7465ca3e9ecf7175f0f43b753b"

def git(*items):
    return subprocess.check_output(["git", "-C", str(root), *items], text=True)

def additions_deletions(patch):
    result = {}
    current = None
    for line in patch.splitlines():
        if line.startswith("diff --git "):
            current = line.split(" b/", 1)[1]
            result[current] = []
        elif line.startswith(("+", "-")) and not line.startswith(("+++", "---")):
            result[current].append(line)
    return result

assert additions_deletions(git("diff", "--unified=0", old + "^", old)) == additions_deletions(git("diff", "--unified=0", base, "HEAD"))
assert not git("diff", base, "HEAD", "--", "tests", ".github", "tools")
assert git("ls-tree", base, "tests") == git("ls-tree", "HEAD", "tests")
original = git("show", base + ":src/ajustes.c")
current = (root / "src/ajustes.c").read_text()
checks = [
    (r"typedef enum \{(?:(?!typedef enum).)*?\} OpcaoId;", "  // #366: discovery rows only, per profile on this device. Append-only.\n  AJ_OCULTAR_FILMES_VISTOS,\n"),
    (r"static const Opcao OPCOES\[AJ_N\] = \{.*?\n\};", '  ESC("Ocultar filmes assistidos", V_LIGA, 2), // por perfil, local: hideWatchedMoviesLocal\n'),
    (r"static const char \*CHAVE\[\] = \{.*?\n\};", '  "hideWatchedMoviesLocal",\n'),
]
for pattern, addition in checks:
    before = re.search(pattern, original, re.S)[0]
    after = re.search(pattern, current, re.S)[0]
    assert after.count(addition) == 1
    assert after.replace(addition, "") == before
original_defaults = git("show", base + ":src/ajustes_ux_padrao.inc")
assert (root / "src/ajustes_ux_padrao.inc").read_text() == original_defaults + "  1, /* hide watched movies (#366): Desligado, per profile on this device */\n"
print(json.dumps({"identical_feature_lines": True, "all_upstream_setting_vectors_retained": True, "original_tests_unchanged": True, "no_test_logging_workflow_changes": True}))
