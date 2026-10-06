# Automated Log Triage System

This system automatically processes user-submitted logs from the D1 database, aggregates them by pattern/version/platform, and posts summaries to the GitHub issue tracker.

## Components

### 1. Triage Script (`servidor/recomendacoes/triagem.mjs`)

Node.js script that:
- Queries the D1 `registro` table for new logs
- Extracts patterns using regex rules (crashes, subtitle errors, debrid issues, etc.)
- Groups logs by pattern, version, and platform
- Stores aggregated results in the `triagem` table
- Posts summaries to GitHub issue #270 via the GitHub API

**Pattern Categories:**
- `crash-wasm`: WASM crashes
- `crash`: General runtime errors
- `sessao-morta`: Dead sessions
- `video-player`: Video playback errors
- `legendas-ass`: ASS subtitle format issues
- `montagem-descartada`: Discarded builds
- `decode-erro`: Decoding failures
- `debrid-erro`: Debrid service errors
- `longtask`: Long task hangs (>2s)
- `diagnostico-v2`: Diagnostic reports
- `desc-quota`: Description/catalog quota exceeded
- `outro`: Other/unclassified

### 2. GitHub Action (`.github/workflows/triagem.yml`)

Automated workflow that:
- Runs every 6 hours on schedule
- Triggers on issue open/reopen events
- Can be manually dispatched via workflow_dispatch

## Setup Requirements

### Environment Variables / Secrets

Add these to your GitHub repository secrets:

```
CLOUDFLARE_API_TOKEN=cfat_xxx...
CLOUDFLARE_ACCOUNT_ID=d0429493c4b...
```

These must have D1 database access permissions.

### Database Tables

Required tables (created by migrations):

1. **registro**: User-submitted logs
   - `id`, `pessoa`, `versao`, `plataforma`, `quando`, `texto`, `criado`

2. **triagem**: Aggregation results
   - `id`, `criado`, `ultimo_log`, `padrao`, `versao`, `plataforma`, `ocorrencias`, `pessoas`, `issue`, `estado`

### GitHub Issue

The system posts to issue #270 "📊 Relatório de logs" (pinned with `triagem` label).

## Usage

### Manual Execution

```bash
cd servidor/recomendacoes
node triagem.mjs
```

### Via GitHub Actions

The workflow can be triggered manually:
1. Go to Actions tab
2. Select "Log Triage"
3. Click "Run workflow"

## Data Privacy

- ✅ Logs exclude person IDs (never posted to public GitHub)
- ✅ Only aggregated counts are public
- ✅ Triagem state stored in D1 (private), not in git
- ✅ Excludes owner's logs (`trakt:iqui27`)

## Pattern Detection

Patterns are identified using regex rules against the log text:

```javascript
// Examples
{ pattern: /\[legenda\]|\[mkvass\].*ass/i, category: 'legendas-ass' }
{ pattern: /\[debrid\].*PLAN_RESTRICTED|baixando/i, category: 'debrid-erro' }
```

## Future Enhancements

- [ ] Machine learning classification of unknown patterns
- [ ] Automatic issue linking when patterns match known issues
- [ ] Regression detection (pattern reappears in newer version)
- [ ] Per-addon error tracking
- [ ] User consent for detailed log analysis
