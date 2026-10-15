// Exercises the real private/public preparation helpers, with bounded
// subtitle and native-player transports. No UI, account network or media.
#include "player.c"
#include <assert.h>
static int fixtureEngineActive=1,fixtureEngineReady=1;

static CatItem fixtureTitle;
static Stream fixtureStream;
static Uint32 clockFixture;
static int subtitlePhase, subtitleStarts, subtitleStops, nativeStarts, nativeStops;
static int nativeState = 1, nativeAdoption, subtitlesEnabled, preloadEnabled = 1, nextNativeStarts;
static unsigned nativeToken;
static int visibleStops, nativeQueueResult = 1, faixasNormal, faixasPreserved, visibleWarmOpens, visibleColdOpens;
static int futureStarts, futureCancels, futureAdopts, futureAllocations, futureFrees, failFutureAdoption;
static int currentSubtitleGeneration;




static double nativeSeconds, nativePercent;
static char subtitlePref[16] = "en", audioPref[16]="en", nativeAudio[16], lastProgressKey[48];
static int audioPreferenceReads, subtitleUsesOriginal, audioUsesOriginal, originalUpdates;
static char originalLanguage[16]="en", titleOriginalLanguage[16]="en";
static ProgRegistro fixtureProgress;
static int hasProgress;
static MkvassEscolher subtitleChoice;

Uint32 SDL_GetTicks(void) { return clockFixture; }
const CatItem *cat_item(int i) { return i == 0 ? &fixtureTitle : NULL; }
const Stream *stream_item(int i) { return i == 0 ? &fixtureStream : NULL; }
int video_ativo(void) { return fixtureEngineActive; }
int ajustes_buffer_preset(void) { return 0; }
int ajustes_fonte_precarregar(void) { return preloadEnabled; }
int ajustes_cache_seek_mb(void) { return 0; }
int ajustes_cw_concluido(void) { return 90; }
const char *ling_legenda(void) { return subtitleUsesOriginal ? originalLanguage : subtitlePref; }
const char *ling_audio(void) { audioPreferenceReads++;return audioUsesOriginal ? originalLanguage : audioPref; }
int ling_legenda_auto(const char *pref, const char *const *emb, int n,
                     int atual, const char *const *ext, int ne, int exato) {
  (void)atual; (void)ext; (void)ne; (void)exato;
  for (int i = 0; i < n; i++) if (!strcmp(pref, emb[i])) return i;
  return -1;
}
void prog_chave(char *dst, unsigned n, const char *id, int t, int e) {
  if (e) snprintf(dst, n, "%s_s%de%d", id, t, e);
  else snprintf(dst, n, "%s", id);
}
int prog_por_chave(const char *chave, ProgRegistro *r) {
  snprintf(lastProgressKey, sizeof lastProgressKey, "%s", chave);
  if (hasProgress) *r = fixtureProgress;
  return hasProgress;
}
int mkvass_prebuscar(const char *url, MkvassEscolher escolher, double fracInicio) {
  (void)url; (void)fracInicio;
  subtitleStarts++; subtitleChoice = escolher; subtitlePhase = 1;
  return subtitlesEnabled;
}
int mkvass_prebusca_fase(void) { return subtitlePhase; }

void mkvass_parar(void) {
  subtitleStops++; subtitlePhase = 0; currentSubtitleGeneration=0;

}








// Controlled visible transport: an armed handoff protects the separate preload.
void video_parar(void) {
  visibleStops++;

}


