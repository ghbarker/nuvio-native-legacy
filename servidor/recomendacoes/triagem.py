#!/usr/bin/env python3

import subprocess
import json
import os
import sys
import re
from collections import defaultdict
from datetime import datetime

CLOUDFLARE_API_TOKEN = os.environ.get('CLOUDFLARE_API_TOKEN')
CLOUDFLARE_ACCOUNT_ID = os.environ.get('CLOUDFLARE_ACCOUNT_ID')

def exec_d1(sql):
    """Execute a D1 query and return the results."""
    cmd = [
        'npx', 'wrangler', 'd1', 'execute', 'nuvio-recomendacoes',
        '--remote', '--json',
        '--command', sql
    ]

    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Error executing D1 query: {e.output}", file=sys.stderr)
        sys.exit(1)

    # Remove ANSI escape codes
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    output = ansi_escape.sub('', output)

    # Find JSON array start (skip warning prefix)
    json_start = output.find('[')
    if json_start == -1:
        print(f"Failed to parse D1 output: {output}", file=sys.stderr)
        sys.exit(1)

    # Extract JSON from lines starting with '['
    lines = output.split('\n')
    json_str = None
    for i, line in enumerate(lines):
        if line.strip().startswith('['):
            json_str = '\n'.join(lines[i:])
            break

    if json_str is None:
        print(f"Failed to find JSON in D1 output", file=sys.stderr)
        print(f"Output: {output[:500]}", file=sys.stderr)
        sys.exit(1)

    try:
        result = json.loads(json_str)
        if not result or not result[0].get('success'):
            print(f"D1 query failed: {result[0] if result else 'unknown error'}", file=sys.stderr)
            sys.exit(1)
        return result[0]['results']
    except json.JSONDecodeError as e:
        print(f"JSON parse error: {e}", file=sys.stderr)
        print(f"JSON string: {json_str[:500]}", file=sys.stderr)
        sys.exit(1)

def extract_patterns(texto):
    """Extract known patterns from log text."""
    patterns = []

    if 'Aborted(' in texto or 'RuntimeError' in texto:
        patterns.append('Aborted/RuntimeError (WASM crash)')
    if 'nao se despediu' in texto:
        patterns.append('nao se despediu')
    if '[video]' in texto and 'avplay' in texto:
        patterns.append('[video] avplay error')
    if '[mkvass]' in texto or '[legenda]' in texto:
        if 'falha' in texto or 'fallback' in texto or 'Fallback' in texto:
            patterns.append('[mkvass/legenda] failure or fallback')
    if 'montagem descartada' in texto:
        patterns.append('montagem descartada')
    if 'decode falhou' in texto or 'decode failed' in texto:
        patterns.append('decode failed')
    if '[debrid]' in texto and 'PLAN_RESTRICTED' in texto:
        patterns.append('[debrid] PLAN_RESTRICTED')
    if 'longtask-max=' in texto:
        import re
        match = re.search(r'longtask-max=(\d+)', texto)
        if match and int(match.group(1)) > 2000:
            patterns.append('longtask-max > 2000ms')
    if '[desc]' in texto and ('fora por cota' in texto or 'quota' in texto):
        patterns.append('[desc] quota exceeded')
    if 'diagnostico=v2' in texto:
        patterns.append('diagnostico=v2 report')

    return patterns if patterns else ['outros']

def format_github_comment(patterns_data):
    """Format the triage data as a GitHub comment."""
    if not patterns_data:
        return "No new logs to report."

    timestamp = datetime.utcnow().isoformat() + 'Z'

    comment = f"""## 📊 Automatic Log Triage Report

**Last update:** {timestamp}
**New logs analyzed:** {patterns_data['total_logs']}
**Unique users affected:** {patterns_data['unique_people']}

### 📋 Pattern Summary

"""

    # Group patterns by status
    new_patterns = [p for p in patterns_data['patterns'] if p['status'] == 'novo']
    known_patterns = [p for p in patterns_data['patterns'] if p['status'] == 'conhecido']
    regressions = [p for p in patterns_data['patterns'] if p['status'] == 'regrediu']

    if regressions:
        comment += "### ⚠️ Regressions Detected\n\n"
        for pattern in regressions:
            comment += f"- **{pattern['padrao']}** ({pattern['versao']}, {pattern['plataforma']}): {pattern['ocorrencias']} occurrences, {pattern['pessoas']} users\n"
        comment += "\n"

    if new_patterns:
        comment += "### 🆕 New Patterns\n\n"
        for pattern in new_patterns:
            comment += f"- **{pattern['padrao']}** ({pattern['versao']}, {pattern['plataforma']}): {pattern['ocorrencias']} occurrences, {pattern['pessoas']} users\n"
        comment += "\n"

    if known_patterns:
        comment += "### 📌 Known Patterns (Continued)\n\n"
        for pattern in known_patterns[:5]:  # Limit to top 5 to keep comment size manageable
            comment += f"- **{pattern['padrao']}** ({pattern['versao']}, {pattern['plataforma']}): {pattern['ocorrencias']} occurrences, {pattern['pessoas']} users\n"
        if len(known_patterns) > 5:
            comment += f"- ... and {len(known_patterns) - 5} more known patterns\n"
        comment += "\n"

    comment += "\n---\n*Automated triage system*"

    return comment

