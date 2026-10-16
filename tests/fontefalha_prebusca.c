// Logs18: one malformed container must not reject the following candidates
// before their subtitle prefetch has handed a URL to the new video session.
// The included functions are copied verbatim from production by the runner.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fontefalha.h"
#include "inicio.h"
#include "video_reconexao.h"
typedef unsigned Uint32;

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
static unsigned durFonteAcaoAte, toastAte, prebuscaDesde;
static char erroTitulo[80], erroDica[80], prebuscaUrl[4096];
static double retomarPct;
static unsigned agora;
static int prefetchEnabled, prepared, opens, syncFailure;
static Uint32 SDL_GetTicks(void) { return agora; }
static int dvtela_ativa(void) { return 0; }
static void dvtela_sair(int reason, unsigned now) { (void)reason; (void)now; }
static unsigned dvRelogio(void) { return agora; }
static void video_dv_tela(int enabled) { (void)enabled; }
static void mkvass_parar(void) {}
static void mkvass_video_aberto(int enabled) { (void)enabled; }
static void dvTelaFonte(const char *url) { (void)url; }
static int preTituloLegendaUsar(const char *url) { (void)url; return prepared; }
static int prebuscaCabe(const char *url) { (void)url; return prefetchEnabled; }
static int escolherLegendaPrebusca(const char *const *languages, int n) { (void)languages; (void)n; return 0; }
static int mkvass_prebuscar(const char *url, int (*select)(const char *const *, int), double pct) {
  (void)url; (void)select; (void)pct; return 1;
}
static void tocarFonte(const char *url) {
  (void)url; opens++; falhou = 0; ativo = 1; comVideo = !syncFailure;
  if (syncFailure) erroFonte = 1;
}
#define DVT_SAIDA_FONTE 1
#include "player.inc"

typedef struct { uint64_t badges; } Stream;
static FonteFalhaSessao *fonteFalha;
static int fonteVODAutomatica, fonteVODTentativas, aguardandoFonte;
static unsigned fonteVODDesde;
static double fonteFalhaPos;
static int requests, probes, decoderErrors, marked, siblings, leave, reconnecting;
static int current, nextLegacy, loading, sourcePlayed, ready;
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
static int video_quadro_visto(void) { return ready; }
static int video_n_audio(void) { return 0; }
static int video_n_legenda(void) { return 0; }
static double video_buffer_fim(void) { return 0; }
static unsigned video_bufferando_ms(void) { return 0; }
static int video_erro_decoder_codigo(void) { return falhou ? 3001 : 0; }
static void sondarAposRecusa(void) { probes++; }
static int stream_atual(void) { return current; }
static int stream_n(void) { return 4; }
static const Stream *stream_item(int i) { (void)i; return NULL; }
static uint64_t badges_bit(const char *name) { (void)name; return 0; }
static int stream_proxima_sem_perda(int i) { (void)i; return 1; }
static void stream_automatico_erro_decoder(int i, int code, int tried) { (void)i; (void)code; (void)tried; decoderErrors++; }
static void guardarPosicaoFonteFalha(void) {}
static void player_definir_retomada_fonte(double pos) { (void)pos; }
static int pedirProximaFonteVOD(int i) { (void)i; requests++; aguardandoFonte = 2; return 1; }
static void player_toast(const char *text, unsigned ms) { (void)text; (void)ms; }
static void player_definir_tentativa(int n, int max) { (void)n; (void)max; }
static const char *i18n(const char *text) { return text; }
static void marcarFonteMorta(int i) { (void)i; marked++; }
static int stream_automatico_excluir_irmas(int i) { (void)i; siblings++; return 0; }
static int stream_proxima_vod(int i) { (void)i; return nextLegacy; }
static void player_erro_fonte(void) { erroFonte = 1; }
static void player_erro_fonte_motivo(const char *text, const char *hint) { (void)text; (void)hint; erroFonte = 1; }
static void marco(const char *text) { (void)text; }
#include "watchdog.inc"

static void reset(void) {
  if (fonteFalha) fontefalha_sessao_liberar(fonteFalha);
  fonteFalha = NULL; aberto = loading = fonteVODAutomatica = 1;
  mini = retido = comVideo = esperandoFonte = erroFonte = falhou = leave = reconnecting = 0;
  sourcePlayed = ready = requests = probes = decoderErrors = marked = siblings = opens = 0;
  aguardandoFonte = prepared = syncFailure = 0; prefetchEnabled = 1;
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
    // Release prefetch at its service boundary; the actual watchdog now sees
    // a current-session decoder failure and queues exactly one next route.
    esperandoFonte = 0; prebuscaUrl[0] = 0; tocarFonte("released.mkv");
    assert(player_tem_video() && !video_falhou());
    falhou = 1; tentarProximaFonteVOD();
    assert(requests == 1 && probes == 1 && decoderErrors == 1);
    tentarProximaFonteVOD(); assert(requests == 1);
  }
  reset(); delayed(); leave = 1; falhou = 1; comVideo = 1;
  tentarProximaFonteVOD(); assert(requests == 0); // Stop/Back remains cancellation.
  reset(); prefetchEnabled = 0; player_definir_fonte("direct.mp4");
  falhou = 1; tentarProximaFonteVOD(); assert(requests == 1 && marked == 1);
  reset(); prepared = 2; player_definir_fonte("prepared.mkv");
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
  reset();
  puts("PASS: deferred prefetch owns no stale backend error; three TorBox links then Premiumize");
  return 0;
}