// Closing exercises the real fecharSessao; these adjacent modules are controlled.
// No media was played in this fixture, so progress/watch callbacks must stay silent.
int cat_indice_titulo(const char *id, int preferido) { (void)id; return preferido; }
int cat_n(void) { return 1; }
int cat_acrescentar(const CatItem *ci) { (void)ci; return 0; }
static CatEp fixtureEpisodes[2];
int cat_n_episodios(int i) { (void)i; return 2; }
const CatEp *cat_episodio(int i, int ep) { (void)i; return ep>=0&&ep<2?&fixtureEpisodes[ep]:NULL; }
int cat_id_stream(int i, int t, int e, char *dst, unsigned cap) {
  (void)i; snprintf(dst, cap, "tt42:%d:%d", t, e); return 1;
}
int trailer_aberto(void) { return 0; }
unsigned trailer_geracao(void) { return 0; }
const char *video_url_atual(void) { return fixtureStream.url; }
double video_creditos(void) { return 2946.94; }
int video_pronto(void) { return fixtureEngineReady; }
int video_falhou(void) { return 0; }
void video_velocidade(int valor) { (void)valor; }
void video_dv_tela(int cobrindo) { (void)cobrindo; }
int dvtela_ativa(void) { return 0; }
void dvtela_sair(int motivo, Uint32 quando) { (void)motivo; (void)quando; }
double intro_creditos_seg(void) { return 2946.94; }
int intro_creditos_aniskip(void) { return 0; }
double intro_creditos_janela(double dur) { (void)dur; return 300; }
double intro_fim_estimado(double dur) { (void)dur; return 50; }
double cred_escolher(const CredEntrada *entrada, int *fonte) {
  (void)entrada; *fonte = CRED_FONTE_CAPITULO; return 2946.94;
}
int cred_viz_ler(const char *id, int t, double *inicio, double *dur) {
  (void)id; (void)t; *inicio = *dur = 0; return 0;
}
double cred_aprendido_resto(const char *id) { (void)id; return 0; }
const char *cred_fonte_nome(int fonte) { (void)fonte; return "fixture-none"; }
int stream_atual(void) { return 0; }
Uint32 stream_idade_ms(void) { return 0; }
int perfis_ativo(void) { return 1; }
const char *sessao_usuario(void) { return "fixture-account"; }
void fontevolta_esquecer(const char *motivo) { (void)motivo; }
void fontevolta_guardar(const char *alvo, const char *conta, int perfil,
                       const Stream *fonte, Uint32 idade, Uint32 quando) {
  (void)alvo; (void)conta; (void)perfil; (void)fonte; (void)idade; (void)quando;
  assert(!"No played media may create a return checkpoint");
}
void servidores_reproducao_fim(const char *url, double pos, double dur) {
  (void)url; (void)pos; (void)dur;
}
void legsync_encerrar(void) {}
void pausao_fechar(void) {}
void reacao_fechar(void) {}
void episodios_fechar(void) {}
void intro_desligar(void) {}
void seekr_desligar(void) {}
void mkvass_video_aberto(int abertoVideo) { (void)abertoVideo; }
void legenda_desligar(void) {}
void home_registrar_retorno(int i, double pos, double dur) {
  (void)i; (void)pos; (void)dur; assert(!"No playback may create a Home checkpoint");
}
void ilhacart_player_saiu(int i, double pos, double dur, int t, int e) {
  (void)i; (void)pos; (void)dur; (void)t; (void)e;
  assert(!"No playback may create a live activity checkpoint");
}
void cat_salvar_progresso_ep(int i, double pos, double dur, int t, int e, int tocou) {
  (void)i; (void)pos; (void)dur; (void)t; (void)e; (void)tocou;
  assert(!"Preloading must not save progress");
}
void desc_refazer_continuar(void) { assert(!"No playback may refresh progress"); }
void trakt_marcar(const char *id, double pos, double dur, double assistido, int viu) {
  (void)id; (void)pos; (void)dur; (void)assistido; (void)viu;
  assert(!"Preloading must not scrobble");
}
void recomenda_atividade_fim(const CatItem *ci, int viu) {
  (void)ci; (void)viu; assert(!"Preloading must not publish activity");
}
void atividade_player_saiu(double pos, double dur, int viu) {
  (void)pos; (void)dur; (void)viu; assert(!"Preloading must not publish activity");
}
void sync_sujar_progresso(void) { assert(!"Preloading must not save account progress"); }
int visto_destinos(void) { assert(!"Preloading must not mark watched"); return 0; }
int vistoep_estado(const char *id, int t, int e) {
  (void)id; (void)t; (void)e; assert(!"Preloading must not mark watched"); return -1;
}
int visto_episodios(const char *id, const char *tipo, const VistoPar *pares,
                   int n, int viu, int destinos) {
  (void)id; (void)tipo; (void)pares; (void)n; (void)viu; (void)destinos;
  assert(!"Preloading must not mark watched"); return 0;
}
int visto_titulo(const char *id, const char *tipo, const int *temporadas,
                 int n, int viu, int destinos) {
  (void)id; (void)tipo; (void)temporadas; (void)n; (void)viu; (void)destinos;
  assert(!"Preloading must not mark watched"); return 0;
}
void vistoep_definir(const char *id, int t, int e, int viu) {
  (void)id; (void)t; (void)e; (void)viu; assert(!"Preloading must not mark watched");
}

