// Exercise the production app worker mailbox and playback guards. Only player,
// network and UI boundaries are mocked; the failover session is the real policy.
#define _GNU_SOURCE
#define SDL_MAIN_HANDLED
#include <pthread.h>
static int fake_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
static int fake_join(pthread_t, void **);
#define pthread_create fake_create
#define pthread_join fake_join
int ajustes_fonte_precarregar(void);
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
#include "session-stubs.h"
#endif
#include SOURCE_APP
#undef pthread_create
#undef pthread_join
#include <assert.h>

static Uint32 agora;
static unsigned lista = 41;
static Stream fontes[4];
static CatItem catalogItem;
static unsigned listAge, ruleVersion = 1;
static int settingsNative = 1, activeVideo, nativeQueueResult=1, actorDuringNativeCreate;
static int detailT, detailE, playerT, playerE;
static int settingsPrepare = 1, settingsManual, openDetail = 1, remembered = -1;
static int profile = 1, nativeState = 1, nativeCancel, nativeAdopt, nativeStarts, listRenew;
static int requests, renewals, settingOrder, sourceAvailable = 1;
static char account[64] = "account-a", stampedTarget[64] = "tt-controller";
static char lastPreparedUrl[4096];
static unsigned lastToken;
static int aberto, mini, sair, carregando, pronto, falhou, playerFalhou;
static int reconectando, tocou, dv, bufferMs, proximaSemPerda;
static double posicao, bufferFim;
static int paradas, retomadas, aberturas, aplicacoes, legadas, mortas, irmas;
static int tentativaN, tentativaT, evento, eventoRetomada, eventoParada;
static double retomada;
static void *(*worker)(void *);
static void *workerArg;
static DebridContexto credentialContext;
static const DebridContexto *boundContext;
static int contextEntrances, contextExits, expectedKey;
static int transitionEvent, adoptionEvent, loadingToasts, listCopyFails, streamsCount=4, sourceErrors;
static int workerEsperar, workerEscolhida;
static int actorAfterConsume, actorDuringClose, actorDuringCapture, actorDuringTarget, actorDuringOpen;

static int fake_create(pthread_t *t, const pthread_attr_t *attr,
                       void *(*fn)(void *), void *arg) {
  (void)attr; assert(!worker); *t = (pthread_t)1; worker = fn; workerArg = arg; return 0;
}
static int fake_join(pthread_t t, void **ret) {
  (void)t; (void)ret; assert(!worker && "UI only joins after worker completion"); return 0;
}
static void concluirWorker(void) {
  assert(worker); void *(*fn)(void *) = worker; void *arg = workerArg;
  worker = NULL; workerArg = NULL; fn(arg);
}

Uint32 SDL_GetTicks(void) { return agora; }
int player_aberto(void) { return aberto; }
int player_mini_ativo(void) { return mini; }
int player_quer_sair(void) { return sair; }
const char *player_id_canal(void) { return ""; }
int player_carregando(void) { return carregando; }
int player_fonte_falhou(void) { return playerFalhou; }
void player_voltar_a_esperar(void) {
  paradas++; eventoParada = ++evento; posicao = 0.0; pronto = 0; carregando = 1;
}
void player_definir_retomada_fonte(double p) {
  retomadas++; retomada = p; eventoRetomada = ++evento;
}
void player_definir_fonte(const char *url) { (void)url; aberturas++; }
void player_definir_tentativa(int n, int t) { tentativaN = n; tentativaT = t; }
void player_toast(const char *txt, unsigned ms) { assert(!strcmp(txt,"Abrindo fonte") && ms == 1500); loadingToasts++; }
void player_erro_fonte(void) { sourceErrors++; }
void player_erro_fonte_motivo(const char *titulo, const char *subtitulo) { (void)titulo; (void)subtitulo; }
void player_fechar_mini(void) { mini = 0; }
int video_falhou(void) { return falhou; }
int video_reconectando(void) { return reconectando; }
int video_pronto(void) { return pronto; }
int video_fonte_tocou(void) { return tocou; }
int video_dv_ativo(void) { return dv; }
unsigned video_bufferando_ms(void) { return (unsigned)bufferMs; }
double video_pos(void) { return posicao; }
double video_buffer_fim(void) { return bufferFim; }
const char *video_url_atual(void) { return ""; }
const char *video_erro_texto(void) { return "fixture playback error"; }
int stream_atual(void) { return 0; }
unsigned stream_lista_geracao(void) { return lista; }
int stream_n(void) { return streamsCount; }
const Stream *stream_item(int i) { return i >= 0 && i < streamsCount ? &fontes[i] : NULL; }
int stream_proxima_sem_perda(int i) { (void)i; return proximaSemPerda; }
void stream_vod_falhou(int i) { (void)i; mortas++; }
int stream_automatico_excluir(int i) { (void)i; mortas++; return 1; }
int stream_automatico_excluir_irmas(int i) { (void)i; irmas++; return 1; }
int stream_proxima_vod(int i) { (void)i; legadas++; return 1; }
int ajustes_fonte_repor(void) { return 0; }
int debrid_servico_disponivel(int servico) { return servico == DEBRID_TB || servico == DEBRID_PM; }
const char *i18n(const char *s) { return s; }
void marco(const char *s) { (void)s; }
uint64_t badges_bit(const char *s) { (void)s; return 0; }
int player_indice(void) { return 0; }
void player_episodio_atual(int *t, int *e) { *t = playerT; *e = playerE; }
const CatItem *cat_item(int i) { (void)i; return &catalogItem; }
int cat_id_stream(int i, int t, int e, char *url, unsigned n) {
  (void)i; snprintf(url, n, "%s:%d:%d", catalogItem.imdb, t, e);
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  if(actorDuringTarget) { actorDuringTarget=0; atomic_fetch_add(&fixtureSession,2); }
#endif
  return 1;
}
void stream_definir_lista(const Stream *s, int n) { assert(!s && !n); streamsCount=0; lista++; memset(fontes,0,sizeof fontes); }
void stream_definir_atual(int i) { (void)i; assert(!"VOD consumer opens in the later UI phase"); }
int stream_automatico_disponivel(int i) { return i >= 0 && i < 4 && sourceAvailable; }
Uint32 stream_idade_ms(void) { return listAge; }
int stalker_resolver(const char *id, char *url, unsigned n) {
  (void)id; (void)url; (void)n; assert(!"VOD policy must not dispatch Stalker"); return 0;
}
int stream_canal_primeira_viva(int n) { (void)n; assert(!"VOD policy must not dispatch channel"); return -1; }
int stream_canal_prazo_longo(int i) { (void)i; return 0; }
int stream_automatico_canal(void) { assert(!"VOD policy must not dispatch channel"); return -1; }
int stream_primeira_boa(int n) { assert(n == 8); legadas++; return 0; }
int stream_verificar_proxima_vod(int i, int n, int *conferidas) {
  (void)i; (void)n; (void)conferidas; legadas++; assert(!"VOD policy must bypass legacy fallback"); return -1;
}
int rede_url_final_tipo(const char *url, int seg, const char *const *cab,
                        char *fim, unsigned tam, int *status,
                        char *mime, unsigned mimeTam, long *corpo) {
  (void)url; (void)seg; (void)cab; (void)fim; (void)tam; (void)status;
  (void)mime; (void)mimeTam; (void)corpo; assert(!"fixture error is not a connection-probe error"); return 0;
}

