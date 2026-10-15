#include NV_STREAMS_SOURCE
#include <assert.h>
static int calls, nativeService, nativeResult = 1, negative, icons,budgetMode,budgetCalls;static int budgetService[8];static char budgetHash[8][80];
static uint64_t epoch = 44;
uint64_t perfis_sessao_geracao(void) { return epoch; }
int perfis_sessao_atual(uint64_t e) { return e == epoch; }
int perfis_sessao_travar(uint64_t e) { return e == epoch; }
void perfis_sessao_destravar(void) {}
int debrid_ativo(void) { return 1; }
int p2p_ativo(void) { return 0; }
int p2pmotor_e_url(const char *u) { (void)u; return 0; }
int p2p_ultimo_erro(void) { return 0; }
int p2p_resolver(const char *h, int f, const char *s, char *u, unsigned n) {
  (void)h; (void)f; (void)s; (void)u; (void)n; assert(!"P2P must not run"); return 0;
}
int debrid_servico_por_nome(const char *s) { (void)s; return nativeService; }
const char *debrid_servico_nome(int s) { (void)s; return "Native test provider"; }
int debrid_servico_disponivel(int s) { assert(s == nativeService); return 1; }
void debrid_contexto_entrar(const DebridContexto *c) { assert(c); }
void debrid_contexto_sair(void) {}
int debrid_resolver_cache(const char *h, int f, char *u, unsigned n, int *fora) {
  assert(h && *h && f == -1); calls++; *fora = negative;
  if (nativeResult == 1) snprintf(u, n, "https://native.invalid/%d.mp4", nativeService);
  return nativeResult == 1;
}
int debrid_resolver_servico_cache(int s, const char *h, int f, char *u, unsigned n, int *fora) {
  if(budgetMode){assert(budgetCalls<8);budgetService[budgetCalls]=s;snprintf(budgetHash[budgetCalls++],80,"%s",h);nativeService=s;nativeResult=s==2?1:0;}assert(s == nativeService); return debrid_resolver_cache(h, f, u, n, fora);
}
int debrid_resolver_escolhido(const char *h, int f, char *u, unsigned n, char *s, unsigned ns, int *pct) {
  assert(h && *h && f == -1); calls++; if (pct) *pct = 50;
  if (s && ns) snprintf(s, ns, "Native test provider");
  if (nativeResult == 1) snprintf(u, n, "https://native.invalid/%d.mp4", nativeService);
  return nativeResult;
}
int rede_url_final_tipo(const char *u, int s, const char *const *c, char *d, unsigned n, int *st, char *m, unsigned nm, long *b) {
  (void)u;(void)s;(void)c;(void)d;(void)n;(void)st;(void)m;(void)nm;(void)b; assert(!"No playback URL probe"); return 0;
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) {
  (void)u;(void)s;(void)c;(void)st; assert(!"No media download"); return NULL;
}
int vazao_url_aviso(const char *u) { (void)u; return 0; }
long fontesmortas_idade(const Stream *s) { (void)s; return -1; }
void gfx_icone(GfxRect r, const char *s, float a, float b, float c, float d) {
  (void)r;(void)a;(void)b;(void)c;(void)d; assert(!strcmp(s,"aj_zap")); icons++;
}
static void ready(const Stream *s, int expected) {
#ifdef NV_CACHE_FIX
  assert(s->emCache == expected);
#ifdef NV_CAPTION_NEGATIVE
assert(s->cacheNegado==!expected);
#endif
  int old = icons; assert(desenharCache(s, 0, 0, 1) == (expected && (!s->foraCache || s->cacheNativo) ? 30 : 0));
  assert(icons == old + (expected && (!s->foraCache || s->cacheNativo)));
#else
  (void)s;(void)expected;
#endif
}
static void reset(Stream *s) {
  memset(s, 0, sizeof *s); snprintf(s->infoHash, sizeof s->infoHash, "0123"); s->fileIdx = -1;
  nativeResult = 1; negative = 0;
#ifdef NV_CAPTION_NEGATIVE
s->cacheNegado=1;
#endif
}
int main(void) {
  Stream s; lista = &s; n = 1; listaGeracao = 7;
  for (nativeService = 0; nativeService < DEBRID_N; nativeService++) {
    reset(&s); int before = calls;
    Conferencia c = {.geracao = 7, .sessao = 44};
    assert(verificarUma(0, &c) == 1 && calls == before + 1); ready(&s, 1);
    // Native-confirmed readiness is newer than the retained operational cache hint.
    s.foraCache = 1; ready(&s, 1);
    reset(&s); negative = 1; nativeResult = 0;
    assert(verificarUma(0, &c) == 0 && s.foraCache); ready(&s, 0);
    reset(&s); nativeResult = 0;
    assert(verificarUma(0, &c) == 0 && !s.foraCache); ready(&s, 0);
    reset(&s); char url[4096], provider[80]; int pct;
    before = calls;
    assert(stream_resolver_escolhida(0, 7, url, sizeof url, provider, sizeof provider, &pct) == 1 && calls == before + 1);
    ready(&s, 1);
    reset(&s); nativeResult = DEBRID_BAIXANDO;
    assert(stream_resolver_escolhida(0, 7, url, sizeof url, provider, sizeof provider, &pct) == DEBRID_BAIXANDO);
    ready(&s, 0);
    reset(&s); before = calls;
    assert(stream_falha_aplicar_sessao(0, 7, nativeService, "https://native.invalid/ready.mp4", 44));
    assert(calls == before); ready(&s, 1);
    reset(&s);
    assert(!stream_falha_aplicar_sessao(0, 6, nativeService, "https://native.invalid/ready.mp4", 44)); ready(&s, 0);
    assert(!stream_falha_aplicar_sessao(0, 7, nativeService, "https://native.invalid/ready.mp4", 43)); ready(&s, 0);
    // Actual cache-only next-episode resolve path, with the real route policy.
    FonteFalhaConfig cfg = {.provedores = 1, .ordem = {0,1,2,3}, .limites = {1,1,1,1}};
    FonteFalhaCandidata candidate = {.indice = 0, .nativa = 1, .identidade = "0123|-1"};
    StreamPreparo p = {.fontes = &s, .candidatas = &candidate, .n = 1, .nc = 1, .sessaoPerfil = 44};
    p.politica = fontefalha_sessao_nova(&cfg, 1u << nativeService, -1); assert(p.politica);
    Stream resolved; before = calls;
    assert(stream_preparo_resolver(&p, NULL, NULL, &resolved, NULL, NULL) == 1 && calls == before + 1);
    ready(&resolved, 1); ready(&s, 1);
    before = calls;
    assert(stream_preparo_renovar(&p, 0, NULL, NULL, &resolved) && calls == before + 1); ready(&resolved, 1);
    fontefalha_sessao_liberar(p.politica);
  }
  // A supplied direct URL is not positive cache evidence.
  reset(&s); s.infoHash[0] = 0; int before = calls;
  assert(stream_falha_aplicar_sessao(0, 7, -1, "https://native.invalid/direct.mp4", 44));
  assert(calls == before); ready(&s, 0);
  lista = NULL; n = 0;
  for(int chip=0;chip<2;chip++){
    Stream candidates[4]={0};FonteFalhaCandidata routes[4]={0};char routeIds[4][80];
    for(int i=0;i<4;i++){snprintf(candidates[i].infoHash,sizeof candidates[i].infoHash,"budget-hash-%d",i);candidates[i].fileIdx=-1;candidates[i].filtroLido=1;routes[i].indice=i;routes[i].nativa=1;snprintf(routes[i].addon,sizeof routes[i].addon,"addon-%d",i%2);snprintf(routeIds[i],80,"budget-hash-%d|-1",i);routes[i].identidade=routeIds[i];
#ifdef NV_CAPTION_NEGATIVE
      candidates[i].cacheNegado=i%2;
#endif
    }
    soCache=chip;budgetMode=1;budgetCalls=0;negative=0;
    FonteFalhaConfig cfg={.provedores=1,.ordem={1,2,0,3},.limites={1,3,1,1}};
    StreamPreparo p={.fontes=candidates,.candidatas=routes,.n=4,.nc=4,.sessaoPerfil=44};
    p.politica=fontefalha_sessao_nova(&cfg,(1u<<1)|(1u<<2),-1);assert(p.politica);
    Stream resolved;assert(stream_preparo_resolver(&p,NULL,NULL,&resolved,NULL,NULL)==1);assert(budgetCalls==4);
    assert(budgetService[0]==1&&budgetService[1]==1&&budgetService[2]==1&&budgetService[3]==2);
    assert(strcmp(budgetHash[0],budgetHash[1])&&strcmp(budgetHash[0],budgetHash[2])&&strcmp(budgetHash[1],budgetHash[2]));
    assert(resolved.emCache&&!resolved.foraCache);
#ifdef NV_CAPTION_NEGATIVE
    assert(!resolved.cacheNegado);
#endif
    printf("budget chip%d TorBox=3 distinct links across addons then Premiumize=1; display state does not consume attempts\n",chip);
    fontefalha_sessao_liberar(p.politica);budgetMode=0;
  }

  printf("cache_native: four providers; existing resolve/prepare/manual/failover; stale ownership; no additional probes (%d existing resolver calls)\n", calls);
}
