#!/usr/bin/env python3
"""
Log Triaging Routine for Nuvio
Reads new logs from Cloudflare D1, aggregates patterns, updates triagem, posts to GitHub
"""

import json
import subprocess
import re
from collections import defaultdict
from datetime import datetime
import sys

DB_NAME = "nuvio-recomendacoes"
OWNER_ID = "trakt:iqui27"
REC_PATH = "/home/user/nuvio-native-legacy/servidor/recomendacoes"

# Pattern detection
PATTERNS = {
    "Aborted crash": r"Aborted.*RuntimeError|crash.*WASM",
    "nao se despediu": r"nao se despediu",
    "[video] avplay error": r"\[video\].*avplay.*error.*errorText(?!.*No Error)",
    "[mkvass] fallback": r"\[mkvass\].*fallback",
    "[legenda] ASS fallback": r"\[legenda\].*fallback.*ASS",
    "montagem discarded": r"montagem descartada",
    "decode error": r"decode falhou",
    "[debrid] TorBox": r"\[debrid\].*TorBox.*PLAN_RESTRICTED",
    "longtask timeout": r"longtask-max",
    "[desc] quota": r"\[desc\].*de fora por cota",
    "native crash": r"motivo=crash-nativo",
    "anr": r"motivo=anr",
    "oom kill": r"oom.*kill|killed.*oom",
}

def run_d1_query(sql):
    """Execute D1 query via wrangler"""
    try:
        cmd = [
            "npx", "--yes", "wrangler", "d1", "execute", DB_NAME,
            "--remote", "--json",
            "--command", sql
        ]
        result = subprocess.run(cmd, cwd=REC_PATH, capture_output=True, text=True, timeout=30)

        if result.returncode != 0:
            if "7403" in result.stderr:
                raise Exception("D1 transient error (7403)")
            print(f"Error: {result.stderr}", file=sys.stderr)
            return None

        # Parse JSON from stdout (skip warning lines)
        lines = result.stdout.split('\n')
        json_start = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('['):
                json_start = i
                break

        if json_start == -1:
            print(f"No JSON found in output", file=sys.stderr)
            return None

        json_str = '\n'.join(lines[json_start:])
        data = json.loads(json_str)
        return data[0].get('results', [])
    except subprocess.TimeoutExpired:
        print("Query timeout", file=sys.stderr)
        return None
    except Exception as e:
        print(f"Query error: {e}", file=sys.stderr)
        return None

def detect_pattern(text):
    """Detect which pattern matches this log text"""
    for name, regex in PATTERNS.items():
        if re.search(regex, text, re.IGNORECASE):
            return name
    return None

def triagem():
    """Main triaging logic"""
    print("🔍 Starting log triage routine...")

    # Get latest triagem ID
    result = run_d1_query("SELECT MAX(ultimo_log) as max_id FROM triagem")
    if not result:
        print("⚠️  Could not read triagem table")
        return None

    last_id = result[0].get('max_id') or 0
    print(f"📊 Last processed log ID: {last_id}")

    # Get new logs
    sql = f"""
    SELECT id, pessoa, versao, plataforma, quando, texto
    FROM registro
    WHERE id > {last_id} AND pessoa != '{OWNER_ID}'
    ORDER BY id ASC
    LIMIT 1000
    """

    registros = run_d1_query(sql)
    if not registros:
        print("✅ No new logs to process")
        return None

    print(f"📝 Found {len(registros)} new logs")

    # Aggregate by pattern
    aggregated = defaultdict(lambda: {
        'pattern': None,
        'version': None,
        'platform': None,
        'count': 0,
        'people': set(),
    })

    for log in registros:
        pattern = detect_pattern(log['texto'])
        if not pattern:
            pattern = 'other'

        key = f"{pattern}|{log['versao']}|{log['plataforma']}"
        if aggregated[key]['pattern'] is None:
            aggregated[key]['pattern'] = pattern
            aggregated[key]['version'] = log['versao'] or 'unknown'
            aggregated[key]['platform'] = log['plataforma'] or 'unknown'

        aggregated[key]['count'] += 1
        aggregated[key]['people'].add(log['pessoa'])

    # Update triagem table
    max_id = max(log['id'] for log in registros)
    now = int(datetime.now().timestamp())

    for key, data in aggregated.items():
        if data['pattern'] == 'other':
            continue  # Skip aggregating 'other' pattern

        sql = f"""
        INSERT INTO triagem (criado, ultimo_log, padrao, versao, plataforma, ocorrencias, pessoas, estado)
        VALUES ({now}, {max_id}, '{data['pattern'].replace("'", "''")}', '{data['version']}', '{data['platform']}', {data['count']}, {len(data['people'])}, 'novo')
        """

        result = run_d1_query(sql)
        if result is None:
            print(f"⚠️  Failed to insert triagem for {data['pattern']}")

    return {
        'count': len(registros),
        'max_id': max_id,
        'patterns': aggregated,
        'timestamp': datetime.now().isoformat(),
    }

def generate_summary(result):
    """Generate GitHub comment summary"""
    if result is None:
        return None

    # Sort patterns by count
    sorted_patterns = sorted(
        result['patterns'].items(),
        key=lambda x: x[1]['count'],
        reverse=True
    )

    md = f"""## Log Triage Report

**Last Updated:** {result['timestamp']}
**New Logs Processed:** {result['count']}
**Max Log ID:** {result['max_id']}

### Detected Patterns

"""

    if not sorted_patterns or all(p[1]['pattern'] == 'other' for p in sorted_patterns):
        md += "No specific patterns detected in this batch.\n"
    else:
        for key, data in sorted_patterns:
            if data['pattern'] == 'other':
                continue

            md += f"- **{data['pattern']}** — {data['count']} occurrence{'s' if data['count'] > 1 else ''} by {len(data['people'])} user{'s' if len(data['people']) > 1 else ''}\n"
            md += f"  - Version: `{data['version']}`\n"
            md += f"  - Platform: `{data['platform']}`\n"

    md += "\n---\n"
    md += "_Generated by log triage automation_\n"

    return md

if __name__ == "__main__":
    try:
        result = triagem()
        summary = generate_summary(result)

        if summary:
            print("\n" + summary)
            print("\n✅ Triagem complete")
        else:
            print("✅ Nothing to report")
    except Exception as e:
        print(f"❌ Error: {e}", file=sys.stderr)
        sys.exit(1)
