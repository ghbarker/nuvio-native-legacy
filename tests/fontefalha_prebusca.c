// Logs18: one malformed container must not reject the following candidates
// before their subtitle prefetch has handed a URL to the new video session.
// The included functions are copied verbatim from production by the runner.
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "fontefalha.h"
#include "inicio.h"
#include "linguas.h"
#include "video_reconexao.h"
typedef unsigned Uint32;
typedef int32_t Sint32;

// The Android stop boundary is real. JNI calls are inert transport boundaries.
typedef void JNIEnv;
static unsigned preparoAdotar;
static int ativo, prontoLoad, primeiroQuadro, tocando, superficieEstavel;
static int pausaPedida, pausaVista, reconErroPend, reconErroVideo;
static int reconFaixasPend, reconBuscarMs, recarregarPausa, falhou;
static int mPararMantendoPreparo, mParar;
static NvReconexao recon;
static void novaRetomada(int p) { (void)p; }
static void capmkv_zerar(void) {}
static JNIEnv *ambiente(void) { return NULL; }
static void kInt(int method, int value) { (void)method; (void)value; }
static void kSemArg(int method) { (void)method; }
#define tocando backendTocando
static int backendTocando;
#include "backend.inc"
#undef tocando

// Player prefetch/UI service boundaries. Navigation and ownership stay real.
static int aberto, mini, retido, comVideo, esperandoFonte, erroFonte, pausaPessoa;
static int retomadaAplicada, inicioImagem, durFonteAvisada, pedProximaFonte;
static int durFonteEsperadaOk, duracaoReal, durCurtaAvisado, durPassouAvisado;
static int visivel, soBarra, erroBotao, semRetomada, retomadaNaPreparacao;
static int memQuadroPendente;
static unsigned durFonteAcaoAte, toastAte, prebuscaDesde;
static unsigned loadEm;
static char erroTitulo[80], erroDica[80], prebuscaUrl[4096];
static double retomarPct, retomarSeg, retomarFonteSeg;
static unsigned agora;
static int prefetchEnabled, opens, syncFailure, prefetchPhase;
static int adoptAllowed, adoptions, cancelled, nativeCancelled, prefetchRequests;
// Only these fields are read by the extracted player/watchdog functions.
// No value with this local shape crosses a production module boundary.
typedef struct {
  uint64_t badges;
  char url[4096], arquivo[512], descricao[2048], rotulo[192];
  int mp4;
} Stream;
static Stream source;
static int haveSource, current;
static int stream_atual(void) { return current; }
static const Stream *stream_item(int i) { (void)i; return haveSource ? &source : NULL; }
static int ehCanal(void) { return 0; }
static const char *i18n(const char *text) { return text; }
static void marco(const char *text) { (void)text; }
static Uint32 SDL_GetTicks(void) { return agora; }
static int dvtela_ativa(void) { return 0; }
static void dvtela_sair(int reason, unsigned now) { (void)reason; (void)now; }
static unsigned dvRelogio(void) { return agora; }
static void video_dv_tela(int enabled) { (void)enabled; }
static void mkvass_parar(void) { cancelled++; }
static void mkvass_video_aberto(int enabled) { (void)enabled; }
static void dvTelaFonte(const char *url) { (void)url; }
static int escolherLegendaPrebusca(const char *const *languages, int n) { (void)languages; (void)n; return 0; }
static int mkvass_prebuscar(const char *url, int (*select)(const char *const *, int), double pct) {
  (void)url; (void)select; (void)pct; prefetchRequests++; return prefetchEnabled;
}
typedef struct MkvassPreparo MkvassPreparo;
static void mkvass_preparo_cancelar(MkvassPreparo **p) { *p = NULL; cancelled++; }
static int mkvass_preparo_adotar(MkvassPreparo **p, const char *url, double pct) {
  (void)url; (void)pct; adoptions++;
  if (!adoptAllowed) return 0;
  *p = NULL; return 1;
}
static int mkvass_prebusca_fase(void) { return prefetchPhase; }
static void mkvass_estatisticas(long *requests, long *bytes, int *collected, int *total) {
  *requests = *bytes = 0; *collected = *total = 0;
}
static void video_precarregar_cancelar(unsigned token) { (void)token; nativeCancelled++; }
static void video_definir_reconexao(int enabled) { (void)enabled; }
static void video_definir_modo_live(int mode) { (void)mode; }
static int ajustes_livetv_modo(void) { return 0; }
static int ajustes_cache_seek_mb(void) { return 0; }
static int ajustes_buffer_preset(void) { return 0; }
static void cacheboost_backend_cache(int mb) { (void)mb; }
static void cacheboost_backend_buffer(int preset) { (void)preset; }
static void memlog_evento(const char *event) { (void)event; }
static const char *proxyts_resolver(const char *url, char *buffer, unsigned n) {
  (void)buffer; (void)n; return url;
}
static int video_tocar_retomada(const char *url, double seconds, double pct) {
  (void)url; (void)seconds; (void)pct;
  // The separate whole-backend JNI test verifies this backend contract.
  opens++; falhou = 0; ativo = 1; return !syncFailure;
}
static void zerarAssistido(void) {}
static void aspArmar(void) {}
typedef struct { float x, y, w, h; } PlrRect;
static PlrRect miniDestino(void) { return (PlrRect){0}; }
static void video_janela(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
static void aplicarAspecto(void) {}
static int video_registro_negado(void) { return 0; }
#define MKVASS_PREBUSCA_MS 4000
#define DVT_SAIDA_FONTE 1
#include "prepared.inc"
#include "player.inc"
#include "prefetch-update.inc"

static FonteFalhaSessao *fonteFalha;
static int fonteVODAutomatica, fonteVODTentativas, aguardandoFonte;
static unsigned fonteVODDesde;
static double fonteFalhaPos;
static int requests, probes, decoderErrors, marked, siblings, leave, reconnecting;
static int nextLegacy, loading, sourcePlayed, ready, decoderCode, firstFrame;
static double buffered;
#define VOD_FONTE_MAX_TENTATIVAS 10
#define VOD_FONTE_BUFFER_MS 30000
static const char *player_id_canal(void) { return ""; }
static int player_aberto(void) { return aberto; }
static int player_quer_sair(void) { return leave; }
static int player_carregando(void) { return loading; }
static int video_reconectando(void) { return reconnecting; }
static int video_fonte_tocou(void) { return sourcePlayed; }
static int video_dv_ativo(void) { return 0; }
static int video_pronto(void) { return ready; }
static int video_quadro_visto(void) { return firstFrame; }
static int video_n_audio(void) { return 0; }
static int video_n_legenda(void) { return 0; }
static double video_buffer_fim(void) { return buffered; }
static unsigned video_bufferando_ms(void) { return 0; }
static int video_erro_decoder_codigo(void) { return falhou ? decoderCode : 0; }
static void sondarAposRecusa(void) { probes++; }
static int stream_n(void) { return 4; }
static uint64_t badges_bit(const char *name) {
  return !strcmp(name, "r-4k") ? 1 : !strcmp(name, "v-dv") ? 2 : 0;
}
static int stream_proxima_sem_perda(int i) { (void)i; return 1; }
static void stream_automatico_erro_decoder(int i, int code, int tried) { (void)i; (void)code; (void)tried; decoderErrors++; }
static void guardarPosicaoFonteFalha(void) {}
static void player_definir_retomada_fonte(double pos) { (void)pos; }
static int pedirProximaFonteVOD(int i) { (void)i; requests++; aguardandoFonte = 2; return 1; }
static void player_toast(const char *text, unsigned ms) { (void)text; (void)ms; }
static void player_definir_tentativa(int n, int max) { (void)n; (void)max; }
static void marcarFonteMorta(int i) { (void)i; marked++; }
static int stream_automatico_excluir_irmas(int i) { (void)i; siblings++; return 0; }
static int stream_proxima_vod(int i) { (void)i; return nextLegacy; }
#include "watchdog.inc"

static void reset(void) {
  if (fonteFalha) fontefalha_sessao_liberar(fonteFalha);
  fonteFalha = NULL; aberto = loading = fonteVODAutomatica = 1;
  mini = retido = comVideo = esperandoFonte = erroFonte = falhou = leave = reconnecting = 0;
  sourcePlayed = ready = firstFrame = requests = probes = decoderErrors = marked = siblings = opens = 0;
  buffered = 0;
  aguardandoFonte = syncFailure = 0; prefetchEnabled = prefetchPhase = adoptAllowed = 1;
  adoptions = cancelled = nativeCancelled = prefetchRequests = 0;
  retomarPct = retomarSeg = 0; retomarFonteSeg = -1;
  haveSource = 0; memset(&source, 0, sizeof source);
  memset(&preTitulo, 0, sizeof preTitulo);
  ling_local_legenda("en"); decoderCode = 3001;
  nextLegacy = 1; agora = 1000; fonteVODDesde = 999; fonteVODTentativas = 1;
  fonteFalhaPos = -1; prebuscaUrl[0] = 0;
}
static void delayed(void) {
  falhou = 1; comVideo = 1; ativo = 1;
  player_voltar_a_esperar();
  assert(video_falhou() && !player_tem_video()); // Android leaves last error latched.
  player_definir_fonte("prefetch.mkv");
  assert(esperandoFonte && prebuscaUrl[0] && !player_tem_video() && opens == 0);
  for (int i = 0; i < 4; i++) { agora += 400; tentarProximaFonteVOD(); }
  assert(requests == 0 && probes == 0 && decoderErrors == 0);
  assert(esperandoFonte && prebuscaUrl[0]);
}

static void prepared(const char *url) {
  preTitulo.token = 42;
  preTitulo.adotado = preTitulo.legenda = preTitulo.nativo = 1;
  preTitulo.mkv = (MkvassPreparo *)(uintptr_t)1;
  preTitulo.desde = agora;
  snprintf(preTitulo.url, sizeof preTitulo.url, "%s", url);
  snprintf(preTitulo.idioma, sizeof preTitulo.idioma, "en");
}

static void preparedLifecycle(void) {
  // Adopt the actual pending subtitle context. Neither the old backend error
  // nor a ready native preload makes the new URL an owned foreground video.
  reset(); falhou = 1; prepared("adopted.mkv");
  player_definir_fonte("adopted.mkv");
  assert(adoptions == 1 && !preTitulo.token && !preTitulo.mkv);
  assert(esperandoFonte && prebuscaUrl[0] && opens == 0 && !player_tem_video());
  assert(prefetchRequests == 0); // Adopted ranges are not requested again.
  tentarProximaFonteVOD(); assert(requests == 0 && probes == 0);
  atualizarPrebusca(agora + MKVASS_PREBUSCA_MS - 1); assert(opens == 0);
  prefetchPhase = 2; atualizarPrebusca(agora);
  assert(opens == 1 && player_tem_video() && !video_falhou() && !esperandoFonte);
  tentarProximaFonteVOD(); assert(requests == 0);

  // A still-running subtitle reader releases the video exactly at its cap.
  reset(); falhou = 1; prepared("cap.mkv"); player_definir_fonte("cap.mkv");
  atualizarPrebusca(agora + MKVASS_PREBUSCA_MS - 1); assert(opens == 0);
  atualizarPrebusca(agora + MKVASS_PREBUSCA_MS);
  assert(opens == 1 && !prebuscaUrl[0] && !esperandoFonte && player_tem_video());

  // The real closed-player guard discards a pending handoff. Reopening cannot
  // start the abandoned URL after its subtitle worker eventually completes.
  reset(); prepared("closed.mkv"); player_definir_fonte("closed.mkv");
  aberto = 0; prefetchPhase = 2; atualizarPrebusca(agora);
  assert(!prebuscaUrl[0] && opens == 0);
  aberto = 1; atualizarPrebusca(agora); assert(opens == 0);

  // Changing the URL or resume position invalidates the prepared context and
  // starts ordinary subtitle preparation for this new opening, without play.
  reset(); prepared("old.mkv"); player_definir_fonte("new.mkv");
  assert(nativeCancelled == 1 && cancelled == 1 && !preTitulo.token);
  assert(!strcmp(prebuscaUrl, "new.mkv") && prefetchRequests == 1 && opens == 0);
  reset(); prepared("resume.mkv"); retomarFonteSeg = 50;
  player_definir_fonte("resume.mkv");
  assert(nativeCancelled == 1 && adoptions == 0 && prefetchRequests == 1 && opens == 0);

  // A service refusing context adoption cancels that token and falls back to
  // a fresh subtitle reader; it must not expose the stale backend failure.
  reset(); prepared("refused.mkv"); adoptAllowed = 0; falhou = 1;
  player_definir_fonte("refused.mkv");
  assert(adoptions == 1 && cancelled == 1 && nativeCancelled == 1);
  assert(prefetchRequests == 1 && esperandoFonte && !player_tem_video());
  tentarProximaFonteVOD(); assert(requests == 0 && probes == 0);
  puts("PASS: prepared MKV adoption, release, timeout, close and cancellation");
}

static void deadlineAndErrorCode(void) {
  const unsigned deadlines[] = {INICIO_ABRE_SEM_SINAL_MS,
      INICIO_ABRE_COM_DADO_MS, INICIO_ABRE_PESADO_MS};
  for (unsigned i = 0; i < sizeof deadlines / sizeof *deadlines; i++) {
    reset(); falhou = 1; fonteVODDesde = agora;
    if (i == 1) buffered = 2.0;
    if (i == 2) { haveSource = 1; source.badges = badges_bit("r-4k"); }
    agora = fonteVODDesde + deadlines[i] - 1;
    tentarProximaFonteVOD(); assert(requests == 0 && probes == 0);
    agora++; tentarProximaFonteVOD(); assert(requests == 0 && probes == 0);
    agora++; tentarProximaFonteVOD(); assert(requests == 1 && probes == 0);
  }
  reset(); sourcePlayed = 1; agora += INICIO_ABRE_PESADO_MS + 1;
  tentarProximaFonteVOD(); assert(requests == 0);
  reset(); firstFrame = 1; agora += INICIO_ABRE_PESADO_MS + 1;
  tentarProximaFonteVOD(); assert(requests == 0);
  // A fresh foreground failure is actionable even when there is no decoder
  // code; a stale code-zero failure while preparing subtitles is not.
  reset(); prefetchEnabled = 0; player_definir_fonte("code-zero.mp4");
  decoderCode = 0; falhou = 1; tentarProximaFonteVOD();
  assert(requests == 1 && probes == 1 && decoderErrors == 1);
  reset(); decoderCode = 0; delayed();
  puts("PASS: exact 15/30/60s startup deadlines, played/frame exemption and code zero");
}

static void filtersBeforeBudget(void) {
  FonteFalhaConfig cfg = {0}; cfg.provedores = cfg.addons = 1;
  const int order[4] = {1, 2, 0, 3};
  for (int i = 0; i < 4; i++) { cfg.ordem[i] = order[i]; cfg.limites[i] = 3; }
  cfg.filtro.excluir[FONTEFILTRO_RESOLUCAO] =
      fontefiltro_valores(FONTEFILTRO_RESOLUCAO, "2160p");
  cfg.filtro.maxGB = 5;
  strcpy(cfg.filtro.grupos, "BLOCKED");
  FonteFalhaCandidata candidates[6] = {{0}};
  const char *identities[] = {"excluded-resolution", "excluded-size", "excluded-group",
                            "allowed-a", "allowed-b", "allowed-c"};
  for (int i = 0; i < 6; i++) {
    candidates[i].indice = i; candidates[i].nativa = 1;
    candidates[i].identidade = identities[i]; candidates[i].ordemAddon = i % 2;
    strcpy(candidates[i].addon, i % 2 ? "addon-b" : "addon-a");
    FonteFiltroEntrada metadata = {0}; metadata.resolucao = i == 0 ? "2160p" : "1080p";
    metadata.tamanhoBytes = i == 1 ? UINT64_C(6000000000) : UINT64_C(1000000000);
    metadata.grupo = i == 2 ? "BLOCKED" : "ALLOWED";
    fontefiltro_classificar(&metadata, &candidates[i].filtro);
  }
  FonteFalhaSessao *session = fontefalha_sessao_nova(&cfg, (1u << 1) | (1u << 2), -1);
  assert(session);
  // Use the actual classifier and planner. Exclusions cannot consume any
  // native-provider/addon slots, even when listed before all eligible links.
  unsigned seen = 0;
  for (int attempt = 0; attempt < 4; attempt++) {
    FonteFalhaRota route;
    assert(fontefalha_proxima(session, candidates, 6, 1, &route) == FONTEFALHA_PROXIMA);
    assert(route.indice >= 3 && route.indice <= 5);
    assert(route.servico == (attempt < 3 ? 1 : 2));
    if (attempt < 3) { assert(!(seen & (1u << route.indice))); seen |= 1u << route.indice; }
    assert(fontefalha_tentar(session, &candidates[route.candidata], route.servico));
    assert(fontefalha_tentativas(session) == attempt + 1);
    fontefalha_marcar_aberta(session);
  }
  assert(seen == ((1u << 3) | (1u << 4) | (1u << 5)));
  fontefalha_sessao_liberar(session);
  puts("PASS: resolution/size/release-group exclusions precede the provider attempt budget");
}
int main(void) {
  reset(); delayed();
  // The real policy must spend three DISTINCT TorBox links before Premiumize.
  FonteFalhaConfig cfg = {0}; cfg.provedores = 1;
  int order[4] = {1,2,0,3};
  for (int i = 0; i < 4; i++) { cfg.ordem[i] = order[i]; cfg.limites[i] = 3; }
  fonteFalha = fontefalha_sessao_nova(&cfg, (1u << 1) | (1u << 2), -1); assert(fonteFalha);
  FonteFalhaCandidata candidates[4] = {{0}};
  const char *identity[4] = {"a:0", "b:0", "c:0", "d:0"};
  unsigned torboxIndices = 0;
  for (int i = 0; i < 4; i++) {
    candidates[i].indice = i; candidates[i].nativa = 1;
    candidates[i].identidade = identity[i]; candidates[i].ordemAddon = i % 2;
    strcpy(candidates[i].addon, i % 2 ? "addon-b" : "addon-a");
  }
  for (int i = 0; i < 4; i++) {
    FonteFalhaRota route;
    assert(fontefalha_proxima(fonteFalha, candidates, 4, 1, &route) == FONTEFALHA_PROXIMA);
    assert(route.servico == (i < 3 ? 1 : 2));
    if (i < 3) {
      assert(!(torboxIndices & (1u << route.indice)));
      torboxIndices |= 1u << route.indice;
    }
    assert(fontefalha_tentar(fonteFalha, &candidates[route.candidata], route.servico));
    fontefalha_marcar_aberta(fonteFalha); current = route.indice;
    int attempts = fontefalha_tentativas(fonteFalha);
    requests = probes = decoderErrors = opens = 0; aguardandoFonte = 0; fonteVODDesde = agora;
    delayed();
    assert(fontefalha_tentativas(fonteFalha) == attempts);
    // Execute the production update fragment that hands the saved URL to
    // tocarFonte once subtitle preparation finishes.
    prefetchPhase = 2; atualizarPrebusca(agora);
    assert(player_tem_video() && !video_falhou());
    falhou = 1; tentarProximaFonteVOD();
    assert(requests == 1 && probes == 1 && decoderErrors == 1);
    tentarProximaFonteVOD(); assert(requests == 1);
  }
  reset(); delayed(); leave = 1; falhou = 1; comVideo = 1;
  tentarProximaFonteVOD(); assert(requests == 0); // Stop/Back remains cancellation.
  reset(); prefetchEnabled = 0; player_definir_fonte("direct.mp4");
  falhou = 1; tentarProximaFonteVOD(); assert(requests == 1 && marked == 1);
  reset();
  preTitulo.adotado = preTitulo.legenda = 1;
  strcpy(preTitulo.url, "prepared.mkv"); strcpy(preTitulo.idioma, "en");
  prefetchPhase = 2; player_definir_fonte("prepared.mkv");
  falhou = 1; tentarProximaFonteVOD(); assert(opens == 1 && requests == 1);
  reset(); prefetchEnabled = 0; syncFailure = 1; player_definir_fonte("refused.mp4");
  assert(!player_tem_video() && player_fonte_falhou());
  tentarProximaFonteVOD(); assert(requests == 1); // Local synchronous refusal still advances.
  reset(); agora += INICIO_ABRE_COM_DADO_MS + 1;
  tentarProximaFonteVOD(); assert(requests == 1 && probes == 0); // Startup timeout retained.
  reset(); reconnecting = 1; comVideo = falhou = 1;
  tentarProximaFonteVOD(); assert(requests == 0); // In-provider reconnect owns its retries.
  reset(); fonteVODAutomatica = 0; comVideo = falhou = 1;
  tentarProximaFonteVOD(); assert(requests == 0); // Manual playback is not automatic failover.
  reset(); retido = comVideo = falhou = 1;
  tentarProximaFonteVOD(); assert(requests == 0); // Retained pipeline is not the foreground attempt.
  preparedLifecycle();
  deadlineAndErrorCode();
  filtersBeforeBudget();
  reset();
  puts("PASS: deferred prefetch owns no stale backend error; three TorBox links then Premiumize");
  return 0;
}
