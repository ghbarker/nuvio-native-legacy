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
static int visibleStops;
static double nativeSeconds, nativePercent;
static char subtitlePref[16] = "en", lastProgressKey[48];
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
const char *ling_legenda(void) { return subtitlePref; }
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
void mkvass_parar(void) { subtitleStops++; subtitlePhase = 0; }
int video_precarregar(const char *url, const char *cab, double segundos,
                     double pct, int buffer, int cacheMb, unsigned token) {
  assert(!strcmp(url, fixtureStream.url)); assert(!strcmp(cab, fixtureStream.cabecalhos));
  assert(buffer == 0 && cacheMb == 0);
  nativeStarts++; nativeToken = token; nativeSeconds = segundos; nativePercent = pct;
  return 1;
}
int video_precarregar_estado(unsigned token) { return token == nativeToken ? nativeState : 0; }
int video_precarregar_proximo(const char *url, const char *cab, double segundos,
                             double pct, int buffer, int cacheMb, unsigned token) {
  nextNativeStarts++;
  return video_precarregar(url, cab, segundos, pct, buffer, cacheMb, token);
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

static void resetFixture(void) {
  player_precarregar_cancelar(0);
  memset(&fixtureTitle, 0, sizeof fixtureTitle); memset(&fixtureStream, 0, sizeof fixtureStream);
  snprintf(fixtureTitle.imdb, sizeof fixtureTitle.imdb, "tt42");
  snprintf(fixtureTitle.tipo, sizeof fixtureTitle.tipo, "movie");
  snprintf(fixtureStream.url, sizeof fixtureStream.url, "https://fixture.invalid/video.mp4");
  snprintf(fixtureStream.cabecalhos, sizeof fixtureStream.cabecalhos, "Authorization: fixture-only");
  fixtureStream.mp4 = 1;
  clockFixture = 100; subtitlePhase = subtitleStarts = subtitleStops = nativeStarts = nativeStops = 0;
  nativeAdoption = subtitlesEnabled = hasProgress = 0; nativeState = 1;
  preloadEnabled = 1; nextNativeStarts = 0; visibleStops = 0;
  prebuscaUrl[0] = 0; esperandoFonte = 0;
  duracaoSeg = posSeg = assistidoSeg = 0; duracaoReal = temFixo = canalSessao = 0;
  aberto = mini = retido = comVideo = semRetomada = retomarPct = 0;
  retomarSeg = 0; retomarFonteSeg = -1;
  strcpy(subtitlePref, "en");
}
static void mkvFixture(void) {
  fixtureStream.mp4 = 0;
  snprintf(fixtureStream.url, sizeof fixtureStream.url, "https://fixture.invalid/video.mkv");
  subtitlesEnabled = 1;
}
int main(void) {
  // MP4 starts native buffering before any visible playback session opens.
  resetFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 1));
  assert(nativeStarts == 1 && !aberto && !comVideo && subtitleStarts == 0);
  assert(player_precarregar_estado(1) == 1 && nativeStarts == 1);
  player_precarregar_adotar(1); assert(nativeAdoption == 1 && nativeStops == 1);
  nativeState = 2; assert(player_precarregar_estado(1) == 2);

  // MKV subtitle reading moves ahead of Play, retaining the original ceiling.
  resetFixture(); mkvFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 2));
  assert(subtitleStarts == 1 && nativeStarts == 0);
  clockFixture += 3999;
  assert(player_precarregar_estado(2) == 1 && nativeStarts == 0);
  const char *languages[] = { "fr", "en" };
  strcpy(subtitlePref, "fr");
  assert(subtitleChoice(languages, 2) == 1); // captured preference, not the new one
  strcpy(subtitlePref, "en");
  clockFixture++;
  assert(player_precarregar_estado(2) == 1 && nativeStarts == 1);
  nativeState = 2;
  player_precarregar_adotar(2);
  assert(preTituloLegendaUsar(fixtureStream.url) == 2 && subtitleStarts == 1);

  // Pressing Play during subtitle preparation does not restart its timeout.
  resetFixture(); mkvFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 3));
  clockFixture += 3000; player_precarregar_adotar(3);
  assert(preTituloLegendaUsar(fixtureStream.url) == 1);
  assert(prebuscaDesde == 100 && !strcmp(prebuscaUrl, fixtureStream.url));
  assert(subtitleStarts == 1 && nativeStarts == 0);

  // A canceled title cannot later start its native download.
  resetFixture(); mkvFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 4));
  player_precarregar_cancelar(4);
  clockFixture += 5000;
  assert(player_precarregar_estado(4) == 0 && nativeStarts == 0 && subtitleStops == 1);

  // Stale token cancellation cannot discard a newer title's buffer.
  resetFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 5));
  assert(player_precarregar_fonte(0, 0, 0, 0, 6));
  int stops = nativeStops;
  player_precarregar_cancelar(5);
  assert(nativeStops == stops && player_precarregar_estado(6) == 1);

  // Resume uses the same local seconds/percentage rule as the normal player.
  resetFixture(); fixtureTitle.progresso = 50;
  fixtureProgress.posSeg = 50; fixtureProgress.durSeg = 100; hasProgress = 1;
  assert(player_precarregar_fonte(0, 0, 0, 0, 7));
  assert(nativeSeconds == 50 && nativePercent == 50 && !strcmp(lastProgressKey, "tt42"));
  resetFixture(); fixtureTitle.progresso = 50; hasProgress = 0;
  assert(player_precarregar_fonte(0, 0, 0, 0, 8));
  assert(nativeSeconds == 0 && nativePercent == 50);

  // An episode's position never leaks into a different episode's preload.
  resetFixture(); strcpy(fixtureTitle.tipo, "series");
  fixtureTitle.progresso = 50; fixtureTitle.temporada = 1; fixtureTitle.episodio = 2;
  assert(player_precarregar_fonte(0, 1, 3, 0, 9));
  assert(nativeSeconds == 0 && nativePercent == 0);

  // Existing fullscreen/mini/retained playback remains untouched.
  resetFixture(); mini = 1;
  assert(!player_precarregar_fonte(0, 0, 0, 0, 10) && nativeStarts == 0);
  mini = 0; retido = 1;
  assert(!player_precarregar_fonte(0, 0, 0, 0, 11) && nativeStarts == 0);
  // The separate Off option stops speculation even when helpers are called.
  resetFixture(); preloadEnabled = 0;
  assert(!player_precarregar_fonte(0, 0, 0, 0, 12) && nativeStarts == 0);
  // Next-episode sample preload leaves current subtitles and playback alone.
  resetFixture(); mkvFixture(); aberto = comVideo = 1;
  assert(player_precarregar_proxima_fonte(0, 1, 3, &fixtureStream, 13));
  assert(nextNativeStarts == 1 && subtitleStarts == 0 && subtitleStops == 0 && aberto && comVideo);
  unsigned tokenA = player_precarregar_novo_token(), tokenB = player_precarregar_novo_token();
  assert(tokenA > 0 && tokenB > tokenA && tokenB <= 0x7fffffffu);
  // Explicit beginning or a changed subtitle language must discard the old window.
  resetFixture(); mkvFixture(); fixtureTitle.progresso = 50;
  assert(player_precarregar_fonte(0, 0, 0, 0, 14));
  player_precarregar_adotar(14); semRetomada = 1; retomarPct = 0;
  assert(preTituloLegendaUsar(fixtureStream.url) == 0);
  assert(subtitleStops == 1 && player_precarregar_estado(14) == 0);
  resetFixture(); mkvFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 15));
  player_precarregar_adotar(15); strcpy(subtitlePref, "fr");
  assert(preTituloLegendaUsar(fixtureStream.url) == 0 && subtitleStops == 1);

  // A changed seconds checkpoint cannot adopt subtitle bytes from the old position.
  resetFixture(); mkvFixture(); fixtureTitle.progresso = 50;
  fixtureProgress.posSeg = 50; fixtureProgress.durSeg = 100; hasProgress = 1;
  assert(player_precarregar_fonte(0, 0, 0, 0, 16));
  player_precarregar_adotar(16); retomarFonteSeg = 55;
  assert(preTituloLegendaUsar(fixtureStream.url) == 0 && subtitleStops == 1);

  // Source mismatch cancels preparation; native failure never becomes "ready".
  resetFixture(); mkvFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 17));
  player_precarregar_adotar(17);
  assert(preTituloLegendaUsar("https://fixture.invalid/other.mkv") == 0 && subtitleStops == 1);
  resetFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 18)); nativeState = -1;
  assert(player_precarregar_estado(18) == -1);
  player_precarregar_cancelar(18); assert(player_precarregar_estado(18) == 0);

  // Canceling the next source leaves the current subtitle worker running.
  resetFixture(); mkvFixture(); aberto = comVideo = 1; subtitlePhase = 1;
  assert(player_precarregar_proxima_fonte(0, 1, 3, &fixtureStream, 19));
  player_precarregar_cancelar(19);
  assert(subtitlePhase == 1 && subtitleStops == 0 && aberto && comVideo);

  // Both title and next use one generator; its signed-JNI boundary skips zero.
  atomic_store_explicit(&preTituloTokens, 0x7ffffffeu, memory_order_relaxed);
  unsigned boundary = player_precarregar_novo_token();
  unsigned wrapped = player_precarregar_novo_token();
  unsigned following = player_precarregar_novo_token();
  assert(boundary == 0x7fffffffu && wrapped == 1 && following == 2);
  // Real close of an adopted, source-waiting player must discard its native token.
  resetFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 20));
  player_precarregar_adotar(20); aberto = esperandoFonte = 1;
  stops = nativeStops;
  fecharSessao(0);
  assert(nativeStops == stops + 1 && nativeToken == 0 && nativeAdoption == 0);
  assert(!aberto && !comVideo && visibleStops == 0);

  // MKV wait transfers ownership out of preTitulo, but Stop must still cancel adoption.
  resetFixture(); mkvFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 21));
  player_precarregar_adotar(21); aberto = 1;
  assert(preTituloLegendaUsar(fixtureStream.url) == 1 && preTitulo.adotado == 0);
  assert(prebuscaUrl[0] && nativeAdoption == 21);
  fecharSessao(0);
  assert(nativeAdoption == 0 && !prebuscaUrl[0] && subtitleStops > 0);

  // Closing the outgoing visible episode keeps the already adopted Next buffer.
  resetFixture(); aberto = comVideo = 1;
  assert(player_precarregar_proxima_fonte(0, 1, 3, &fixtureStream, 22));
  player_precarregar_adotar(22); stops = nativeStops;
  fecharSessao(0);
  assert(visibleStops == 1 && nativeStops == stops && nativeToken == 22 && nativeAdoption == 22);
  // If that new player is stopped before its source opens, ownership ends there.
  aberto = esperandoFonte = 1;
  fecharSessao(0);
  assert(nativeToken == 0 && nativeAdoption == 0);

  // An unadopted title buffer is not canceled by an ordinary initial close.
  resetFixture();
  assert(player_precarregar_fonte(0, 0, 0, 0, 23)); stops = nativeStops;
  fecharSessao(0);
  assert(nativeStops == stops && nativeToken == 23 && preTitulo.adotado == 0);
  puts("preload-player: background buffering, subtitle timing, cancellation and resume PASS");
  return 0;
}