// Full production opening/source APIs run; transport/UI boundaries are quiet.
char *SDL_GetBasePath(void) { return strdup("/tmp/nuvio-player-fixture-no-prefs/"); }
void SDL_free(void *p) { free(p); }
int video_largura(void) { return 1920; }
int video_altura(void) { return 1080; }
int video_recorte_fonte(void) { return 0; }
void video_janela(int x,int y,int w,int h) { (void)x;(void)y;(void)w;(void)h; }
void video_janela_fonte(int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh) {
  (void)sx;(void)sy;(void)sw;(void)sh;(void)dx;(void)dy;(void)dw;(void)dh;
}
int video_registro_negado(void) { return 0; }
const char *i18n(const char *s) { return s; }
void marco(const char *s) { (void)s; }
void video_definir_reconexao(int on) { (void)on; }
void video_definir_modo_live(int m) { (void)m; }
void cacheboost_backend_cache(int mb) { assert(mb==0); }
void cacheboost_backend_buffer(int b) { assert(b==0); }
void cacheboost_sessao(void) {}
void memlog_evento(const char *s) { (void)s; }
const char *proxyts_resolver(const char *url,char *buf,size_t n) { (void)buf;(void)n;return url; }
int video_tocar_retomada(const char *url,double seconds,double percent) {
  assert(!strcmp(url,fixtureStream.url));assert(seconds==nativeSeconds);
  assert(percent==0||percent==nativePercent);
  if(nativeAdoption&&nativeToken==(unsigned)nativeAdoption)visibleWarmOpens++;else visibleColdOpens++;
  nativeAdoption=0;nativeToken=0;return 1;
}
int video_tocar(const char *url) { (void)url;assert(!"empty opening must not start visible media");return 0; }
int ajustes_livetv_modo(void) { return 0; }
void intro_pedir_ids(const char *id,long tmdb,int t,int e,double prev,double next) {
  (void)id;(void)tmdb;(void)t;(void)e;(void)prev;(void)next;
}
int stream_n(void) { return 1; }
int stream_lista_do_alvo(const char *id) { (void)id;return 1; }
void stream_invalidar(const char *why) { (void)why;assert(!"matching target must retain its list"); }
int video_dv_candidato(const char *url) { (void)url;return 0; }
void dvtela_definir_arte(const char *url) { (void)url; }
void dvtela_entrar(Uint32 now) { (void)now; }
const char *artehero_url(const CatItem *ci) { (void)ci;return ""; }
void trailer_fechar(void) {}
const char *extras_idioma_original(void) { return titleOriginalLanguage; }
void ling_definir_original(const char *lang) { originalUpdates++;snprintf(originalLanguage,sizeof originalLanguage,"%s",lang); }
void desc_episodios_garantir(int i) { assert(i==0); }
void artehero_logo_sessao_iniciar(const CatItem *ci) { (void)ci; }
void guia_carregar(void) {}
void epg_iniciar(void) {}
int ajustes_proporcao_padrao(void) { return -1; }
int ajustes_legenda_sync_auto(void) { return 0; }
void legsync_auto_habilitar(int on) { (void)on; }
LegendaDocumento *mkvass_referencia(const char *url,uint64_t epoch,const int *ex,int n) {
  (void)url;(void)epoch;(void)ex;(void)n;return NULL;
}
void legref_fonte_local(LegendaDocumento *(*f)(const char *,uint64_t,const int *,int)) { (void)f; }
void legref_fonte_cabecalhos(const char *(*f)(void)) { (void)f; }
const char *video_cabecalhos(void) { return fixtureStream.cabecalhos; }
void legsync_iniciar(const char *url) { (void)url; }
void legsync_ui_ligar(void) {}
int ajustes_classif_player(void) { return 0; }
void parental_pedir(const char *id) { (void)id; }
// Adjacent opening transports remain inert; the real player opening runs.
void faixas_reiniciar(void) { faixasNormal++;mkvass_parar(); }
void faixas_reiniciar_preparada(void) { faixasPreserved++; }



static void resetFixture(void) {
 memset(&fixtureTitle,0,sizeof fixtureTitle);memset(&fixtureStream,0,sizeof fixtureStream);
 strcpy(fixtureTitle.imdb,"tt42");strcpy(fixtureTitle.tipo,"movie");
 strcpy(fixtureStream.url,"https://fixture.invalid/video.mp4");fixtureStream.mp4=1;
 clockFixture=100;prebuscaUrl[0]=0;esperandoFonte=0;
 duracaoSeg=posSeg=assistidoSeg=0;duracaoReal=temFixo=canalSessao=0;
 aberto=mini=retido=comVideo=semRetomada=retomarPct=0;retomarSeg=0;retomarFonteSeg=-1;
}