// Simulated network worker asks the real policy for its next route, consumes a
// distinct link, and publishes only its private cloned session.
int stream_falha_verificar(FonteFalhaSessao *s, unsigned ger, int escolhida,
                          int (*cancelado)(void *), void *ctx,
                          int *idx, int *serv, char *url, unsigned n) {
  assert(boundContext && boundContext->chave[DEBRID_TB][0] == expectedKey);
  workerEscolhida = escolhida; *idx = *serv = -1; url[0] = 0;
  if (cancelado(ctx) || ger != lista) return FONTEFALHA_FIM;
  if (workerEsperar) return FONTEFALHA_ESPERAR;
  FonteFalhaCandidata c[4] = {{0}};
  for (int i = 0; i < 4; i++) {
    c[i].indice = i; c[i].nativa = 1; c[i].ordemAddon = i / 2;
    strcpy(c[i].addon, fontes[i].addonChave); c[i].identidade = fontes[i].infoHash;
  }
  FonteFalhaRota r;
  int estado = fontefalha_proxima(s, c, 4, 1, &r);
  if (estado != FONTEFALHA_PROXIMA) return estado;
  requests++; assert(fontefalha_tentar(s, &c[r.candidata], r.servico));
  snprintf(url, n, "https://resolved.invalid/%d/%d.mp4", r.servico, r.indice);
  assert(fontefalha_url_registrar(s, url)); *idx = r.indice; *serv = r.servico;
  return estado;
}
int stream_falha_aplicar(int idx, unsigned ger, int serv, const char *url) {
  assert(ger == lista); assert(idx >= 0 && idx < 4); assert(url && *url);
  aplicacoes++; memmove(fontes[idx].url, url, strlen(url) + 1);
  fontes[idx].debridServico = serv + 1; return 1;
}
void stream_falha_identidade(const Stream *s, char *dst, unsigned tam) {
  snprintf(dst, tam, "%s", s->infoHash);
}


void debrid_definir_episodio(int t,int e) { credentialContext.temporada=t; credentialContext.episodio=e; }
void debrid_contexto_capturar(DebridContexto *ctx) {
  *ctx = credentialContext;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  if(actorDuringCapture) { assert(!fixtureGateDepth); actorDuringCapture=0; atomic_fetch_add(&fixtureSession,2); }
#endif
}
void debrid_contexto_entrar(const DebridContexto *ctx) { assert(!boundContext); boundContext = ctx; contextEntrances++; }
void debrid_contexto_sair(void) { assert(boundContext); boundContext = NULL; contextExits++; }
int perfis_ativo(void) { return profile; }
const char *sessao_usuario(void) { return account; }
unsigned fonteregra_versao(void) { return ruleVersion; }
int ajustes_fonte_preparar(void) { return settingsPrepare; }
int ajustes_fonte_precarregar(void) { return settingsNative; }
int ajustes_fonte_manual(void) { return settingsManual; }
int ajustes_fonte_primeira(void) { return 0; }
int ajustes_fonte_escopo(void) { return 0; }
int ajustes_fonte_regex_modo(void) { return 0; }
int ajustes_fonte_usar_outros(void) { return 0; }
int ajustes_fonte_ordem_uso(void) { return settingOrder; }
int ajustes_fonte_prioridade(void) { return 1; }
int ajustes_fonte_hdr(void) { return 1; }
int ajustes_dolby_vision(void) { return 0; }
int ajustes_dv_mkv(void) { return 0; }
int ajustes_dolby_atmos(void) { return 0; }
int ajustes_tamanho_min_gb(void) { return 0; }
int ajustes_tamanho_max_gb(void) { return 0; }
int ajustes_buffer_preset(void) { return 0; }
int ajustes_cache_seek_mb(void) { return 0; }
int ajustes_fonte_conferir_varias(void) { return 0; }
int ajustes_fonte_tocar_conferindo(void) { return 0; }
const char *ajustes_qualidade(void) { return "Automatica"; }
int player_retido(void) { return 0; }
int video_ativo(void) { return activeVideo; }
int detail_aberto(void) { return openDetail; }
int detail_indice(void) { return 0; }
int detail_ep_foco(int *t, int *e) { *t = detailT; *e = detailE; return detailT > 0 && detailE > 0; }
AddEstado addons_estado(void) { return ADD_PRONTO; }
int stream_n_candidatas(void) { return 4; }
int stream_lista_do_alvo(const char *alvo) { return !strcmp(alvo, stampedTarget); }
void stream_definir_alvo(const char *alvo) { snprintf(stampedTarget, sizeof stampedTarget, "%s", alvo); }
void addons_buscar_renovar(const char *id, const char *tipo) { (void)id; (void)tipo; listRenew++; }
int fontepref_escolher(const char *base) { (void)base; return remembered; }
void fontepref_id_base(const char *alvo, char *dst, unsigned n) { snprintf(dst, n, "%s", alvo); }
void stream_preferir(int idx) { (void)idx; }
unsigned player_precarregar_novo_token(void) { return ++lastToken; }
int player_precarregar_fonte(int cat, int t, int e, int idx, unsigned token) {
  assert(cat == 0 && t == detailT && e == detailE); assert(idx >= 0 && idx < 4);
  assert(!activeVideo&&!aberto&&!mini); nativeStarts++; lastToken = token;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  if(actorDuringNativeCreate) { assert(fixtureGateDepth>0); actorDuringNativeCreate=0; fixtureActorAfterUnlock=1; }
#endif
  snprintf(lastPreparedUrl, sizeof lastPreparedUrl, "%s", fontes[idx].url); return nativeQueueResult;
}
int player_precarregar_estado(unsigned token) { assert(token == lastToken); return nativeState; }
void player_precarregar_cancelar(unsigned token) { assert(!token || token == lastToken); nativeCancel++; }
void player_precarregar_adotar(unsigned token) {
  assert(token == lastToken);
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  assert(fixtureGateDepth>0); // both title and next adoption are actor-gated
#endif
  nativeAdopt++; adoptionEvent=++transitionEvent;
}
int stream_falha_renovar(int idx, unsigned gen, int service, int (*cancelled)(void *), void *ctx,
                         char *url, unsigned n) {
  assert(gen == lista && idx >= 0 && idx < 4); renewals++;
  if (cancelled(ctx)) return 0;
  snprintf(url, n, "https://renewed.invalid/%d/%d.mp4", service, idx); return 1;
}

