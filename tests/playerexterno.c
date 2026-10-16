// Exercise the real handoff state machine, including its process-wide token
// allocator. No Android runtime or fake playback-completion callback is used.
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/playerexterno.c"

static const char fonte[] = "https://cdn.example/video.mkv?token=a%2Fb%2Bz&sig=A+B%3D&x=1&x=2#part";
static const char cabs[] = "User-Agent: Nuvio Test\nReferer: https://origin.example/a?x=1\nAuthorization: Bearer fixture\nCookie: a=b";

static void igual(const PlayerExterno *a, const PlayerExterno *b) {
  assert(!memcmp(a, b, sizeof *a));
}
static void recusar(PlayerExterno *p, const char *url, const char *headers) {
  PlayerExterno antes = *p;
  uint64_t seq = atomic_load(&sequencia);
  assert(playerexterno_pedir(p, url, headers, "Title", 12.5) == -1);
  igual(p, &antes);
  assert(atomic_load(&sequencia) == seq);
}
static void ignorar(PlayerExterno *p, uint64_t token, uint64_t sessao, int evento) {
  PlayerExterno antes = *p;
  assert(playerexterno_evento(p, token, sessao, evento) == PEX_NADA);
  igual(p, &antes);
}

static void interno(void) {
  const int modos[] = { INT_MIN, -1, PEX_INTERNO, PEX_PERGUNTAR + 1, INT_MAX };
  for (unsigned i = 0; i < sizeof modos / sizeof modos[0]; i++) {
    PlayerExterno p;
    playerexterno_iniciar(&p, modos[i], 41);
    assert(p.modo == PEX_INTERNO && p.decisao == PEX_INTERNO && p.sessao == 41);
    assert(!playerexterno_ativo(&p) && !p.token);
    PlayerExterno antes = p;
    uint64_t seq = atomic_load(&sequencia);
    // Internal playback does not inspect, reject or copy external-only inputs.
    assert(playerexterno_pedir(&p, NULL, NULL, NULL, NAN) == 0);
    assert(playerexterno_pedir(&p, "file:///private/video", cabs, "Name", INFINITY) == 0);
    igual(&p, &antes);
    assert(atomic_load(&sequencia) == seq);
    ignorar(&p, 0, 41, PEX_EV_ABRIU);
  }
  puts("PASS: Internal default and invalid modes leave external inputs and tokens untouched");
}

static void perguntarInterno(void) {
  PlayerExterno p;
  playerexterno_iniciar(&p, PEX_PERGUNTAR, 51);
  assert(p.modo == PEX_PERGUNTAR && p.decisao == -1);
  assert(playerexterno_pedir(&p, fonte, cabs, "Episode", 91.25) == 1);
  uint64_t token = p.token;
  assert(token && p.fase == PEX_ESCOLHA && playerexterno_ativo(&p));
  ignorar(&p, token + 1, 51, PEX_EV_INTERNO);
  ignorar(&p, token, 51, PEX_EV_ABRIU);
  ignorar(&p, token, 51, 0);
  ignorar(&p, token, 51, INT_MAX);
  assert(playerexterno_evento(&p, token, 51, PEX_EV_INTERNO) == PEX_CONTINUAR_INTERNO);
  assert(!playerexterno_ativo(&p) && p.decisao == PEX_INTERNO);
  ignorar(&p, token, 51, PEX_EV_INTERNO);
  ignorar(&p, token, 51, PEX_EV_EXTERNO);
  PlayerExterno antes = p;
  uint64_t seq = atomic_load(&sequencia);
  assert(playerexterno_pedir(&p, NULL, NULL, NULL, -1) == 0);
  igual(&p, &antes);
  assert(atomic_load(&sequencia) == seq);
  playerexterno_cancelar(&p);
  assert(p.decisao == PEX_INTERNO && !p.token && !playerexterno_ativo(&p));
  assert(playerexterno_pedir(&p, "invalid", NULL, NULL, NAN) == 0);
  // A genuinely new media context asks again, with a fresh process-wide token.
  playerexterno_iniciar(&p, PEX_PERGUNTAR, 51);
  assert(playerexterno_pedir(&p, fonte, NULL, NULL, 0) == 1 && p.token > token);
  playerexterno_cancelar(&p);
  puts("PASS: Ask Internal continues once and remembers the decision only for this context");
}

