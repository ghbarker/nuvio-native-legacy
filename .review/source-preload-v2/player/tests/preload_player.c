// Exercises the real private/public preparation helpers, with bounded
// subtitle and native-player transports. No UI, account network or media.
#include "player.c"
#include <assert.h>

static CatItem fixtureTitle;
static Stream fixtureStream;
static Uint32 clockFixture;
static int subtitlePhase, subtitleStarts, subtitleStops, nativeStarts, nativeStops;
static int nativeState = 1, nativeAdoption, subtitlesEnabled, preloadEnabled = 1, nextNativeStarts;
static unsigned nativeToken;
static int visibleStops, nativeQueueResult = 1, faixasNormal, faixasPreserved, visibleWarmOpens, visibleColdOpens;
static int futureStarts, futureCancels, futureAdopts, futureAllocations, futureFrees, failFutureAdoption;
static int currentSubtitleGeneration;
struct MkvassPreparo { char url[4096], preference[16]; double fraction; int phase, generation; MkvassEscolherCom choose; };
static MkvassPreparo *currentFuture;
static MkvassPreparo *newestFuture;

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
int video_ativo(void) { return 0; }
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
int mkvass_prebusca_fase(void) { return currentFuture ? currentFuture->phase : subtitlePhase; }
static void freeFuture(MkvassPreparo *p) { if(newestFuture==p) newestFuture=NULL; free(p); futureFrees++; }
void mkvass_parar(void) {
  subtitleStops++; subtitlePhase = 0; currentSubtitleGeneration=0;
  if(currentFuture) { freeFuture(currentFuture); currentFuture=NULL; }
}
MkvassPreparo *mkvass_preparo_criar(const char *url, MkvassEscolherCom choose,
                                  const void *context, size_t contextSize, double fraction) {
  assert(url&&choose&&context&&contextSize==sizeof preTitulo.idioma);
  if(!subtitlesEnabled) return NULL;
  MkvassPreparo *p=calloc(1,sizeof *p); assert(p); futureAllocations++;futureStarts++;
  snprintf(p->url,sizeof p->url,"%s",url); memcpy(p->preference,context,contextSize);
  p->fraction=fraction;p->phase=1;p->generation=200+futureAllocations;p->choose=choose;
  newestFuture=p; return p;
}
int mkvass_preparo_fase(MkvassPreparo *p) { assert(p);return p->phase; }
int mkvass_preparo_adotar(MkvassPreparo **slot,const char *url,double fraction) {
  assert(slot&&*slot);MkvassPreparo *p=*slot;
  if(failFutureAdoption||strcmp(url,p->url)||fabs(fraction-p->fraction)>0.000001) return 0;
  if(currentFuture) freeFuture(currentFuture);
  currentFuture=p;currentSubtitleGeneration=p->generation;*slot=NULL;futureAdopts++;return 1;
}
void mkvass_preparo_cancelar(MkvassPreparo **slot) {
  if(!slot||!*slot)return;freeFuture(*slot);*slot=NULL;futureCancels++;
}
int video_precarregar(const char *url, const char *cab, double segundos,
                     double pct, int buffer, int cacheMb, unsigned token, const char *audio) {
  assert(audio);snprintf(nativeAudio,sizeof nativeAudio,"%s",audio);
  assert(!strcmp(url, fixtureStream.url)); assert(!strcmp(cab, fixtureStream.cabecalhos));
  assert(buffer == 0 && cacheMb == 0);
  nativeStarts++; nativeSeconds = segundos; nativePercent = pct;
  if(nativeQueueResult) nativeToken=token;
  return nativeQueueResult;
}
int video_precarregar_estado(unsigned token) { return token == nativeToken ? nativeState : 0; }
int video_precarregar_proximo(const char *url, const char *cab, double segundos,
                             double pct, int buffer, int cacheMb, unsigned token, const char *audio) {
  nextNativeStarts++;
  return video_precarregar(url, cab, segundos, pct, buffer, cacheMb, token, audio);
}
void video_precarregar_cancelar(unsigned token) {
  if (!token || nativeAdoption == (int)token) nativeAdoption = 0;
  if (!token || token == nativeToken) { nativeStops++; nativeToken = 0; }
}
// Controlled visible transport: an armed handoff protects the separate preload.
void video_parar(void) {
  visibleStops++;
  if (!nativeAdoption) video_precarregar_cancelar(0);
}
void video_precarregar_adotar(unsigned token) { nativeAdoption = (int)token; }

