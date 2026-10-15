#define main prior_worker_main
#include "reviewed-fontefalha_worker.c"
#undef main
#include <limits.h>
int ajustes_fonte_preparar(void) { return 1; }
int ajustes_fonte_precarregar(void) { return 1; }
int ajustes_buffer_preset(void) { return 0; }
int ajustes_cache_seek_mb(void) { return 0; }
int ajustes_fonte_conferir_varias(void) { return 0; }
int ajustes_fonte_tocar_conferindo(void) { return 0; }
void debrid_contexto_capturar(DebridContexto *c) { memset(c, 0, sizeof *c); }
void debrid_contexto_entrar(const DebridContexto *c) { assert(c); }
void debrid_contexto_sair(void) {}
int fontepref_escolher_lista(const char *alvo, const Stream *l, int n, int (*f)(int,void*), void *u) {
  (void)alvo;(void)l;(void)n;(void)f;(void)u; return -1;
}
int main(int argc, char **argv) {
  assert(argc == 2); fontefalha_dir(argv[1]); resetar();
  assert(fontefalha_definir_provedores(1)); assert(fontefalha_definir_addons(1));
  assert(fontefalha_provedor_mover(DEBRID_TB, -1));
  assert(fontefalha_provedor_mover(DEBRID_PM, -1));
  for (int i = 0; i < 4; i++) assert(fontefalha_provedor_definir_limite(i, 3));
  FonteFalhaConfig c = cfg(1,1,3);
  const char *addons[] = {c.addon[0].chave, c.addon[1].chave}; fontefalha_addons_registrar(addons, 2);
  for (int i = 0; i < 2; i++) assert(fontefalha_addon_definir_limite(addons[i], 3));
  FonteFiltroConfig filtro = {0}; filtro.excluir[FONTEFILTRO_RESOLUCAO] = fontefiltro_valores(FONTEFILTRO_RESOLUCAO, "2160p");
  assert(fontefalha_filtro_definir(&filtro));
  Stream l[] = {fonte(&c,0,"filtered-a",0), fonte(&c,0,"eligible-a",0),
                fonte(&c,1,"filtered-b",0), fonte(&c,1,"eligible-b",0), fonte(&c,0,"eligible-c",0)};
  l[0].altura = l[2].altura = 2160;
  disponiveis = (1u << DEBRID_TB) | (1u << DEBRID_PM);
  StreamPreparo *p = stream_preparo_criar(l, 5, 0); assert(p && p->nc == 3);
  for (int i = 0; i < p->nc; i++) assert(p->candidatas[i].filtro.valores[FONTEFILTRO_RESOLUCAO]);
  Stream out; int idx;
  for (int i = 0; i < 4; i++) {
    assert(stream_preparo_resolver(p, NULL, NULL, &out, &idx, NULL) == 1);
    assert(nPedidos == i + 1 && pedidos[i].servico == (i < 3 ? DEBRID_TB : DEBRID_PM));
    assert(strncmp(pedidos[i].hash, "eligible-", 9) == 0);
  }
  assert(fontefalha_tentativas(p->politica) == 4 && nHttp == 0);
  int before = nPedidos;
  assert(!stream_preparo_renovar(p, 0, NULL, NULL, &out) && nPedidos == before);
  // New preferences invalidate adoption but never mutate an existing worker's copy.
  filtro.excluir[FONTEFILTRO_RESOLUCAO] |= fontefiltro_valores(FONTEFILTRO_RESOLUCAO,"1080p");
  assert(fontefalha_filtro_definir(&filtro)); assert(!stream_preparo_contexto_atual(p));
  assert(stream_preparo_renovar(p, 1, NULL, NULL, &out) && nPedidos == before + 1);
  stream_preparo_liberar(p);
  p = stream_preparo_criar(l, 5, 0); assert(p && p->nc == 0);
  before = nPedidos; assert(!stream_preparo_resolver(p,NULL,NULL,&out,NULL,NULL) && nPedidos == before);
  stream_preparo_liberar(p);
  // Synthetic large sizes cannot wrap below the selected maximum.
  Stream huge = {.tamanhoMB = LONG_MAX}; FonteFiltroDados dados;
  stream_filtro_dados(&huge, &dados); assert(dados.tamanhoBytes == UINT64_MAX);
  stream_definir_lista(NULL,0);
  puts("preparation_filters: three eligible TorBox links across addons, then Premiumize; zero calls/budget for exclusions; renew snapshots and adoption invalidation PASS");
}