static void externo(void) {
  PlayerExterno p;
  char url[sizeof fonte], headers[sizeof cabs], titulo[] = "Original episode";
  strcpy(url, fonte); strcpy(headers, cabs);
  playerexterno_iniciar(&p, PEX_PERGUNTAR, 61);
  assert(playerexterno_pedir(&p, url, headers, titulo, 125.75) == 1);
  uint64_t token = p.token;
  memset(url, 'x', sizeof url - 1); memset(headers, 'y', sizeof headers - 1); titulo[0] = '!';
  assert(!strcmp(p.fonte.url, fonte) && !strcmp(p.fonte.cabecalhos, cabs));
  assert(!strcmp(p.fonte.titulo, "Original episode") && p.fonte.posicaoMs == 125750);
  assert(playerexterno_evento(&p, token, 61, PEX_EV_EXTERNO) == PEX_LANCAR_EXTERNO);
  assert(p.decisao == PEX_EXTERNO && p.fase == PEX_LIBERAR);
  recusar(&p, fonte, cabs); // duplicate Play cannot replace the pending snapshot
  ignorar(&p, token, 61, PEX_EV_EXTERNO);
  assert(playerexterno_evento(&p, token, 61, PEX_EV_ABRIU) == PEX_NADA);
  assert(p.fase == PEX_ABERTO && playerexterno_ativo(&p));
  ignorar(&p, token, 61, PEX_EV_ABRIU);
  ignorar(&p, token, 61, PEX_EV_INTERNO);
  PlayerExternoFonte copia = p.fonte;
  assert(playerexterno_evento(&p, token, 61, PEX_EV_VOLTOU) == PEX_SAIR);
  assert(!playerexterno_ativo(&p) && p.decisao == PEX_EXTERNO);
  assert(!memcmp(&p.fonte, &copia, sizeof copia)); // no invented playback progress
  ignorar(&p, token, 61, PEX_EV_VOLTOU);
  ignorar(&p, token, 61, PEX_EV_ERRO);
  // Retrying another link keeps the chosen external mode but changes ownership.
  assert(playerexterno_pedir(&p, "https://cdn.example/next?sig=unchanged%2F", NULL, NULL, 0) == 2);
  assert(p.token > token && p.fase == PEX_LIBERAR);
  ignorar(&p, token, 61, PEX_EV_ABRIU);
  ignorar(&p, token, 61, PEX_EV_VOLTOU);
  uint64_t novo = p.token;
  assert(playerexterno_evento(&p, novo, 62, PEX_EV_ABRIU) == PEX_SAIR);
  assert(!p.token && !playerexterno_ativo(&p) && !p.fonte.url[0]);
  puts("PASS: Ask External releases, starts and returns once with an immutable owned source");
}

static void cancelamentoErro(void) {
  const int eventos[] = { PEX_EV_CANCELADO, PEX_EV_ERRO, PEX_EV_VOLTOU };
  const int fases[] = { PEX_ESCOLHA, PEX_LIBERAR, PEX_ABERTO };
  for (unsigned i = 0; i < sizeof eventos / sizeof eventos[0]; i++) {
    for (unsigned j = 0; j < sizeof fases / sizeof fases[0]; j++) {
      PlayerExterno p;
      playerexterno_iniciar(&p, PEX_PERGUNTAR, 71);
      assert(playerexterno_pedir(&p, fonte, cabs, "Canceled", 4) == 1);
      uint64_t token = p.token;
      if (fases[j] != PEX_ESCOLHA)
        assert(playerexterno_evento(&p, token, 71, PEX_EV_EXTERNO) == PEX_LANCAR_EXTERNO);
      if (fases[j] == PEX_ABERTO)
        assert(playerexterno_evento(&p, token, 71, PEX_EV_ABRIU) == PEX_NADA);
      assert(p.fase == fases[j]);
      int resultado = playerexterno_evento(&p, token, 71, eventos[i]);
      assert(resultado == (eventos[i] == PEX_EV_ERRO ? PEX_FALHOU : PEX_SAIR));
      assert(!playerexterno_ativo(&p) && p.fonte.posicaoMs == 4000);
      ignorar(&p, token, 71, eventos[i]);
      assert(playerexterno_pedir(&p, fonte, cabs, "Retry", 0) == (j == 0 ? 1 : 2));
      assert(p.token > token);
      playerexterno_cancelar(&p);
      PlayerExternoFonte vazia = {0};
      assert(!p.token && !playerexterno_ativo(&p) && !memcmp(&p.fonte, &vazia, sizeof vazia));
      PlayerExterno antes = p;
      playerexterno_cancelar(&p); igual(&p, &antes);
    }
  }
  // A stale title/profile callback cannot affect the replacement request.
  PlayerExterno p;
  playerexterno_iniciar(&p, PEX_EXTERNO, 81);
  assert(playerexterno_pedir(&p, fonte, NULL, "Old", 0) == 2);
  uint64_t velho = p.token;
  playerexterno_iniciar(&p, PEX_EXTERNO, 82);
  assert(playerexterno_pedir(&p, fonte, NULL, "New", 0) == 2 && p.token > velho);
  ignorar(&p, velho, 81, PEX_EV_VOLTOU);
  ignorar(&p, 0, 82, PEX_EV_ERRO);
  assert(playerexterno_evento(&p, p.token, 81, PEX_EV_VOLTOU) == PEX_SAIR);
  assert(!p.token && !p.fonte.url[0] && !playerexterno_ativo(&p));
  puts("PASS: Cancel, return and error never imply completion, fallback or stale-profile mutation");
}

