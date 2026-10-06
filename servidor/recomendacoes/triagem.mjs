#!/usr/bin/env node
import { execSync } from 'child_process';

const PATTERN_RULES = [
  { pattern: /Aborted.*RuntimeError.*WASM/i, category: 'crash-wasm' },
  { pattern: /Aborted|RuntimeError/i, category: 'crash' },
  { pattern: /\[video\].*avplay.*error/i, category: 'video-player' },
  { pattern: /\[legenda\]|\[mkvass\].*ass/i, category: 'legendas-ass' },
  { pattern: /nao se despediu|não se despediu/i, category: 'sessao-morta' },
  { pattern: /montagem descartada/i, category: 'montagem-descartada' },
  { pattern: /decode falhou/i, category: 'decode-erro' },
  { pattern: /\[debrid\].*PLAN_RESTRICTED|baixando/i, category: 'debrid-erro' },
  { pattern: /longtask-max=\s*(\d{4,})/i, category: 'longtask' },
  { pattern: /diagnostico=v2/i, category: 'diagnostico-v2' },
  { pattern: /\[desc\].*cota|quota|de fora/i, category: 'desc-quota' },
];

async function executeD1(sql) {
  try {
    const cmd = `npx --yes wrangler d1 execute nuvio-recomendacoes --remote --json --command "${sql.replace(/"/g, '\\"')}"`;

    const output = execSync(cmd, {
      encoding: 'utf-8',
      maxBuffer: 50 * 1024 * 1024 // 50MB buffer
    });

    // Parse JSON from output (skip warnings)
    const jsonStart = output.indexOf('[');
    if (jsonStart === -1) {
      console.error('No JSON found in output');
      return null;
    }

    const jsonStr = output.substring(jsonStart);
    const parsed = JSON.parse(jsonStr);

    // Extract results from wrangler response
    if (Array.isArray(parsed) && parsed[0] && parsed[0].results) {
      return parsed[0].results;
    }
    return parsed;
  } catch (error) {
    console.error('D1 Error:', error.message);
    throw error;
  }
}

function extractPattern(texto) {
  if (!texto) return 'outro';

  for (const rule of PATTERN_RULES) {
    if (rule.pattern.test(texto)) {
      return rule.category;
    }
  }

  return 'outro';
}

async function postGitHubComment(issueNumber, summary) {
  try {
    // Use curl to post via GitHub API since gh has restrictions
    let comment = '## Triage Summary\n\n';
    comment += `**Processing date:** ${new Date().toISOString()}\n`;
    comment += `**New logs processed:** ${summary.totalNew}\n\n`;

    if (summary.patterns.length > 0) {
      comment += '### Pattern Distribution\n\n';
      comment += '| Pattern | Version | Platform | Occurrences | Users |\n';
      comment += '|---------|---------|----------|------------|-------|\n';

      for (const item of summary.patterns.sort((a, b) => b.count - a.count)) {
        comment += `| \`${item.pattern}\` | ${item.versao || '-'} | ${item.plataforma || '-'} | ${item.count} | ${item.pessoas} |\n`;
      }
    }

    comment += '\n---\n';

    // Escape quotes for JSON
    const body = comment.replace(/"/g, '\\"');

    // Use REST API endpoint
    const cmd = `curl -s -X POST https://api.github.com/repos/iqui27/nuvio-native-legacy/issues/${issueNumber}/comments -H "Authorization: token ${process.env.GITHUB_TOKEN}" -H "Accept: application/vnd.github.v3+json" -d '{"body": "${body}"}'`;

    const output = execSync(cmd, { encoding: 'utf-8' });
    const result = JSON.parse(output);

    if (result.id) {
      console.log(`✓ Comment posted to issue #${issueNumber}`);
      return true;
    } else {
      console.error('Failed to post comment:', result);
      return false;
    }
  } catch (error) {
    console.error('Error posting comment:', error.message);
    return false;
  }
}

async function main() {
  console.log('🔍 Iniciando triagem de logs...\n');

  // Get the latest triagem entry
  const maxResult = await executeD1('SELECT MAX(ultimo_log) as max_id FROM triagem');
  const maxId = maxResult && maxResult[0] ? maxResult[0].max_id || 0 : 0;

  console.log(`📍 Último log processado: ${maxId}`);

  // Get count of new logs
  const countQuery = `SELECT COUNT(*) as count FROM registro WHERE id > ${maxId} AND pessoa != 'trakt:iqui27'`;
  const countResult = await executeD1(countQuery);
  const totalNew = countResult && countResult[0] ? countResult[0].count : 0;

  if (totalNew === 0) {
    console.log('✅ Nenhum novo registro para processar');
    return;
  }

  console.log(`📊 Processando ${totalNew} registros novos\n`);

  // Get new logs in batches to extract patterns
  const grouped = {};
  const limit = 50;

  for (let offset = 0; offset < totalNew; offset += limit) {
    const query = `
      SELECT id, pessoa, versao, plataforma, texto
      FROM registro
      WHERE id > ${maxId} AND pessoa != 'trakt:iqui27'
      ORDER BY id ASC
      LIMIT ${limit}
      OFFSET ${offset}
    `;

    const registros = await executeD1(query);

    if (!registros || registros.length === 0) break;

    for (const reg of registros) {
      const pattern = extractPattern(reg.texto);
      const key = `${pattern}|${reg.versao}|${reg.plataforma}`;

      if (!grouped[key]) {
        grouped[key] = {
          pattern,
          versao: reg.versao,
          plataforma: reg.plataforma,
          count: 0,
          pessoas: new Set(),
          maxId: 0
        };
      }

      grouped[key].count++;
      grouped[key].pessoas.add(reg.pessoa);
      grouped[key].maxId = Math.max(grouped[key].maxId, reg.id);
    }
  }

  // Update triagem table
  console.log('💾 Atualizando tabela de triagem...\n');

  const patterns = [];

  for (const [key, data] of Object.entries(grouped)) {
    const agora = Math.floor(Date.now() / 1000);
    const pessoasCount = data.pessoas.size;

    const insertSql = `
      INSERT INTO triagem (criado, ultimo_log, padrao, versao, plataforma, ocorrencias, pessoas, estado)
      VALUES (${agora}, ${data.maxId}, '${data.pattern.replace(/'/g, "''")}', '${data.versao.replace(/'/g, "''")}', '${data.plataforma.replace(/'/g, "''")}', ${data.count}, ${pessoasCount}, 'novo')
    `;

    try {
      await executeD1(insertSql);
      console.log(`✓ ${data.pattern} | v${data.versao} | ${data.plataforma}: ${data.count} ocorrências, ${pessoasCount} pessoas`);
      patterns.push({
        pattern: data.pattern,
        versao: data.versao,
        plataforma: data.plataforma,
        count: data.count,
        pessoas: pessoasCount
      });
    } catch (error) {
      console.error(`✗ Erro ao gravar triagem: ${key}`);
    }
  }

  console.log('\n✅ Triagem concluída!');

  // Post summary to GitHub (issue #270)
  console.log('\n📤 Postando relatório no GitHub...\n');
  await postGitHubComment(270, { totalNew, patterns });
}

main().catch(error => {
  console.error('Erro fatal:', error.message);
  process.exit(1);
});