// Closing exercises the real fecharSessao; these adjacent modules are controlled.
// No media was played in this fixture, so progress/watch callbacks must stay silent.
int cat_indice_titulo(const char *id, int preferido) { (void)id; return preferido; }
int cat_n(void) { return 1; }
int cat_acrescentar(const CatItem *ci) { (void)ci; return 0; }
int cat_n_episodios(int i) { (void)i; return 0; }
const CatEp *cat_episodio(int i, int ep) { (void)i; (void)ep; return NULL; }
int cat_id_stream(int i, int t, int e, char *dst, unsigned cap) {
  (void)i; snprintf(dst, cap, "tt42:%d:%d", t, e); return 1;
}
int trailer_aberto(void) { return 0; }
unsigned trailer_geracao(void) { return 0; }
const char *video_url_atual(void) { return fixtureStream.url; }
double video_creditos(void) { return 0; }
int video_pronto(void) { return 0; }
int video_falhou(void) { return 0; }
void video_velocidade(int valor) { (void)valor; }
void video_dv_tela(int cobrindo) { (void)cobrindo; }
int dvtela_ativa(void) { return 0; }
void dvtela_sair(int motivo, Uint32 quando) { (void)motivo; (void)quando; }
double intro_creditos_seg(void) { return 0; }
int intro_creditos_aniskip(void) { return 0; }
double intro_creditos_janela(double dur) { (void)dur; return 0; }
double intro_fim_estimado(double dur) { (void)dur; return 0; }
double cred_escolher(const CredEntrada *entrada, int *fonte) {
  (void)entrada; *fonte = CRED_FONTE_NENHUMA; return 0;
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
void posplay_fechar(void) {}
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
void relogio_zerar(Relogio *r) { memset(r,0,sizeof *r); }
void velmed_zerar(VelMedidor *m) { memset(m,0,sizeof *m); }
int ajustes_classif_player(void) { return 0; }
void parental_pedir(const char *id) { (void)id; }
// Adjacent opening transports remain inert; the real player opening runs.
void faixas_reiniciar(void) { faixasNormal++;mkvass_parar(); }
void faixas_reiniciar_preparada(void) { faixasPreserved++; }
static void resetFixture(void) {
  player_precarregar_cancelar(0); mkvass_parar();
  assert(futureAllocations==futureFrees&&!currentFuture&&!newestFuture);
  memset(&fixtureTitle, 0, sizeof fixtureTitle); memset(&fixtureStream, 0, sizeof fixtureStream);
  snprintf(fixtureTitle.imdb, sizeof fixtureTitle.imdb, "tt42");
  snprintf(fixtureTitle.tipo, sizeof fixtureTitle.tipo, "movie");
  snprintf(fixtureStream.url, sizeof fixtureStream.url, "https://fixture.invalid/video.mp4");
  snprintf(fixtureStream.cabecalhos, sizeof fixtureStream.cabecalhos, "Authorization: fixture-only");
  fixtureStream.mp4 = 1;
  clockFixture = 100; subtitlePhase = subtitleStarts = subtitleStops = nativeStarts = nativeStops = 0;
  nativeAdoption = subtitlesEnabled = hasProgress = 0; nativeState = 1;
  preloadEnabled = 1; nextNativeStarts = 0; visibleStops = 0; nativeQueueResult=1;
  futureStarts=futureCancels=futureAdopts=failFutureAdoption=0;currentSubtitleGeneration=0;
  faixasNormal=faixasPreserved=visibleWarmOpens=visibleColdOpens=0;
  prebuscaUrl[0] = 0; esperandoFonte = 0;
  duracaoSeg = posSeg = assistidoSeg = 0; duracaoReal = temFixo = canalSessao = 0;
  aberto = mini = retido = comVideo = semRetomada = retomarPct = 0;
  retomarSeg = 0; retomarFonteSeg = -1;
  strcpy(subtitlePref, "en");strcpy(audioPref,"en");nativeAudio[0]=0;audioPreferenceReads=0;
  subtitleUsesOriginal=audioUsesOriginal=originalUpdates=0;strcpy(originalLanguage,"en");strcpy(titleOriginalLanguage,"en");
}
static void mkvFixture(void) {
  fixtureStream.mp4 = 0;
  snprintf(fixtureStream.url, sizeof fixtureStream.url, "https://fixture.invalid/video.mkv");
  subtitlesEnabled = 1;
}
int main(void) {
  // Idle title capture resolves Original from this title; Next preserves current Original.
  for(int next=0;next<2;next++) {
    resetFixture();mkvFixture();strcpy(originalLanguage,"fr");strcpy(titleOriginalLanguage,"en");
    subtitleUsesOriginal=audioUsesOriginal=1;
    if(next) { aberto=comVideo=1;assert(player_precarregar_proxima_fonte(0,1,3,&fixtureStream,29+next)); }
    else assert(player_precarregar_fonte(0,0,0,0,29+next));
    const char *expected=next?"fr":"en";
    assert(!strcmp(originalLanguage,expected)&&originalUpdates==!next);
    assert(!strcmp(preTitulo.idioma,expected)&&!strcmp(preTitulo.audio,expected)&&!strcmp(nativeAudio,expected));
    const char *languages[]={"fr","en"};
    assert(newestFuture->choose(languages,2,newestFuture->preference)==!next);
    if(next)assert(aberto&&comVideo&&!subtitleStops);
    player_precarregar_cancelar(29+next);
  }

  // Both title and Next own their audio preference, including later starts.
  for(int next=0;next<2;next++) {
    resetFixture();strcpy(audioPref,"fr");
    if(next) { aberto=comVideo=1;assert(player_precarregar_proxima_fonte(0,1,3,&fixtureStream,27+next)); }
    else assert(player_precarregar_fonte(0,0,0,0,27+next));
    assert(!strcmp(nativeAudio,"fr")&&!strcmp(preTitulo.audio,"fr")&&audioPreferenceReads==1);
    strcpy(audioPref,"de");int starts=nativeStarts;
    for(int i=0;i<5;i++)assert(player_precarregar_estado(27+next)==1);
    assert(nativeStarts==starts&&audioPreferenceReads==1&&!strcmp(nativeAudio,"fr")&&!strcmp(preTitulo.audio,"fr"));
    // The private restart seam must resend owned bytes, never live settings.
    assert(preTituloComecarVideo());
    assert(nativeStarts==starts+1&&audioPreferenceReads==1&&!strcmp(nativeAudio,"fr"));
    assert(nextNativeStarts==(next?2:0));
    player_precarregar_cancelar(27+next);
  }

  // MP4 starts native buffering without any playback or subtitle work.
  resetFixture(); assert(player_precarregar_fonte(0,0,0,0,1));
  assert(nativeStarts==1&&!aberto&&!comVideo&&!futureStarts&&!subtitleStarts);
  assert(player_precarregar_estado(1)==1&&nativeStarts==1);
  player_precarregar_adotar(1);assert(nativeAdoption==1&&nativeStops==1);
  nativeState=2;assert(player_precarregar_estado(1)==2);

  // Native/MKV requests start together; only readiness waits the original ceiling.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,2));
  assert(futureStarts==1&&nativeStarts==1&&!subtitleStarts&&preTitulo.mkv);
  nativeState=2;clockFixture+=3999;
  assert(player_precarregar_estado(2)==1&&nativeStarts==1);
  const char *languages[]={"fr","en"};strcpy(subtitlePref,"fr");
  assert(newestFuture->choose(languages,2,newestFuture->preference)==1);
  strcpy(subtitlePref,"en");clockFixture++;
  assert(player_precarregar_estado(2)==2&&nativeStarts==1);
  player_precarregar_adotar(2);
  assert(preTituloLegendaUsar(fixtureStream.url)==2&&futureStarts==1&&futureAdopts==1&&!subtitleStarts);

  // A completed future is adopted immediately without another Range/parser start.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,3));
  nativeState=2;newestFuture->phase=2;
  assert(player_precarregar_estado(3)==2);player_precarregar_adotar(3);
  assert(preTituloLegendaUsar(fixtureStream.url)==2&&futureAdopts==1&&futureStarts==1&&!subtitleStarts);
  assert(currentFuture&&!preTitulo.mkv&&currentFuture->phase==2);

  // Early Play adopts the pending future and preserves only its remaining second.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,4));
  nativeState=2;clockFixture+=3000;player_precarregar_adotar(4);
  assert(preTituloLegendaUsar(fixtureStream.url)==1);
  assert(prebuscaDesde==100&&MKVASS_PREBUSCA_MS-(clockFixture-prebuscaDesde)==1000);
  assert(!strcmp(prebuscaUrl,fixtureStream.url)&&futureStarts==1&&futureAdopts==1&&nativeStarts==1&&!subtitleStarts);

  // Cancel owns both future and native transports, and never the current subtitles.
  resetFixture();mkvFixture();subtitlePhase=1;currentSubtitleGeneration=77;
  assert(player_precarregar_fonte(0,0,0,0,5));player_precarregar_cancelar(5);
  clockFixture+=5000;
  assert(player_precarregar_estado(5)==0&&nativeStarts==1&&futureCancels==1);
  assert(!subtitleStops&&subtitlePhase==1&&currentSubtitleGeneration==77&&futureAllocations==futureFrees);

  // Stale cancellation cannot discard a newer title or future context.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,6));
  assert(player_precarregar_fonte(0,0,0,0,7));int stops=nativeStops,cancels=futureCancels;
  player_precarregar_cancelar(6);
  assert(nativeStops==stops&&futureCancels==cancels&&player_precarregar_estado(7)==1&&preTitulo.mkv);

  // Saved seconds/percent and episode identity agree with the normal open rule.
  resetFixture();fixtureTitle.progresso=50;fixtureProgress.posSeg=50;fixtureProgress.durSeg=100;hasProgress=1;
  assert(player_precarregar_fonte(0,0,0,0,8));assert(nativeSeconds==50&&nativePercent==50&&!strcmp(lastProgressKey,"tt42"));
  resetFixture();fixtureTitle.progresso=50;assert(player_precarregar_fonte(0,0,0,0,9));assert(!nativeSeconds&&nativePercent==50);
  resetFixture();strcpy(fixtureTitle.tipo,"series");fixtureTitle.progresso=50;fixtureTitle.temporada=1;fixtureTitle.episodio=2;
  assert(player_precarregar_fonte(0,1,3,0,10));assert(!nativeSeconds&&!nativePercent);

  // Explicit Off, PiP and retained playback still block title speculation.
  resetFixture();mini=1;assert(!player_precarregar_fonte(0,0,0,0,11)&&!nativeStarts&&!originalUpdates);
  mini=0;retido=1;assert(!player_precarregar_fonte(0,0,0,0,12)&&!nativeStarts&&!originalUpdates);
  resetFixture();preloadEnabled=0;assert(!player_precarregar_fonte(0,0,0,0,13)&&!nativeStarts&&!originalUpdates);

  // Preparing Next keeps the current subtitle generation/phase and playback untouched.
  resetFixture();mkvFixture();aberto=comVideo=1;subtitlePhase=1;currentSubtitleGeneration=88;
  assert(player_precarregar_proxima_fonte(0,1,3,&fixtureStream,14));
  assert(nextNativeStarts==1&&nativeStarts==1&&futureStarts==1&&!subtitleStarts&&!subtitleStops);
  assert(subtitlePhase==1&&currentSubtitleGeneration==88&&aberto&&comVideo);
  player_precarregar_cancelar(14);
  assert(subtitlePhase==1&&currentSubtitleGeneration==88&&!subtitleStops&&futureCancels==1&&aberto&&comVideo);

  // Beginning/language/checkpoint/source mismatches cancel only future ownership.
  for(int mismatch=0;mismatch<4;mismatch++) {
    resetFixture();mkvFixture();subtitlePhase=1;currentSubtitleGeneration=99;
    if(mismatch==0)fixtureTitle.progresso=50;
    if(mismatch==2){fixtureTitle.progresso=50;fixtureProgress.posSeg=50;fixtureProgress.durSeg=100;hasProgress=1;}
    assert(player_precarregar_fonte(0,0,0,0,15+mismatch));player_precarregar_adotar(15+mismatch);
    if(mismatch==0){semRetomada=1;retomarPct=0;}
    if(mismatch==1)strcpy(subtitlePref,"fr");
    if(mismatch==2)retomarFonteSeg=55;
    const char *url=mismatch==3?"https://fixture.invalid/other.mkv":fixtureStream.url;
    assert(preTituloLegendaUsar(url)==0&&futureCancels==1&&!futureAdopts);
    assert(!subtitleStops&&subtitlePhase==1&&currentSubtitleGeneration==99&&!nativeToken&&!nativeAdoption);
  }

  // A raced/failed adoption leaves the current context and releases its owned future.
  resetFixture();mkvFixture();subtitlePhase=1;currentSubtitleGeneration=123;
  assert(player_precarregar_fonte(0,0,0,0,19));player_precarregar_adotar(19);failFutureAdoption=1;
  assert(preTituloLegendaUsar(fixtureStream.url)==0&&futureCancels==1&&!futureAdopts&&!preTitulo.mkv);
  assert(!subtitleStops&&currentSubtitleGeneration==123&&subtitlePhase==1&&!nativeToken&&!nativeAdoption);

  // Public source opening falls back once if the independent adoption races.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,26));
  player_precarregar_adotar(26);player_abrir(0,NULL);failFutureAdoption=1;
  player_definir_fonte(fixtureStream.url);
  assert(futureCancels==1&&!futureAdopts&&!preTitulo.mkv&&!nativeToken&&!nativeAdoption);
  assert(subtitleStarts==1&&prebuscaUrl[0]&&esperandoFonte&&!visibleWarmOpens&&!visibleColdOpens);

  // Native failure is exposed unchanged; cancellation releases pending future too.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,20));nativeState=-1;
  assert(player_precarregar_estado(20)==-1);player_precarregar_cancelar(20);
  assert(player_precarregar_estado(20)==0&&futureCancels==1&&!preTitulo.mkv);

  // A refused native constructor cannot strand a future handle behind return0.
  resetFixture();mkvFixture();subtitlePhase=1;currentSubtitleGeneration=124;nativeQueueResult=0;
  assert(!player_precarregar_fonte(0,0,0,0,21));
  assert(futureStarts==1&&futureCancels==1&&!preTitulo.mkv&&!preTitulo.token&&futureAllocations==futureFrees);
  assert(!subtitleStops&&currentSubtitleGeneration==124&&subtitlePhase==1);

  // Signed JNI boundary skips zero and shares one title/Next token sequence.
  atomic_store_explicit(&preTituloTokens,0x7ffffffeu,memory_order_relaxed);
  assert(player_precarregar_novo_token()==0x7fffffffu&&player_precarregar_novo_token()==1&&player_precarregar_novo_token()==2);

  // Actual Stop while source-waiting cancels the adopted native+future handles.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,22));player_precarregar_adotar(22);
  aberto=esperandoFonte=1;stops=nativeStops;fecharSessao(0);
  assert(nativeStops==stops+1&&!nativeToken&&!nativeAdoption&&futureCancels==1&&!preTitulo.mkv&&!aberto&&!comVideo&&!visibleStops);

  // Actual Stop after pending subtitle transfer still cancels native ARM/current future.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,23));player_precarregar_adotar(23);aberto=1;
  assert(preTituloLegendaUsar(fixtureStream.url)==1&&!preTitulo.adotado&&futureAdopts==1&&currentFuture);
  assert(prebuscaUrl[0]&&nativeAdoption==23);fecharSessao(0);
  assert(!nativeAdoption&&!prebuscaUrl[0]&&!currentFuture&&futureAllocations==futureFrees);

  // Outgoing close preserves the already adopted Next native+independent MKV.
  resetFixture();mkvFixture();aberto=comVideo=1;subtitlePhase=1;currentSubtitleGeneration=126;
  assert(player_precarregar_proxima_fonte(0,1,3,&fixtureStream,24));player_precarregar_adotar(24);stops=nativeStops;
  fecharSessao(0);
  assert(visibleStops==1&&nativeStops==stops&&nativeToken==24&&nativeAdoption==24&&preTitulo.mkv&&futureCancels==0);
  MkvassPreparo *sameFuture=preTitulo.mkv;newestFuture->phase=2;nativeState=2;
  player_abrir(0,NULL);
  assert(faixasNormal==1&&!faixasPreserved&&preTitulo.mkv==sameFuture&&sameFuture->phase==2);
  player_definir_fonte(fixtureStream.url);
  assert(futureAdopts==1&&futureStarts==1&&!subtitleStarts&&!preTitulo.mkv);
  assert(visibleWarmOpens==1&&!visibleColdOpens&&currentFuture==sameFuture&&comVideo);
  // Once consumed, the visible teardown owns ARM cancellation in the video backend.
  video_parar();video_precarregar_cancelar(0);mkvass_parar();
  assert(!nativeToken&&!nativeAdoption&&!currentFuture);

  // Ordinary no-video initial close preserves an unadopted background future.
  resetFixture();mkvFixture();assert(player_precarregar_fonte(0,0,0,0,25));stops=nativeStops;fecharSessao(0);
  assert(nativeStops==stops&&nativeToken==25&&!preTitulo.adotado&&preTitulo.mkv&&!futureCancels);
  resetFixture();assert(futureAllocations==futureFrees);
  puts("preload-player: parallel native/MKV, original subtitle deadline, current-Next isolation, exact adoption, cancellation/resume and no owned-handle leaks PASS");
  return 0;
}