static void alternativaInterna(void) {
  // No compatible header-capable external player: only an explicit Internal
  // choice may resume the pipeline, even when the saved preference is External.
  const int modos[] = { PEX_EXTERNO, PEX_PERGUNTAR };
  for (unsigned i = 0; i < sizeof modos / sizeof modos[0]; i++) {
    PlayerExterno p;
    playerexterno_iniciar(&p, modos[i], 83);
    assert(playerexterno_pedir(&p, fonte, cabs, "Fallback", 61) == (i == 0 ? 2 : 1));
    uint64_t token = p.token;
    if (i == 1)
      assert(playerexterno_evento(&p, token, 83, PEX_EV_EXTERNO) == PEX_LANCAR_EXTERNO);
    assert(p.fase == PEX_LIBERAR);
    ignorar(&p, token + 1, 83, PEX_EV_INTERNO);
    assert(playerexterno_evento(&p, token, 83, PEX_EV_INTERNO) == PEX_CONTINUAR_INTERNO);
    assert(p.decisao == PEX_INTERNO && !playerexterno_ativo(&p));
    ignorar(&p, token, 83, PEX_EV_INTERNO);
    ignorar(&p, token, 83, PEX_EV_ABRIU);
    PlayerExterno antes = p;
    uint64_t seq = atomic_load(&sequencia);
    assert(playerexterno_pedir(&p, NULL, NULL, NULL, NAN) == 0);
    igual(&p, &antes); assert(atomic_load(&sequencia) == seq);
    // After the external activity actually starts, Internal is not a second
    // concurrent playback path. Return/cancel must finish that handoff first.
    playerexterno_iniciar(&p, modos[i], 84);
    assert(playerexterno_pedir(&p, fonte, cabs, "Started", 61) == (i == 0 ? 2 : 1));
    token = p.token;
    if (i == 1)
      assert(playerexterno_evento(&p, token, 84, PEX_EV_EXTERNO) == PEX_LANCAR_EXTERNO);
    assert(playerexterno_evento(&p, token, 84, PEX_EV_ABRIU) == PEX_NADA);
    assert(p.fase == PEX_ABERTO);
    ignorar(&p, token, 84, PEX_EV_INTERNO);
    assert(playerexterno_evento(&p, token, 84, PEX_EV_CANCELADO) == PEX_SAIR);
    assert(p.decisao == PEX_EXTERNO && !playerexterno_ativo(&p));
    playerexterno_cancelar(&p);
  }
  puts("PASS: Explicit Internal fallback is allowed before launch and refused after external start");
}