static void reset(int providers, int addons) {
  assert(!worker && !fioFonteVivo);
  actorAfterConsume=actorDuringClose=actorDuringCapture=actorDuringTarget=actorDuringOpen=0;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  atomic_store(&fixtureSession,1); fixtureActorAfterUnlock=0; assert(!fixtureGateDepth);
#endif
  listCopyFails=0; streamsCount=4; sourceErrors=0; activeVideo=0; nativeQueueResult=1; actorDuringNativeCreate=0;
  preparoInvalidar(); limparFonteVOD(); limparFontePendente();
  memset(&fonteJob, 0, sizeof fonteJob);
  assert(fontefalha_definir_provedores(providers)); assert(fontefalha_definir_addons(addons));
  fonteEscolhida = -2; fontePedidoGeracao = novaGeracaoFonte(); aguardandoFonte = 0;
  aberto = mini = sair = carregando = falhou = playerFalhou = 0;
  agora = 1001; lista++; listAge = 0; ruleVersion = 1;
  settingsPrepare = settingsNative = openDetail = sourceAvailable = 1; detailT = detailE = playerT = playerE = 0; settingsManual = settingOrder = 0;
  profile = 1; strcpy(account, "account-a"); nativeState = 1;
  memset(&catalogItem, 0, sizeof catalogItem);
  strcpy(catalogItem.imdb, "tt-controller"); strcpy(catalogItem.tipo, "movie");
  strcpy(stampedTarget, catalogItem.imdb); remembered = -1;
  memset(fontes, 0, sizeof fontes);
  char keys[2][FONTEFALHA_CHAVE];
  fontefalha_addon_chave("https://addon-a.invalid/manifest.json", keys[0], sizeof keys[0]);
  fontefalha_addon_chave("https://addon-b.invalid/manifest.json", keys[1], sizeof keys[1]);
  const char *kp[] = {keys[0],keys[1]}; fontefalha_addons_registrar(kp,2);
  for (int i = 0; i < 4; i++) {
    fontes[i].fileIdx = 0; fontes[i].addonOrdem = i / 2;
    strcpy(fontes[i].addonChave, keys[i / 2]);
    snprintf(fontes[i].infoHash, sizeof fontes[i].infoHash, "hash-%d:0", i);
    snprintf(fontes[i].url, sizeof fontes[i].url, "https://original.invalid/%d.mp4", i);
    snprintf(fontes[i].rotulo, sizeof fontes[i].rotulo, "Source %d", i);
  }
  memset(&credentialContext, 0, sizeof credentialContext);
  credentialContext.sessao = 1; credentialContext.chave[DEBRID_TB][0] = expectedKey = 1;
  contextEntrances = contextExits = 0;
  requests = renewals = legadas = aplicacoes = listRenew = nativeStarts = nativeCancel = nativeAdopt = loadingToasts = 0;
}
static void prepared(void) {
  prepararAoAbrir(); assert(worker && preparoRodando);
  concluirWorker(); processarFonteJob(); assert(preparo.tem && !worker && !fioFonteVivo);
}
static ProximoPreparoResultado nextPayload;
static int nextReady, nextBusy, closeEvent, closes, opens, coldRequests;
static int savedT, savedE;
static unsigned nextEpoch=1;
static char nextTarget[64];
#ifndef JOINED_APP_NEXT
unsigned proximopreparo_geracao(void) { return nextEpoch; }
void proximopreparo_passo(const char *current) { (void)current; }
void proximopreparo_cancelar(void) { nextEpoch++; nextBusy=0; }
int proximopreparo_em_curso(const char *target) { return nextBusy && !strcmp(target,nextTarget); }
int proximopreparo_consumir(const char *target, ProximoPreparoResultado *out) {
  memset(out,0,sizeof *out);
  if(!nextReady || strcmp(target,nextTarget)) return 0;
  *out=nextPayload; memset(&nextPayload,0,sizeof nextPayload); nextReady=0;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  if(actorAfterConsume) atomic_fetch_add(&fixtureSession,2);
#endif
  return 1;
}
void proximopreparo_liberar_resultado(ProximoPreparoResultado *r) {
  if(r->token) player_precarregar_cancelar(r->token); free(r->fontes); fontefalha_sessao_liberar(r->politica); memset(r,0,sizeof *r);
}
#endif
void diagnostico_cancelar_vazao(void) {}
void player_encerrar(void) {
  closes++; savedT=playerT; savedE=playerE; closeEvent=++transitionEvent; aberto=0;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  if(actorDuringClose) atomic_fetch_add(&fixtureSession,2);
#endif
#ifdef JOINED_APP_NEXT
  proximopreparo_cancelar();
#else
  nextEpoch++;
#endif
}
void player_abrir(int i,const char *url) {
  assert(i==0&&!url); opens++; aberto=1;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  if(actorDuringOpen) { actorDuringOpen=0; atomic_fetch_add(&fixtureSession,2); }
#endif
}
void player_definir_episodio(int t,int e) { playerT=t; playerE=e; }
void cat_apontar_episodio(int i,int t,int e) { assert(i==0&&playerT==t&&playerE==e); catalogItem.temporada=t; catalogItem.episodio=e; }
void stream_definir_lista_idade(const Stream *list,int count,Uint32 age) {
  assert(count==4); if(listCopyFails) return; memcpy(fontes,list,4*sizeof *fontes); listAge=age; lista++;
}
void addons_buscar(const char *target,const char *type) { assert(!strcmp(target,nextTarget)&&!strcmp(type,"series")); coldRequests++; }
void addons_buscar_legendas(const char *target,const char *type) { assert(!strcmp(target,nextTarget)&&!strcmp(type,"series")); }
static void nextReset(void) {
  reset(1,1); strcpy(catalogItem.tipo,"series"); playerT=1; playerE=1; aberto=1;
  catalogItem.temporada=1; catalogItem.episodio=1; catalogItem.progresso=85;
  strcpy(stampedTarget,"tt-controller:1:1"); strcpy(nextTarget,"tt-controller:1:2");
  memset(&proximoIntento,0,sizeof proximoIntento); memset(&nextPayload,0,sizeof nextPayload);
  nextEpoch=1; nextReady=nextBusy=transitionEvent=adoptionEvent=closeEvent=closes=opens=coldRequests=0;
}
static void makeNextReady(void) {
  nextPayload.fontes=malloc(4*sizeof *fontes); assert(nextPayload.fontes);
  memcpy(nextPayload.fontes,fontes,4*sizeof *fontes);
  strcpy(nextPayload.fontes[2].url,"https://next.invalid/source.mp4");
  nextPayload.n=4; nextPayload.indiceFonte=2; nextPayload.catalogo=0; nextPayload.temporada=1; nextPayload.episodio=2;
  nextPayload.token=player_precarregar_novo_token(); nextPayload.idadeMs=1000;
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  nextPayload.sessaoPerfil=perfis_sessao_geracao();
#endif
  FonteFalhaConfig cfg; fontefalha_config(&cfg); nextPayload.politica=fontefalha_sessao_nova(&cfg,6,-1); assert(nextPayload.politica);
  FonteFalhaCandidata c={.indice=2,.nativa=1,.identidade=fontes[2].infoHash,.ordemAddon=1}; strcpy(c.addon,fontes[2].addonChave);
  assert(fontefalha_tentar(nextPayload.politica,&c,DEBRID_TB));
  assert(fontefalha_url_registrar(nextPayload.politica,nextPayload.fontes[2].url)); nextReady=1; nativeState=2;
}
static void nextAppTransitionAndCancellation(void) {
  nextReset(); makeNextReady(); abrirProximoEpisodio(1,2,1);
  assert(closes==1&&opens==1&&savedT==1&&savedE==1&&playerT==1&&playerE==2);
  assert(catalogItem.temporada==1&&catalogItem.episodio==2&&adoptionEvent<closeEvent&&nativeAdopt==1);
  assert(fonteFalha&&fontefalha_tentativas(fonteFalha)==1&&fonteFalhaLista==lista);
  assert(credentialContext.temporada==1&&credentialContext.episodio==2);
  assert(fonteEscolhida==2&&aguardandoFonte==2&&!strcmp(fontes[2].url,"https://next.invalid/source.mp4")&&!coldRequests);
  nextReset(); makeNextReady(); nextPayload.indiceFonte=-2; nextPayload.token=0;
  abrirProximoEpisodio(1,2,1); assert(worker&&fonteJob.tipo==FJOB_FALHA&&fontefalha_tentativas(fonteFalha)==1);
  concluirWorker(); processarFonteJob(); assert(fonteEscolhida!=2&&fonteEscolhida>=0&&fontefalha_tentativas(fonteFalha)==2);
  nextReset(); makeNextReady(); listCopyFails=1; unsigned oldList=lista;
  abrirProximoEpisodio(1,2,1);
  assert(lista==oldList+1&&!coldRequests&&!aguardandoFonte&&!fonteFalha&&!worker&&nativeCancel==1);
  assert(!streamsCount&&sourceErrors==1&&fonteEscolhida==-1&&!requests);
  nextReset(); nextBusy=1; abrirProximoEpisodio(1,2,1);
  assert(proximoIntento.tem&&!closes&&!opens&&loadingToasts==1);
  abrirProximoEpisodio(1,2,1); assert(loadingToasts==1);
  proximoPreparoPasso(); assert(!closes&&loadingToasts==1);
  nextBusy=0; makeNextReady(); proximoPreparoPasso(); assert(closes==1&&opens==1&&!proximoIntento.tem&&!coldRequests);
  nextReset(); nextBusy=1; abrirProximoEpisodio(1,2,1); assert(proximoIntento.tem);
  nextEpoch++; nextBusy=0; proximoPreparoPasso(); assert(!proximoIntento.tem&&!closes&&!opens&&!coldRequests&&playerE==1);
  nextReset(); abrirProximoEpisodio(1,2,0); assert(closes==1&&opens==1&&coldRequests==1&&aguardandoFonte==1&&!fonteFalha);
#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
  nextReset(); actorDuringTarget=1; abrirProximoEpisodio(1,2,0);
  assert(!closes&&!opens&&!coldRequests&&!nativeAdopt&&!fonteFalha);
  nextReset(); makeNextReady(); actorAfterConsume=1; abrirProximoEpisodio(1,2,1);
  assert(!closes&&!opens&&!coldRequests&&!nativeAdopt&&nativeCancel==1&&!fonteFalha);
  assert(!nextReady&&!proximoIntento.tem&&!requests&&playerE==1);
  nextReset(); makeNextReady(); actorDuringClose=1; abrirProximoEpisodio(1,2,1);
  assert(closes==1&&!opens&&!coldRequests&&nativeAdopt==1&&nativeCancel==1&&!fonteFalha);
  assert(!requests&&playerE==1);
  nextReset(); makeNextReady(); actorDuringOpen=1; unsigned unchangedList=lista;
  abrirProximoEpisodio(1,2,1);
  assert(closes==2&&opens==1&&!aberto&&lista==unchangedList&&!coldRequests);
  assert(nativeAdopt==1&&nativeCancel==1&&!fonteFalha&&!requests&&fonteEscolhida==-2);
  nextReset(); actorDuringClose=1; abrirProximoEpisodio(1,2,0);
  assert(closes==1&&!opens&&!coldRequests&&!fonteFalha);
  nextReset(); nextBusy=1; abrirProximoEpisodio(1,2,1); nextBusy=0;
  atomic_fetch_add(&fixtureSession,2); proximoPreparoPasso();
  assert(!proximoIntento.tem&&!closes&&!opens&&!coldRequests&&playerE==1);
#endif
  puts("PASS app Next adopts before close, transfers spent policy, reuses pending worker, and rejects actor changes after consume/during close/pending cold fallback");
}
static void distinctSettingsAndEpisodes(void) {
  for (int source = 0; source <= 1; source++) for (int native = 0; native <= 1; native++) {
    reset(1,1); settingsPrepare = source; settingsNative = native;
    prepararAoAbrir();
    if (!(source || native)) { assert(!worker && !preparoRodando && !nativeStarts && !requests); continue; }
    assert(worker); concluirWorker(); processarFonteJob(); assert(preparo.tem && requests == 1);
    assert(nativeStarts == native);
    int idx; assert(preparoUsar(catalogItem.imdb,-1,&idx) && idx == 0);
    assert(nativeAdopt == native && !nativeCancel);
  }
  reset(1,1); strcpy(catalogItem.tipo,"series"); detailT = 2; detailE = 7;
  credentialContext.temporada = detailT; credentialContext.episodio = detailE;
  cat_id_stream(0,detailT,detailE,stampedTarget,sizeof stampedTarget);
  prepared(); assert(!strcmp(preparo.alvo,"tt-controller:2:7"));
  nativeState = 2; assert(nativeStarts == 1 && lastPreparedUrl[0]);
  detailE = 8; preparoVigiar(); assert(!preparo.tem && nativeCancel == 1);
  cat_id_stream(0,detailT,detailE,stampedTarget,sizeof stampedTarget); lista++;
  credentialContext.episodio = detailE;
  prepared(); int idx; assert(preparoUsar("tt-controller:2:8",-1,&idx));
  assert(idx == 0 && nativeAdopt == 1 && nativeStarts == 2);
  puts("PASS separate source/native settings matrix and exact episode target changes");
}
static void lateIdleNativeRetry(void) {
  reset(1,1); activeVideo=1; prepared();
  FonteFalhaSessao *same=preparo.falha; unsigned sameList=lista; Uint32 since=preparo.quando;
  assert(same&&fontefalha_tentativas(same)==1&&!nativeStarts&&!preparo.token&&!preparo.bufferTentou);
  for(int i=0;i<100;i++) prepararAoAbrir();
  assert(activeVideo==1&&!nativeStarts&&requests==1&&!worker&&preparo.falha==same);
  activeVideo=0; prepararAoAbrir();
  assert(nativeStarts==1&&preparo.token&&preparo.bufferTentou&&requests==1&&!worker);
  assert(preparo.falha==same&&fontefalha_tentativas(same)==1&&lista==sameList&&preparo.quando==since);
  assert(!strcmp(lastPreparedUrl,fontes[preparo.idx].url));
  for(int i=0;i<100;i++) prepararAoAbrir();
  assert(nativeStarts==1&&requests==1&&!worker&&fontefalha_tentativas(same)==1);
  int idx=-1; assert(preparoUsar(catalogItem.imdb,-1,&idx)&&idx==0&&nativeAdopt==1);
  reset(1,1); activeVideo=1; prepared(); nativeQueueResult=0; activeVideo=0;
  prepararAoAbrir(); for(int i=0;i<100;i++) prepararAoAbrir();
  assert(nativeStarts==1&&!preparo.token&&preparo.bufferTentou&&requests==1&&!worker);
  reset(1,1); strcpy(catalogItem.tipo,"series"); detailT=2;detailE=7;
  credentialContext.temporada=2; credentialContext.episodio=7;
  cat_id_stream(0,detailT,detailE,stampedTarget,sizeof stampedTarget);
  activeVideo=1; prepared(); activeVideo=0; prepararAoAbrir();
  assert(nativeStarts==1&&preparo.temporada==2&&preparo.episodio==7&&requests==1);
  for(int change=0;change<5;change++) {
    reset(1,1);activeVideo=1;prepared();activeVideo=0;
    if(change==0) { atomic_fetch_add(&fixtureSession,2); }
    else if(change==1) lista++;
    else if(change==2) settingsNative=0;
    else if(change==3) openDetail=0;
    else strcpy(catalogItem.imdb,"tt-new-title");
    preparoVigiar();preparoBufferTentar();
    assert(!preparo.tem&&!nativeStarts&&requests==1&&!worker&&!fonteFalha);
  }
  reset(1,1);activeVideo=1;prepared();activeVideo=0;actorDuringNativeCreate=1;
  prepararAoAbrir();assert(nativeStarts==1&&nativeCancel==1&&!preparo.token&&requests==1&&!worker);
  preparoVigiar();assert(!preparo.tem);
  puts("PASS busy trailer caches one funded source; idle queues native once; no HTTP/policy reset; stale owner/list/settings/title and refused bridge do not retry");
}
static void flagsAndPolicyTransfer(void) {
  for (int p = 0; p <= 1; p++) for (int a = 0; a <= 1; a++) {
    reset(p,a); prepared();
    assert(preparo.falha != NULL || !(p || a));
    assert(fonteJob.tipo == ((p || a) ? FJOB_PREPARO_FALHA : FJOB_PREPARO));
    assert((p || a) ? requests == 1 && !legadas : legadas == 1 && !requests);
    int idx = -1; FonteFalhaSessao *stored = preparo.falha;
    assert(preparoUsar(catalogItem.imdb,-1,&idx) && idx == 0 && !preparo.tem);
    assert(nativeAdopt == 1 && !nativeCancel);
    if (stored) assert(fonteFalha == stored && fontefalha_tentativas(fonteFalha) == 1);
  }
  reset(1,1); prepared(); nativeState = 2;
  int idx; assert(preparoUsar(catalogItem.imdb,-1,&idx));
  fontefalha_marcar_aberta(fonteFalha); aberto = 1; aguardandoFonte = 2;
  const int services[] = {DEBRID_TB, DEBRID_TB, DEBRID_PM};
  const int indices[] = {1,2,0};
  for (int i = 0; i < 3; i++) {
    fontePedidoGeracao = novaGeracaoFonte();
    assert(pedirFonteJob(FJOB_FALHA,fontePedidoGeracao,NULL,0) == 0);
    concluirWorker(); processarFonteJob();
    assert(fonteEscolhida == indices[i] && fontes[indices[i]].debridServico == services[i]+1);
    assert(fontefalha_tentativas(fonteFalha) == i+2);
    fontefalha_marcar_aberta(fonteFalha);
  }
  assert(requests == 4 && !legadas);
  puts("PASS all failover flags and prepared policy retains three TB links across addons before PM");
}
static void inFlightPlayAndWorkerInvalidation(void) {
  reset(1,1); prepararAoAbrir(); assert(worker && requests == 0);
  assert(preparoEmCurso(catalogItem.imdb,-1));
  aberto = 1; aguardandoFonte = 2; preparoEspera = 1; preparoLembradaPlay = -1;
  fontePedidoGeracao = novaGeracaoFonte(); preparoVigiar();
  assert(preparoRodando && preparoEspera && fontePendenteTipo == FJOB_NONE);
  concluirWorker(); processarFonteJob();
  assert(requests == 1 && fonteEscolhida == 0 && fonteFalha && !nativeStarts && !worker);
  assert(fontefalha_tentativas(fonteFalha) == 1 && !fonteJob.falha);
  for (int cause = 0; cause < 9; cause++) {
    reset(1,1); prepararAoAbrir(); concluirWorker();
    switch(cause) {
      case 0: openDetail = 0; break;
      case 1: strcpy(account,"account-b"); break;
      case 2: profile++; break;
      case 3: strcpy(catalogItem.imdb,"tt-other"); break;
      case 4: lista++; break;
      case 5: settingsPrepare = 0; break;
      case 6: ruleVersion++; break;
      case 7: credentialContext.chave[DEBRID_TB][0]++; break;
      case 8: credentialContext.busca++; break;
    }
    preparoVigiar(); processarFonteJob();
    assert(!preparo.tem && !preparo.falha && !fonteJob.falha && !nativeStarts && !aplicacoes);
  }
  reset(1,1); credentialContext.temporada = 3; credentialContext.episodio = 7;
  prepararAoAbrir(); assert(worker);
  credentialContext.chave[DEBRID_TB][0]++; credentialContext.temporada = 4; credentialContext.episodio = 2;
  concluirWorker();
  assert(fonteJob.debrid.chave[DEBRID_TB][0] == expectedKey);
  assert(fonteJob.debrid.temporada == 3 && fonteJob.debrid.episodio == 7);
  processarFonteJob(); assert(!preparo.tem && !fonteJob.falha && contextEntrances == contextExits);
  puts("PASS Play during resolving reuses one worker; stale completed jobs rejected");
}
static void cacheCancellation(void) {
  for (int cause = 0; cause < 11; cause++) {
    reset(1,1); prepared(); assert(nativeStarts == 1);
    switch(cause) {
      case 0: openDetail = 0; break;
      case 1: strcpy(account,"account-b"); break;
      case 2: profile++; break;
      case 3: strcpy(catalogItem.imdb,"tt-other"); break;
      case 4: lista++; break;
      case 5: settingsPrepare = 0; break;
      case 6: ruleVersion++; break;
      case 7: settingsManual = 1; break;
      case 8: settingOrder++; break;
      case 9: credentialContext.chave[DEBRID_TB][0]++; break;
      case 10: credentialContext.busca++; break;
    }
    preparoVigiar(); assert(!preparo.tem && !preparo.falha && nativeCancel == 1);
  }
  reset(1,1); prepared(); remembered = 1;
  int idx = -1; assert(!preparoUsar(catalogItem.imdb, remembered, &idx) && nativeCancel == 1);
  puts("PASS account, profile, title, list, settings, remembered-source and Back cancel cache");
}
static void transportEpochCancelsBeforeSpending(void) {
  reset(1,1); prepararAoAbrir(); assert(worker && fonteJob.falha);
  DebridContexto owner = credentialContext;
  credentialContext.busca++;
  assert(credentialContext.sessao == owner.sessao &&
         !memcmp(credentialContext.chave, owner.chave, sizeof owner.chave));
  assert(!preparoEmCurso(catalogItem.imdb, -1));
  preparoVigiar();
  assert(!preparoRodando && !preparoEspera && fonteJobCancelado(&fonteJob));
  concluirWorker();
  assert(!requests && !renewals && !nativeStarts &&
         fontefalha_tentativas(fonteJob.falha) == 0);
  processarFonteJob(); assert(!preparo.tem && !preparo.falha && !fonteJob.falha);
  puts("PASS same owner/credentials with only transport epoch change cancels before spending source budget");
}
#ifdef NUVIO_SOURCE_EPOCH
static void ownerVisitEpochRejectsABA(void) {
  for(int kind=0;kind<2;kind++) {
    reset(1,1); actorDuringCapture=1; preparoLembradaRod=-1;
    int r=iniciarFonteJob(kind?FJOB_PREPARO_FALHA:FJOB_ADDON,lista,catalogItem.imdb,0);
    assert(r==-1&&!worker&&!fioFonteVivo&&!fonteJob.falha&&!requests&&!nativeStarts&&!legadas);
  }
  reset(1,1); prepared(); fixtureActorAfterUnlock=1; int titleIdx=-1;
  assert(!preparoUsar(catalogItem.imdb,-1,&titleIdx)&&!nativeAdopt&&nativeCancel==1&&!fonteFalha);
  reset(1,1); prepared();
  int oldProfile=profile; char oldAccount[sizeof account]; strcpy(oldAccount,account);
  profile++; strcpy(account,"temporary-owner"); atomic_fetch_add(&fixtureSession,1);
  profile=oldProfile; strcpy(account,oldAccount); atomic_fetch_add(&fixtureSession,1);
  int idx=-1; assert(!preparoUsar(catalogItem.imdb,-1,&idx)&&nativeCancel==1&&!nativeAdopt);
  reset(1,1); prepararAoAbrir();
  atomic_fetch_add(&fixtureSession,2); // actor A -> B -> A before the UI polls
  assert(fonteJobCancelado(&fonteJob)); concluirWorker(); processarFonteJob();
  assert(!requests&&!nativeStarts&&!aplicacoes&&!preparo.tem&&!fonteJob.falha);
  reset(0,0); prepararAoAbrir(); atomic_fetch_add(&fixtureSession,2);
  concluirWorker(); processarFonteJob(); assert(!legadas&&!requests&&!preparo.tem);
#ifndef NUVIO_COMBINED
  reset(1,1); prepared(); proximoIntento.tem=1; aberto=1; playerT=2; playerE=7; closes=0;
  unsigned old=atomic_load(&preparoGeracao); preparoEncerrarPerfil();
  assert(!preparo.tem&&!preparo.falha&&!preparo.token&&!proximoIntento.tem&&nativeCancel==1);
  assert(closes==1&&savedT==2&&savedE==7&&!aberto&&atomic_load(&preparoGeracao)==old+1);
  assert(!streamsCount&&!aguardandoFonte&&fontePendenteTipo==FJOB_NONE);
#endif
  puts("PASS source owner ABA: unchanged final account/profile rejects cache and old workers; before-selection hook closes old owner and cancels prepared Next");
}
#endif
static void nativeFaultContinuation(void) {
  for(int stale=0;stale<2;stale++) {
    reset(0,0); prepared(); nativeState=-1;
    if(stale) atomic_fetch_add(&fixtureSession,2); else lista++;
    int idx=-1; assert(!preparoUsar(catalogItem.imdb,-1,&idx));
    assert(!mortas&&!preparo.tem&&!preparo.falha&&!nativeAdopt&&nativeCancel==1);
  }
  reset(1,1); prepared();
  nativeState=0; assert(preparoVale(catalogItem.imdb,-1)); prepararAoAbrir();
  assert(!worker&&requests==1&&fontefalha_tentativas(preparo.falha)==1);
  const int expectedIndex[]={1,2,0}, expectedService[]={DEBRID_TB,DEBRID_TB,DEBRID_PM};
  for(int i=0;i<3;i++) {
    nativeState=-1; prepararAoAbrir(); assert(worker&&!fonteJob.renovar&&preparo.falhou);
    nativeState=1; concluirWorker(); processarFonteJob();
    assert(preparo.idx==expectedIndex[i]&&preparo.servico==expectedService[i]);
    assert(fontefalha_tentativas(preparo.falha)==i+2&&requests==i+2&&!renewals);
  }
  reset(1,1); prepared(); nativeState=-1;
  int idx; assert(preparoUsar(catalogItem.imdb,-1,&idx)&&idx==-2&&fonteFalha);
  assert(fontefalha_tentativas(fonteFalha)==1&&!worker);
  aberto=1; aguardandoFonte=2; fontePedidoGeracao=novaGeracaoFonte();
  assert(pedirFonteJob(FJOB_FALHA,fontePedidoGeracao,NULL,0)==0);
  concluirWorker(); processarFonteJob(); assert(fonteEscolhida==1&&fontefalha_tentativas(fonteFalha)==2);
  puts("PASS native0 cancellation preserves link; native-1 advances same three-TB-then-PM budget; Play continues spent ledger");
}
static void expirationRenewalAndBoundedBuffer(void) {
  reset(1,1); prepared();
  agora += FA_PREPARADA_VALE_MS + 1; listAge = FA_LINK_VALE_MS + 100;
  prepararAoAbrir(); assert(worker && fonteJob.renovar && fonteJob.servico == DEBRID_TB);
  assert(fontefalha_tentativas(fonteJob.falha) == 1);
  concluirWorker(); processarFonteJob();
  assert(renewals == 1 && requests == 1 && nativeStarts == 2 && nativeCancel == 1);
  assert(preparo.linkRenovado && fontefalha_tentativas(preparo.falha) == 1 && !listRenew);
  for (int i=0;i<100;i++) prepararAoAbrir();
  assert(!worker && renewals == 1 && requests == 1);
  nativeState = 2; agora += FA_PREPARADA_VALE_MS+1; listAge += FA_PREPARADA_VALE_MS+1;
  assert(preparoVale(catalogItem.imdb,-1)); prepararAoAbrir(); assert(!worker);
  agora = preparo.quando + FA_BUFFER_VALE_MS;
  assert(preparoVale(catalogItem.imdb,-1));
  agora++; assert(!preparoVale(catalogItem.imdb,-1));
  prepararAoAbrir(); assert(worker && fonteJob.renovar); concluirWorker(); processarFonteJob();
  reset(1,1); prepared(); nativeState = -1; agora += FA_PREPARADA_VALE_MS+1;
  assert(!preparoVale(catalogItem.imdb,-1)); prepararAoAbrir(); assert(worker);
  concluirWorker(); processarFonteJob();
  reset(0,0); fontes[0].infoHash[0] = 0; prepared();
  nativeState = 0; listAge = FA_LINK_VALE_MS+1; agora += FA_LINK_VALE_MS+1;
  prepararAoAbrir(); assert(listRenew == 1 && !preparo.tem && !worker);
  for(int i=0;i<100;i++) prepararAoAbrir();
  assert(listRenew == 1 && !worker);
  puts("PASS 45sec native renew preserves budget; ready buffer120sec; failed native fallback; stale direct list refresh throttled");
}
#ifdef NUVIO_COMBINED
static void combinedProfileLifetime(void);
#endif
int main(int argc, char **argv) {
  assert(argc == 2); fontefalha_dir(argv[1]);
  while (fontefalha_provedor_posicao(DEBRID_TB) > 0) assert(fontefalha_provedor_mover(DEBRID_TB,-1));
#ifdef NUVIO_COMBINED
  combinedProfileLifetime();
#endif
  nextAppTransitionAndCancellation(); distinctSettingsAndEpisodes(); flagsAndPolicyTransfer(); inFlightPlayAndWorkerInvalidation(); cacheCancellation(); transportEpochCancelsBeforeSpending();
#ifdef NUVIO_SOURCE_EPOCH
  ownerVisitEpochRejectsABA();
#endif
  nativeFaultContinuation(); expirationRenewalAndBoundedBuffer(); lateIdleNativeRetry();
  preparoInvalidar(); limparFonteVOD();
  puts("source preload controller: PASS"); return 0;
}

