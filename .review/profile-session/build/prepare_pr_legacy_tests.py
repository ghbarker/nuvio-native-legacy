#!/usr/bin/env python3
"""Supply fixed-session neighbours in unchanged reduced upstream fixtures only."""
import importlib.util
from pathlib import Path
import shlex

ROOT = Path(__file__).resolve().parent
legacy_path = ROOT / 'prepare_legacy_tests.py'
if not legacy_path.exists():
    # Local pre-packaging location; CI copies both scripts beside each other.
    legacy_path = ROOT.parent / 'prepare_legacy_tests.py'
spec = importlib.util.spec_from_file_location('profile_legacy', legacy_path)
legacy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(legacy)

PLAYBACK_NEIGHBOURS = r'''// Only the unchanged upstream fontefalha_playback reduced fixture links
// this adapter. It intentionally has one session and no provider accounts.
// All routing, player and mailbox assertions remain in the original fixture.
#include "debrid.h"
#include "streams.h"
#include "stalker.h"
#include <assert.h>
#include <string.h>
static const DebridContexto *entered;
void debrid_contexto_capturar(DebridContexto *c) {
  assert(c); memset(c, 0, sizeof *c);
}
void debrid_contexto_entrar(const DebridContexto *c) {
  assert(c && c->sessao == 0 && !entered); entered = c;
}
void debrid_contexto_sair(void) { assert(entered); entered = 0; }
int stream_primeira_boa_sessao(int n, uint64_t epoch) {
  assert(epoch == 0); return stream_primeira_boa(n);
}
int stream_canal_primeira_viva_sessao(int n, uint64_t epoch) {
  assert(epoch == 0); return stream_canal_primeira_viva(n);
}
int stream_verificar_proxima_vod_sessao(int idx, int n, int *checked, uint64_t epoch) {
  assert(epoch == 0); return stream_verificar_proxima_vod(idx, n, checked);
}
int stalker_resolver_sessao(const char *id, char *url, unsigned n, uint64_t epoch) {
  assert(epoch == 0); return stalker_resolver(id, url, n);
}
int stream_falha_aplicar_sessao(int idx, unsigned generation, int service,
                              const char *url, uint64_t epoch) {
  assert(epoch == 0); return stream_falha_aplicar(idx, generation, service, url);
}
'''

def adapt_provenance_snapshot(path):
    text = path.read_text()
    old = '  fioFontes(&c);'
    new = '''  // Supply the same immutable dispatch snapshot as consultar(). This
  // reduced fixture calls its private worker directly; all existing full-base,
  // partial-publication and plugin identity assertions remain unchanged.
  c.sessao = nv_perfil_sessao_ler();
  c.origens = capturarPedidos(c.sessao, &c.nOrigens, &c.versao);
  assert(c.origens && c.nOrigens == nAddon);
  fioFontes(&c);
  free(c.origens); c.origens = NULL;'''
    if text.count(new) == 2:
        return
    assert text.count(old) == 2 and new not in text, path
    path.write_text(text.replace(old, new))

def adapt_budget_selectors(path):
    text = path.read_text()
    marker = '#include "../src/app.c"'
    aliases = '''// Keep the original initial/channel selector doubles. This fixture's
// NEXT-VOD verifier and app consumer remain real and enforce the total budget.
#define stream_primeira_boa_sessao teste_stream_primeira_boa_sessao
#define stream_canal_primeira_viva_sessao teste_stream_canal_primeira_viva_sessao
'''
    if aliases not in text:
        assert text.count(marker) == 1, path
        text = text.replace(marker, aliases + marker)
    marker = '\nint main(int argc, char **argv) {'
    wrappers = '''
int teste_stream_primeira_boa_sessao(int n, uint64_t epoch) {
  assert(epoch == 0); return stream_primeira_boa(n);
}
int teste_stream_canal_primeira_viva_sessao(int n, uint64_t epoch) {
  assert(epoch == 0); return stream_canal_primeira_viva(n);
}
'''
    if wrappers not in text:
        assert text.count(marker) == 1, path
        text = text.replace(marker, wrappers + marker)
    path.write_text(text)

BUDGET_NEIGHBOURS = r'''// Only the original reduced fontesmortas_orcamento fixture links these
// omitted account neighbours. The next-VOD worker and budget remain real.
#include "debrid.h"
#include "stalker.h"
#include <assert.h>
#include <string.h>
static const _Thread_local DebridContexto *entered;
void debrid_contexto_capturar(DebridContexto *c) {
  assert(c); memset(c, 0, sizeof *c);
}
void debrid_contexto_entrar(const DebridContexto *c) {
  assert(c && c->sessao == 0 && !entered); entered = c;
}
void debrid_contexto_sair(void) { assert(entered); entered = 0; }
int stalker_resolver_sessao(const char *id, char *url, unsigned n, uint64_t epoch) {
  assert(epoch == 0); return stalker_resolver(id, url, n);
}
'''

def main():
    # Preserves original profile-selection transports and legacy VOD assertions.
    legacy.main()
    compatibility = shlex.quote(str(legacy_path.with_name('legacy-epoch.c')))
    neighbours = ROOT / 'native-playback-neighbours.c'
    neighbours.write_text(PLAYBACK_NEIGHBOURS)
    changed = []
    # These original upstream fixtures include a reduced addons/streams/app
    # module and model one session. They do not link the profile subsystem.
    # Ownership/race suites and all full-source builds still use real perfis.c.
    for name in ('addons_proveniencia', 'fontefalha_worker', 'fontefalha_playback'):
        path = Path('tests') / (name + '.sh')
        assert path.is_file(), ('Missing latest-upstream fixture', str(path))
        original = 'cc "${flags[@]}" tests/' + name + '.c'
        intermediate = 'cc "${flags[@]}" ' + compatibility + ' tests/' + name + '.c'
        extra = (' ' + shlex.quote(str(neighbours))) if name == 'fontefalha_playback' else ''
        replacement = 'cc "${flags[@]}" ' + compatibility + extra + ' tests/' + name + '.c'
        if replacement not in path.read_text() and intermediate in path.read_text():
            legacy.replace_once(path, intermediate, replacement)
        else:
            legacy.replace_once(path, original, replacement)
        changed.append(name)
    # The original app fixture supplies legacy fake selectors, but deliberately
    # omits credentials and epochs. Forward new scoped entry points to exactly
    # those existing mocks; never link this adapter into ownership fixtures.
    adapt_provenance_snapshot(Path('tests/addons_proveniencia.c'))
    changed.append('addons_proveniencia immutable private-worker input')
    changed.append('fontefalha_playback scoped selectors and absent credential context')
    budget = ROOT / 'native-budget-neighbours.c'
    budget.write_text(BUDGET_NEIGHBOURS)
    path = Path('tests/fontesmortas_orcamento.sh')
    old = 'cc "${flags[@]}" tests/fontesmortas_orcamento.c'
    new = 'cc "${flags[@]}" ' + compatibility + ' ' + shlex.quote(str(budget)) + ' tests/fontesmortas_orcamento.c'
    legacy.replace_once(path, old, new)
    adapt_budget_selectors(Path('tests/fontesmortas_orcamento.c'))
    changed.append('fontesmortas_orcamento original first/channel doubles plus absent account neighbours')
    print('Latest-upstream reduced fixture neighbours supplied: ' + ', '.join(changed))

if __name__ == '__main__':
    main()