static void limites(void) {
  PlayerExterno p;
  playerexterno_iniciar(&p, PEX_EXTERNO, 0);
  const char *invalidas[] = { NULL, "", "http://", "https://", "https:///path", "https://?query", "https://#fragment",
    "https//cdn.example/a", "file:///private/a", "content://private/a", "intent://a", "ftp://cdn.example/a",
    " https://cdn.example/a", "https://cdn.example/a ", "https://cdn.example/a b", "https://cdn.example/a\tb",
    "https://cdn.example/a\rb", "https://cdn.example/a\nb", "https://cdn.example/a\001b", "https://cdn.example/a\177b" };
  for (unsigned i = 0; i < sizeof invalidas / sizeof invalidas[0]; i++) recusar(&p, invalidas[i], NULL);
  char url[sizeof p.fonte.url + 1], headers[sizeof p.fonte.cabecalhos + 1];
  memset(url, 'x', sizeof url); memcpy(url, "https://cdn.example/?signature=", 31);
  url[sizeof p.fonte.url - 1] = 0;
  memset(headers, 'h', sizeof headers); headers[sizeof p.fonte.cabecalhos - 1] = 0;
  assert(strlen(url) == sizeof p.fonte.url - 1 && strlen(headers) == sizeof p.fonte.cabecalhos - 1);
  assert(playerexterno_pedir(&p, url, headers, NULL, 0) == 2);
  assert(!strcmp(p.fonte.url, url) && !strcmp(p.fonte.cabecalhos, headers) && !p.fonte.titulo[0]);
  playerexterno_cancelar(&p);
  url[sizeof p.fonte.url - 1] = 'x'; url[sizeof p.fonte.url] = 0;
  recusar(&p, url, NULL);
  url[sizeof p.fonte.url - 1] = 0;
  headers[sizeof p.fonte.cabecalhos - 1] = 'h'; headers[sizeof p.fonte.cabecalhos] = 0;
  recusar(&p, url, headers);
  assert(playerexterno_pedir(&p, "HTTPS://cdn.example/a?x=%20&sig=%2f+%3D", NULL, NULL, 0) == 2);
  assert(!strcmp(p.fonte.url, "HTTPS://cdn.example/a?x=%20&sig=%2f+%3D") && !p.fonte.cabecalhos[0]);
  playerexterno_cancelar(&p);
  char titulo[sizeof p.fonte.titulo + 1]; memset(titulo, 't', sizeof titulo); titulo[sizeof titulo - 1] = 0;
  assert(playerexterno_pedir(&p, fonte, cabs, titulo, 0) == 2);
  assert(strlen(p.fonte.titulo) == sizeof p.fonte.titulo - 1 && !strcmp(p.fonte.url, fonte));
  playerexterno_cancelar(&p);
  char utf8[301];
  for (int i = 0; i < 100; ++i) memcpy(utf8 + i * 3, "\xe5\xad\x97", 3);
  utf8[300] = 0;
  assert(playerexterno_pedir(&p, fonte, cabs, utf8, 0) == 2);
  assert(strlen(p.fonte.titulo) == 255 && !memcmp(p.fonte.titulo, utf8, 255));
  playerexterno_cancelar(&p);
  // A four-byte character at the truncation boundary must disappear whole.
  memset(utf8, 'a', 254); memcpy(utf8 + 254, "\xf0\x9f\x8e\xac", 5);
  assert(playerexterno_pedir(&p, fonte, cabs, utf8, 0) == 2);
  assert(strlen(p.fonte.titulo) == 254 && !memcmp(p.fonte.titulo, utf8, 254));
  playerexterno_cancelar(&p);
  puts("PASS: Signed URLs and required headers copy exactly at capacity and reject overflow or unsafe schemes");
}

static void posicao(void) {
  const struct { double segundos; int64_t ms; } casos[] = {
    { NAN, 0 }, { INFINITY, 0 }, { -INFINITY, 0 }, { -1, 0 }, { 0, 0 },
    { 0.0009, 0 }, { 1.234, 1234 }, { 90.125, 90125 },
    { (double)INT_MAX / 1000.0, INT_MAX }, { (double)INT_MAX / 1000.0 + 0.001, 0 }, { DBL_MAX, 0 }
  };
  for (unsigned i = 0; i < sizeof casos / sizeof casos[0]; i++) {
    PlayerExterno p;
    playerexterno_iniciar(&p, PEX_EXTERNO, 91);
    assert(playerexterno_pedir(&p, fonte, NULL, NULL, casos[i].segundos) == 2);
    assert(p.fonte.posicaoMs == casos[i].ms && p.fonte.posicaoMs >= 0 && p.fonte.posicaoMs <= INT_MAX);
    playerexterno_cancelar(&p);
  }
  puts("PASS: Resume uses finite explicit milliseconds within Int bounds without percent inference");
}

static void esgotamento(void) {
  atomic_store(&sequencia, (uint64_t)INT64_MAX - 1);
  PlayerExterno p, outra;
  playerexterno_iniciar(&p, PEX_EXTERNO, 101);
  assert(playerexterno_pedir(&p, fonte, NULL, NULL, 0) == 2 && p.token == INT64_MAX);
  uint64_t ultimo = p.token;
  playerexterno_cancelar(&p);
  recusar(&p, fonte, NULL);
  playerexterno_iniciar(&outra, PEX_PERGUNTAR, 102);
  recusar(&outra, fonte, NULL);
  assert(!p.token && !outra.token && atomic_load(&sequencia) == ultimo);
  ignorar(&outra, ultimo, 101, PEX_EV_ABRIU);
  playerexterno_iniciar(&p, PEX_INTERNO, 103);
  assert(playerexterno_pedir(&p, NULL, NULL, NULL, NAN) == 0);
  assert(atomic_load(&sequencia) == ultimo);
  puts("PASS: Exhausted launch tokens are refused across contexts and never wrap or get reused");
}

int main(void) {
  interno(); perguntarInterno(); externo(); cancelamentoErro(); alternativaInterna(); limites(); posicao(); esgotamento();
  return 0;
}