def run_triage():
    """Run the complete triage routine."""
    print("Starting log triage...")

    # Get last triage checkpoint
    max_results = exec_d1("SELECT max(ultimo_log) as max_id FROM triagem")
    last_log_id = max_results[0]['max_id'] if max_results and max_results[0]['max_id'] else 0

    print(f"Last triage checkpoint: {last_log_id}")

    # Get new logs
    new_logs = exec_d1(
        f"SELECT id, pessoa, versao, plataforma, quando, texto FROM registro "
        f"WHERE id > {last_log_id} AND pessoa != 'trakt:iqui27' "
        f"ORDER BY id ASC"
    )

    print(f"Found {len(new_logs)} new logs")

    if not new_logs:
        print("No new logs, exiting")
        return None

    # Aggregate patterns
    triage_map = {}
    distinct_people = set()
    max_new_log_id = last_log_id

    for log in new_logs:
        max_new_log_id = max(max_new_log_id, log['id'])
        distinct_people.add(log['pessoa'])

        patterns = extract_patterns(log['texto'])

        for padrao in patterns:
            key = f"{padrao}|{log['versao']}|{log['plataforma']}"
            if key not in triage_map:
                triage_map[key] = {
                    'padrao': padrao,
                    'versao': log['versao'],
                    'plataforma': log['plataforma'],
                    'ocorrencias': 0,
                    'pessoas': set(),
                }
            triage_map[key]['ocorrencias'] += 1
            triage_map[key]['pessoas'].add(log['pessoa'])

    # Get existing triage entries for state detection
    existing_triagem = exec_d1("SELECT padrao, versao, plataforma, estado FROM triagem ORDER BY criado DESC")

    state_map = {}
    for row in existing_triagem:
        key = f"{row['padrao']}|{row['versao']}|{row['plataforma']}"
        state_map[key] = row['estado']

    # Prepare triagem data
    triagem_rows = []
    patterns_summary = []
    now = int(datetime.utcnow().timestamp())

    for key, row in triage_map.items():
        estado = 'novo'

        if key in state_map:
            prev_estado = state_map[key]
            if prev_estado.startswith('corrigido-em-'):
                fixed_version = prev_estado.replace('corrigido-em-', '')
                if row['versao'] >= fixed_version:
                    estado = 'regrediu'
                else:
                    estado = prev_estado
            else:
                estado = 'conhecido'

        triagem_rows.append({
            'criado': now,
            'ultimo_log': max_new_log_id,
            'padrao': row['padrao'],
            'versao': row['versao'],
            'plataforma': row['plataforma'],
            'ocorrencias': row['ocorrencias'],
            'pessoas': len(row['pessoas']),
            'estado': estado,
        })

        patterns_summary.append({
            'padrao': row['padrao'],
            'versao': row['versao'],
            'plataforma': row['plataforma'],
            'ocorrencias': row['ocorrencias'],
            'pessoas': len(row['pessoas']),
            'status': estado,
        })

    # Insert new triagem entries
    for row in triagem_rows:
        escaped_padrao = row['padrao'].replace("'", "''")
        sql = (
            f"INSERT INTO triagem (criado, ultimo_log, padrao, versao, plataforma, ocorrencias, pessoas, estado) "
            f"VALUES ({row['criado']}, {row['ultimo_log']}, '{escaped_padrao}', "
            f"'{row['versao']}', '{row['plataforma']}', {row['ocorrencias']}, {row['pessoas']}, '{row['estado']}')"
        )
        try:
            exec_d1(sql)
        except Exception as e:
            print(f"Warning: Failed to insert triagem row: {e}")

    print(f"Inserted {len(triagem_rows)} triagem rows")

    return {
        'total_logs': len(new_logs),
        'unique_people': len(distinct_people),
        'max_new_log_id': max_new_log_id,
        'patterns': patterns_summary,
    }

def main():
    if not CLOUDFLARE_API_TOKEN or not CLOUDFLARE_ACCOUNT_ID:
        print("Error: Missing CLOUDFLARE_API_TOKEN or CLOUDFLARE_ACCOUNT_ID", file=sys.stderr)
        sys.exit(1)

    summary = run_triage()

    if summary:
        print("\n=== TRIAGE SUMMARY ===")
        print(f"Total new logs: {summary['total_logs']}")
        print(f"Unique people: {summary['unique_people']}")
        print(f"Patterns found: {len(summary['patterns'])}")

        # Format and prepare GitHub comment
        comment = format_github_comment(summary)
        print("\n=== GITHUB COMMENT ===")
        print(comment)

        # Save for potential GitHub posting
        with open('/tmp/triagem_comment.txt', 'w') as f:
            f.write(comment)
        print("\nComment saved to /tmp/triagem_comment.txt")
    else:
        print("No new data to report")

if __name__ == '__main__':
    main()
