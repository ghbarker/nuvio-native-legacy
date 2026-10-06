#!/usr/bin/env node

import { execSync } from 'child_process';
import { readFileSync } from 'fs';

const KNOWN_PATTERNS = {
  'Aborted': 'WASM crash (Samsung)',
  'RuntimeError': 'WASM runtime error',
  'nao se despediu': 'Dead session (no goodbye)',
  '[video] avplay erro': 'Video player error',
  '[mkvass]': 'MKV subtitle error',
  '[legenda]': 'Subtitle loading failed',
  'montagem descartada': 'Discarded assembly',
  'decode falhou': 'Decode failed',
  '[debrid]': 'Debrid service error',
  '[desc]': 'Addon quota exceeded',
  'longtask-max=': 'Long task (>2000ms)',
};

async function runQuery(sql) {
  try {
    const output = execSync(
      `cd /home/user/nuvio-native-legacy && npx --yes wrangler d1 execute nuvio-recomendacoes --remote --json --command "${sql.replace(/"/g, '\\"')}"`,
      { encoding: 'utf-8', maxBuffer: 50 * 1024 * 1024 }
    );

    // Parse JSON output (skip noise before first [)
    const jsonStart = output.indexOf('[');
    if (jsonStart === -1) {
      console.error('No JSON found in output:', output);
      return [];
    }

    const jsonStr = output.substring(jsonStart);
    const jsonEnd = jsonStr.lastIndexOf(']') + 1;
    return JSON.parse(jsonStr.substring(0, jsonEnd));
  } catch (e) {
    if (e.message.includes('7403')) {
      console.log('Transient error 7403, retrying...');
      await new Promise(r => setTimeout(r, 3000));
      return runQuery(sql);
    }
    console.error('Query error:', e.message);
    return [];
  }
}

async function getMaxTriagemId() {
  const results = await runQuery('SELECT max(ultimo_log) as max_id FROM triagem');
  return results[0]?.max_id || 0;
}

async function getNewRegistros(afterId) {
  const sql = `SELECT id, pessoa, versao, plataforma, quando, texto FROM registro
               WHERE id > ${afterId} AND pessoa != 'trakt:iqui27'
               ORDER BY id`;
  return await runQuery(sql);
}

function extractPattern(texto) {
  // Buscar padrões conhecidos no texto
  for (const [pattern, label] of Object.entries(KNOWN_PATTERNS)) {
    if (texto.includes(pattern)) {
      return pattern;
    }
  }

  // Diagnóstico v2 - contar aplicacao values
  if (texto.includes('diagnostico=v2')) {
    return 'diagnostico-v2';
  }

  // Sessão normal ou outro padrão
  if (texto.includes('fim')) {
    return null; // Normal termination, ignore
  }

  // Padrão desconhecido - usar primeiros 50 chars
  return texto.substring(0, 50);
}

async function updateTriagem(patterns, maxId) {
  const now = Math.floor(Date.now() / 1000);

  for (const [pattern, data] of Object.entries(patterns)) {
    if (!pattern) continue; // Skip null patterns

    // Verificar se já existe
    const existing = await runQuery(
      `SELECT id, ocorrencias, pessoas, estado FROM triagem
       WHERE padrao = '${pattern.replace(/'/g, "''")}'`
    );

    const ocorrencias = data.count;
    const pessoas = data.uniquePeople.size;

    if (existing.length > 0) {
      const prev = existing[0];
      let estado = prev.estado;

      // Detectar regressão
      if (prev.estado.startsWith('corrigido-em-')) {
        estado = 'regrediu';
      }

      // Atualizar
      await runQuery(
        `UPDATE triagem SET
         ultimo_log = ${maxId},
         ocorrencias = ${ocorrencias},
         pessoas = ${pessoas},
         estado = '${estado}'
         WHERE padrao = '${pattern.replace(/'/g, "''")}'`
      );
    } else {
      // Inserir novo
      await runQuery(
        `INSERT INTO triagem (criado, ultimo_log, padrao, versao, plataforma, ocorrencias, pessoas, estado)
         VALUES (${now}, ${maxId}, '${pattern.replace(/'/g, "''")}', '${data.version.replace(/'/g, "''")}', '${data.platform.replace(/'/g, "''")}', ${ocorrencias}, ${pessoas}, 'novo')`
      );
    }
  }
}

async function linkToIssues(patterns) {
  // Buscar issues abertas
  const issues = JSON.parse(
    execSync(
      'cd /home/user/nuvio-native-legacy && gh issue list --state open --json number,title,labels --limit 100',
      { encoding: 'utf-8' }
    )
  );

  const linked = {};

  for (const [pattern, data] of Object.entries(patterns)) {
    if (!pattern) continue;

    // Match by pattern or issue number
    for (const issue of issues) {
      const title = issue.title.toLowerCase();
      const pattern_lower = pattern.toLowerCase();

      if (title.includes(pattern_lower) || title.includes(pattern)) {
        linked[pattern] = issue.number;
        break;
      }
    }
  }

  return linked;
}

async function commentOnTriagemIssue(summary) {
  // Procurar pela issue fixa "Relatório de logs"
  const issues = JSON.parse(
    execSync(
      'cd /home/user/nuvio-native-legacy && gh issue list --state open --search "Relatório de logs" --json number,title --limit 5',
      { encoding: 'utf-8' }
    )
  );

  let triageIssue = issues.find(i => i.title.includes('Relatório de logs'));

  if (!triageIssue) {
    console.log('No "Relatório de logs" issue found, skipping comment');
    return;
  }

  // Comentar com o resumo agregado
  execSync(
    `cd /home/user/nuvio-native-legacy && gh issue comment ${triageIssue.number} --body '${summary.replace(/'/g, "'\\''")}'`,
    { encoding: 'utf-8' }
  );

  console.log(`Commented on issue #${triageIssue.number}`);
}

async function main() {
  console.log('Starting triage automation...');

  const maxId = await getMaxTriagemId();
  console.log(`Last processed ID: ${maxId}`);

  const registros = await getNewRegistros(maxId);
  console.log(`Found ${registros.length} new registros`);

  if (registros.length === 0) {
    console.log('No new registros to process');
    return;
  }

  // Agrupar por padrão
  const patterns = {};

  for (const reg of registros) {
    const pattern = extractPattern(reg.texto);
    if (!pattern) continue;

    const key = `${pattern}|${reg.versao}|${reg.plataforma}`;

    if (!patterns[key]) {
      patterns[key] = {
        pattern,
        version: reg.versao,
        platform: reg.plataforma,
        count: 0,
        uniquePeople: new Set(),
      };
    }

    patterns[key].count++;
    patterns[key].uniquePeople.add(reg.pessoa);
  }

  // Atualizar triagem
  await updateTriagem(patterns, maxId);

  // Linkar issues
  const linked = await linkToIssues(patterns);

  // Gerar resumo
  let summary = '## Log Triage Report\n\n';
  summary += `Processed registros up to ID ${maxId}.\n\n`;

  summary += '### New Patterns\n';
  for (const [key, data] of Object.entries(patterns)) {
    if (data.pattern) {
      const issue = linked[data.pattern];
      const issueLinkage = issue ? ` (see #${issue})` : '';
      summary += `- **${data.pattern}**: ${data.count} occurrences, ${data.uniquePeople.size} users, v${data.version}, ${data.platform}${issueLinkage}\n`;
    }
  }

  summary += '\n---\n_Automated triage report_';

  console.log('Summary:', summary);

  // Comentar na issue fixa
  await commentOnTriagemIssue(summary);
}

main().catch(console.error);