#if defined(NUVIO_COMBINED) || defined(NUVIO_SOURCE_EPOCH)
int stream_primeira_boa_sessao(int n,uint64_t s) { return perfis_sessao_atual(s) ? stream_primeira_boa(n) : -1; }
int stream_verificar_proxima_vod_sessao(int i,int n,int *c,uint64_t s) { return perfis_sessao_atual(s) ? stream_verificar_proxima_vod(i,n,c) : -1; }
int stream_canal_primeira_viva_sessao(int n,uint64_t s) { return perfis_sessao_atual(s) ? stream_canal_primeira_viva(n) : -1; }
int stream_falha_aplicar_sessao(int i,unsigned g,int p,const char *u,uint64_t s) { return perfis_sessao_atual(s) ? stream_falha_aplicar(i,g,p,u) : 0; }
int stalker_resolver_sessao(const char *id,char *u,unsigned n,uint64_t s) { return perfis_sessao_atual(s) ? stalker_resolver(id,u,n) : 0; }
#endif

void player_descartar_retido(void) {}
void fontevolta_esquecer(const char *why) { (void)why; }
void detail_fechar_seco(void) { openDetail=0; }
#ifdef NUVIO_COMBINED
void ponteiro_abrir_sessao(void) {}
void ctx_abrir_sessao(void) {}
void estatvivo_fechar(void) {}
void celb_fechar(void) {}
void trailer_fechar(void) {}
void p2pmotor_cancelar(void) {}
int vertudo_aberta(void) { return 0; }
void vertudo_fechar_seco(void) {}
void spot_fechar(void) {}
void spainel_fechar(void) {}
void central_fechar(void) {}
void ilha_modal_fechar(int s) { (void)s; }
void ilhacart_esquecer_vivo(void) {}
void home_retomar_esquecer(void) {}
void perfil_definir_dados(const PerfilDados *data) { (void)data; }
void ilhasalvar_abrir_sessao(void) {}
void ilhaacao_abrir_sessao(void) {}
void agendaviso_abrir_sessao(void) {}
void ilha_abrir_sessao(void) {}
static void combinedProfileLifetime(void) {
  reset(1,1); prepared(); proximoIntento.tem=1;
  unsigned old=atomic_load(&preparoGeracao);
  perfilEncerrarSessao();
  assert(!preparo.tem&&!preparo.falha&&!preparo.token&&!preparoRodando&&!preparoEspera&&!proximoIntento.tem);
  assert(nativeCancel==1&&atomic_load(&preparoGeracao)==old+1);
  reset(1,1); prepararAoAbrir(); old=atomic_load(&preparoGeracao);
  perfilEncerrarSessao(); atomic_fetch_add(&fixtureSession,1);
  assert(atomic_load(&preparoGeracao)==old+1&&!preparoRodando);
  concluirWorker(); processarFonteJob(); assert(!preparo.tem&&!fonteJob.falha&&!requests&&!nativeStarts);
  reset(1,1); prepararAoAbrir(); concluirWorker(); atomic_fetch_add(&fixtureSession,1);
  processarFonteJob(); assert(!preparo.tem&&!fonteJob.falha&&!aplicacoes&&!nativeStarts);
  reset(1,1); prepared(); atomic_fetch_add(&fixtureSession,1);
  int idx; assert(!preparoUsar(catalogItem.imdb,-1,&idx)&&nativeCancel==1);
  puts("PASS combined same-profile visit invalidates cache/jobs; real close hook frees policy/token and cancels pending Next");
}
#endif
