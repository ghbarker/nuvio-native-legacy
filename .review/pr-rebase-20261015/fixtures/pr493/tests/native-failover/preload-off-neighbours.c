// Legacy player/failover fixtures predate opt-in preload and have no title page.
// Supply missing disabled-feature neighbours only. Strong existing fixture
// mocks always win. The real preload controller has separate ownership tests.
#include "ajustes.h"
#include "detail.h"
#include "fonteregra.h"
#include "streams.h"
#include "debrid.h"
#include <assert.h>
#define W __attribute__((weak))
#define OFF(name) W int name(void) { return 0; }
OFF(ajustes_fonte_preparar)
OFF(ajustes_fonte_precarregar)
OFF(ajustes_fonte_manual)
OFF(ajustes_fonte_primeira)
OFF(ajustes_fonte_escopo)
OFF(ajustes_fonte_regex_modo)
OFF(ajustes_fonte_usar_outros)
OFF(ajustes_fonte_ordem_uso)
OFF(ajustes_fonte_prioridade)
OFF(ajustes_fonte_hdr)
OFF(ajustes_dolby_vision)
OFF(ajustes_dv_mkv)
OFF(ajustes_dolby_atmos)
OFF(ajustes_tamanho_min_gb)
OFF(ajustes_tamanho_max_gb)
OFF(ajustes_buffer_preset)
OFF(ajustes_cache_seek_mb)
OFF(ajustes_fonte_conferir_varias)
OFF(ajustes_fonte_tocar_conferindo)
W const char *ajustes_qualidade(void) { return "Automática"; }
W unsigned fonteregra_versao(void) { return 0; }
W int perfis_ativo(void) { return 1; }
W const char *sessao_usuario(void) { return ""; }
W int detail_indice(void) { return -1; }
W int detail_aberto(void) { return 0; }
W int detail_ep_foco(int *t, int *e) { if (t) *t = 0; if (e) *e = 0; return 0; }
W int stream_lista_do_alvo(const char *id) { (void)id; return 0; }
W int debrid_servico_disponivel(int servico) { (void)servico; return 0; }
W int stream_falha_renovar(int i, unsigned g, int s, int (*c)(void *), void *ctx,
                          char *url, unsigned n) {
  (void)i; (void)g; (void)s; (void)c; (void)ctx; (void)url; (void)n;
  assert(!"Disabled preload must not renew a prepared route"); return 0;
}
W int debrid_resolver_servico_cache(int s, const char *hash, int file,
                                  char *url, unsigned n, int *fora) {
  (void)s; (void)hash; (void)file; (void)url; (void)n; (void)fora;
  assert(!"Legacy budget has no native provider route"); return 0;
}
