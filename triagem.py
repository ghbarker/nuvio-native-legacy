#!/usr/bin/env python3

import subprocess
import json
import sys
from collections import defaultdict
from datetime import datetime

KNOWN_PATTERNS = {
    'Aborted': 'WASM crash',
    'RuntimeError': 'WASM runtime error',
    'nao se despediu': 'Dead session',
    '[video] avplay erro': 'Video player error',
    '[mkvass]': 'MKV subtitle error',
    '[legenda]': 'Subtitle loading failed',
    'montagem descartada': 'Discarded assembly',
    'decode falhou': 'Decode failed',
    '[debrid]': 'Debrid service error',
    '[desc]': 'Addon quota exceeded',
    'longtask-max=': 'Long task',
}

def run_wrangler(sql):
    """Execute Wrangler D1 command and parse JSON"""
    cmd = [
        'npx', '--yes', 'wrangler', 'd1', 'execute',
        'nuvio-recomendacoes', '--remote', '--json', '--command', sql
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd='/home/user/nuvio-native-legacy')

    if result.returncode != 0:
        print(f"Error: {result.stderr}", file=sys.stderr)
        return []

    # Extract JSON (skip warnings/noise)
    output = result.stdout
    try:
        start = output.index('[')
        end = output.rindex(']') + 1
        json_str = output[start:end]
        data = json.loads(json_str)
        return data[0]['results'] if data and len(data) > 0 else []
    except (ValueError, IndexError) as e:
        print(f"Parse error: {e}", file=sys.stderr)
        return []

def get_new_registros(after_id, batch_size=500):
    """Fetch new registros in batches"""
    all_registros = []
    offset = 0

    while True:
        sql = f"""
        SELECT id, pessoa, versao, plataforma, quando, texto
        FROM registro
        WHERE id > {after_id} AND pessoa != 'trakt:iqui27'
        ORDER BY id
        LIMIT {batch_size} OFFSET {offset}
        """

        batch = run_wrangler(sql)
        if not batch:
            break

        all_registros.extend(batch)
        if len(batch) < batch_size:
            break

        offset += batch_size
        print(f"Fetched {len(all_registros)} registros so far...", file=sys.stderr)

    return all_registros

def extract_pattern(texto):
    """Extract known pattern from log text"""
    if not texto:
        return None

    # Check for known patterns
    for pattern in KNOWN_PATTERNS:
        if pattern in texto:
            return pattern

    # Diagnóstico v2
    if 'diagnostico=v2' in texto:
        return 'diagnostico-v2'

    # Sessão normal (fim)
    if 'fim' in texto:
        return None

    # Unknown pattern - use first 50 chars
    return texto[:50].strip()

def analyze_patterns(registros):
    """Group registros by pattern and extract statistics"""
    patterns = defaultdict(lambda: {
        'count': 0,
        'people': set(),
        'versions': set(),
        'platforms': set(),
    })

    for reg in registros:
        pattern = extract_pattern(reg.get('texto', ''))
        if not pattern:
            continue

        key = f"{pattern}|{reg.get('versao', '')}|{reg.get('plataforma', '')}"
        patterns[key]['count'] += 1
        patterns[key]['people'].add(reg.get('pessoa'))
        patterns[key]['versions'].add(reg.get('versao', ''))
        patterns[key]['platforms'].add(reg.get('plataforma', ''))

    return patterns

def update_triagem(patterns, max_id):
    """Update triagem table with new patterns"""
    now = int(datetime.now().timestamp())

    for key, data in patterns.items():
        parts = key.split('|', 2)
        pattern = parts[0]
        version = parts[1] if len(parts) > 1 else ''
        platform = parts[2] if len(parts) > 2 else ''

        ocorrencias = data['count']
        pessoas = len(data['people'])

        # Escape quotes
        pattern_escaped = pattern.replace("'", "''")
        version_escaped = version.replace("'", "''")
        platform_escaped = platform.replace("'", "''")

        # Check if exists
        sql_check = f"SELECT id, estado FROM triagem WHERE padrao = '{pattern_escaped}' AND versao = '{version_escaped}' AND plataforma = '{platform_escaped}'"
        existing = run_wrangler(sql_check)

        if existing:
            # Update
            estado = existing[0]['estado']

            # Check for regression
            if estado.startswith('corrigido-em-'):
                estado = 'regrediu'

            sql_update = f"""
            UPDATE triagem
            SET ultimo_log = {max_id}, ocorrencias = {ocorrencias}, pessoas = {pessoas}, estado = '{estado}'
            WHERE padrao = '{pattern_escaped}' AND versao = '{version_escaped}' AND plataforma = '{platform_escaped}'
            """
            run_wrangler(sql_update)
            print(f"Updated: {pattern} (v{version}, {platform})", file=sys.stderr)
        else:
            # Insert
            sql_insert = f"""
            INSERT INTO triagem (criado, ultimo_log, padrao, versao, plataforma, ocorrencias, pessoas, estado)
            VALUES ({now}, {max_id}, '{pattern_escaped}', '{version_escaped}', '{platform_escaped}', {ocorrencias}, {pessoas}, 'novo')
            """
            run_wrangler(sql_insert)
            print(f"Created: {pattern} (v{version}, {platform})", file=sys.stderr)

def generate_summary(patterns):
    """Generate markdown summary of findings"""
    if not patterns:
        return "No new log patterns found in this period."

    summary = "### New Log Patterns\n\n"

    for key in sorted(patterns.keys()):
        data = patterns[key]
        parts = key.split('|', 2)
        pattern = parts[0]
        version = parts[1] if len(parts) > 1 else 'unknown'
        platform = parts[2] if len(parts) > 2 else 'unknown'

        label = KNOWN_PATTERNS.get(pattern, pattern)
        summary += f"- **{label}**: {data['count']} occurrences, {len(data['people'])} users | v{version} | {platform}\n"

    return summary

def main():
    print("Starting automated log triage...", file=sys.stderr)

    # Get current max triagem ID
    max_triagem_result = run_wrangler("SELECT max(ultimo_log) as max_id FROM triagem")
    max_id = max_triagem_result[0]['max_id'] if max_triagem_result else 0
    print(f"Last processed ID: {max_id}", file=sys.stderr)

    # Get new registros
    registros = get_new_registros(max_id)
    print(f"Found {len(registros)} new registros", file=sys.stderr)

    if not registros:
        print("No new registros to process", file=sys.stderr)
        return

    max_new_id = max(r['id'] for r in registros)
    print(f"Processing up to ID {max_new_id}", file=sys.stderr)

    # Analyze patterns
    patterns = analyze_patterns(registros)
    print(f"Found {len(patterns)} unique patterns", file=sys.stderr)

    # Update triagem table
    update_triagem(patterns, max_new_id)

    # Generate summary
    summary = generate_summary(patterns)
    print("\n" + summary)

    return summary

if __name__ == '__main__':
    summary = main()
    # Save to file for GitHub comment
    with open('/tmp/triage_summary.txt', 'w') as f:
        f.write(summary)
