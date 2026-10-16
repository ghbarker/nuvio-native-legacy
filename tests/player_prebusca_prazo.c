/* Exercise the complete prefetch-release block extracted from player.c.
 * The worker phase and pipeline are boundaries; the deadline and ownership
 * decisions come verbatim from the production update path. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "player_prebusca_constante.inc"

typedef uint32_t Uint32;
typedef int32_t Sint32;
static char prebuscaUrl[4096];
static Uint32 prebuscaDesde;
static int esperandoFonte;
static int faseDoFio, aberturas, verificacoes, falhas;
static char aberta[4096], ultimoLog[1024];

static int mkvass_prebusca_fase(void) { return faseDoFio; }
static void mkvass_estatisticas(long *ped, long *bytes, int *col, int *tot) {
  *ped = 2; *bytes = 2048; *col = 1; *tot = 2;
}
static void tocarFonte(const char *url) {
  aberturas++;
  snprintf(aberta, sizeof aberta, "%s", url);
}
static int capturarLog(const char *formato, ...) {
  int n;
  va_list ap;
  va_start(ap, formato);
  n = vsnprintf(ultimoLog, sizeof ultimoLog, formato, ap);
  va_end(ap);
  return n;
}
static void atualizar(Uint32 agora) {
#define printf capturarLog
#include "player_prebusca_prazo.inc"
#undef printf
}
static void preparar(Uint32 desde, int fase, int temUrl) {
  snprintf(prebuscaUrl, sizeof prebuscaUrl, "%s", temUrl ? "https://fixture.invalid/video.mkv" : "");
  prebuscaDesde = desde;
  esperandoFonte = 1;
  faseDoFio = fase;
  aberturas = 0;
  aberta[0] = ultimoLog[0] = 0;
}
static void conferir(int ok, const char *nome) {
  verificacoes++;
  if (!ok) {
    falhas++;
    fprintf(stderr, "not ok  %s (open=%d waiting=%d pending=%d log=%s)\n",
            nome, aberturas, esperandoFonte, !!prebuscaUrl[0], ultimoLog);
  } else {
    printf("ok  %s\n", nome);
  }
}
static int retida(void) {
  return aberturas == 0 && esperandoFonte == 1 && prebuscaUrl[0] && !aberta[0];
}
static int solta(void) {
  return aberturas == 1 && esperandoFonte == 0 && !prebuscaUrl[0] &&
         !strcmp(aberta, "https://fixture.invalid/video.mkv");
}
int main(void) {
  preparar(1001u, 1, 1);
  atualizar(1000u);
  conferir(retida(), "same-frame timestamp 1ms before prefetch must keep the source pending");
  preparar(1002u, 1, 1);
  atualizar(1000u);
  conferir(retida(), "same-frame timestamp 2ms before prefetch must not expire its deadline");
  preparar(UINT32_MAX, 1, 1);
  atualizar(UINT32_MAX - 1u);
  conferir(retida(), "stale frame just before the tick counter wraps also remains pending");
  preparar(1000u, 1, 1);
  atualizar(1000u);
  conferir(retida(), "new prefetch remains pending at zero elapsed time");
  atualizar(1000u + MKVASS_PREBUSCA_MS - 1u);
  conferir(retida(), "active worker keeps its complete prefetch budget");
  atualizar(1000u + MKVASS_PREBUSCA_MS);
  conferir(solta(), "active worker releases exactly at its deadline");
  atualizar(1000u + MKVASS_PREBUSCA_MS + 1u);
  conferir(solta(), "released URL reaches the pipeline only once");
  preparar(UINT32_MAX - 1000u, 1, 1);
  atualizar(0u);
  conferir(retida(), "real tick-counter wrap preserves a small positive elapsed time");
  preparar(UINT32_MAX - (MKVASS_PREBUSCA_MS - 2u), 1, 1);
  atualizar(0u);
  conferir(retida(), "real wrap still holds one millisecond before deadline");
  preparar(UINT32_MAX - (MKVASS_PREBUSCA_MS - 1u), 1, 1);
  atualizar(0u);
  conferir(solta(), "real wrap releases at the correct positive deadline");
  preparar(1002u, 2, 1);
  atualizar(1000u);
  conferir(solta(), "completed worker releases immediately even with a stale frame time");
  preparar(1002u, 0, 1);
  atualizar(1000u);
  conferir(solta(), "worker with nothing to collect retains its immediate fallback");
  preparar(1000u, 2, 0);
  atualizar(1000u + MKVASS_PREBUSCA_MS);
  conferir(aberturas == 0 && !prebuscaUrl[0], "cancelled or absent pending source cannot reopen the pipeline");
  preparar(2001u, 1, 1);
  atualizar(2000u);
  conferir(retida(), "stale first frame retains URL ownership");
  atualizar(2001u);
  conferir(retida(), "following current frame starts at zero without losing the URL");
  atualizar(2001u + MKVASS_PREBUSCA_MS - 1u);
  conferir(retida(), "following frames retain the original deadline");
  atualizar(2001u + MKVASS_PREBUSCA_MS);
  conferir(solta(), "same source opens once after the full budget following stale input");
  printf("player_prebusca_prazo: %d checks, %d failed\n", verificacoes, falhas);
  return falhas ? 1 : 0;
}
