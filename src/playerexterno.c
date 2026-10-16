#include "playerexterno.h"
#include <limits.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

// Shared across media sessions. Refuse exhaustion instead of reusing a token
// whose delayed Android callback could belong to a different title/profile.
static _Atomic uint64_t sequencia;
static uint64_t novoToken(void) {
  uint64_t anterior = atomic_load_explicit(&sequencia, memory_order_relaxed);
  do {
    if (anterior >= INT64_MAX) return 0;
  } while (!atomic_compare_exchange_weak_explicit(&sequencia, &anterior, anterior + 1,
             memory_order_relaxed, memory_order_relaxed));
  return anterior + 1;
}

void playerexterno_iniciar(PlayerExterno *p, int modo, uint64_t sessao) {
  memset(p, 0, sizeof *p);
  p->modo = modo >= PEX_INTERNO && modo <= PEX_PERGUNTAR ? modo : PEX_INTERNO;
  p->decisao = p->modo == PEX_PERGUNTAR ? -1 : p->modo;
  p->sessao = sessao;
}

int playerexterno_pedir(PlayerExterno *p, const char *url, const char *cabecalhos,
                       const char *titulo, double segundos) {
  if (p->decisao == PEX_INTERNO) return 0;
  if (p->fase != PEX_INATIVO || !url || !*url ||
      (strncasecmp(url, "https://", 8) && strncasecmp(url, "http://", 7)) ||
      strlen(url) >= sizeof p->fonte.url ||
      (cabecalhos && strlen(cabecalhos) >= sizeof p->fonte.cabecalhos)) return -1;
  const char *autoridade = strstr(url, "://") + 3;
  if (!*autoridade || *autoridade == '/' || *autoridade == '?' || *autoridade == '#') return -1;
  for (const unsigned char *s = (const unsigned char *)url; *s; s++)
    if (*s <= 32 || *s == 127) return -1;
  uint64_t token = novoToken();
  if (!token) return -1;
  snprintf(p->fonte.url, sizeof p->fonte.url, "%s", url);
  snprintf(p->fonte.cabecalhos, sizeof p->fonte.cabecalhos, "%s", cabecalhos ? cabecalhos : "");
  // An optional long title may shorten, but never split a UTF-8 character.
  titulo = titulo ? titulo : "";
  size_t tituloN = strlen(titulo);
  if (tituloN >= sizeof p->fonte.titulo) {
    tituloN = sizeof p->fonte.titulo - 1;
    while (tituloN && ((unsigned char)titulo[tituloN] & 0xc0) == 0x80) --tituloN;
  }
  memcpy(p->fonte.titulo, titulo, tituloN);
  p->fonte.titulo[tituloN] = 0;
  p->fonte.posicaoMs = isfinite(segundos) && segundos > 0 && segundos * 1000.0 <= INT_MAX
      ? (int64_t)(segundos * 1000.0) : 0;
  p->token = token;
  p->fase = p->decisao < 0 ? PEX_ESCOLHA : PEX_LIBERAR;
  return p->fase == PEX_ESCOLHA ? 1 : 2;
}

int playerexterno_evento(PlayerExterno *p, uint64_t token, uint64_t sessao, int evento) {
  if (!p->token || token != p->token || p->fase == PEX_INATIVO) return PEX_NADA;
  if (sessao != p->sessao) { playerexterno_cancelar(p); return PEX_SAIR; }
  // A missing/incompatible handler may offer an explicit Internal button.
  // This is still before launch; never start a second player after ABRIU.
  if (evento == PEX_EV_INTERNO && (p->fase == PEX_ESCOLHA || p->fase == PEX_LIBERAR)) {
    p->decisao = PEX_INTERNO; p->fase = PEX_INATIVO;
    return PEX_CONTINUAR_INTERNO;
  }
  if (evento == PEX_EV_EXTERNO && p->fase == PEX_ESCOLHA) {
    p->decisao = PEX_EXTERNO; p->fase = PEX_LIBERAR;
    return PEX_LANCAR_EXTERNO;
  }
  if (evento == PEX_EV_ABRIU && p->fase == PEX_LIBERAR) {
    p->fase = PEX_ABERTO; return PEX_NADA;
  }
  if (evento == PEX_EV_VOLTOU || evento == PEX_EV_CANCELADO || evento == PEX_EV_ERRO) {
    p->fase = PEX_INATIVO;
    return evento == PEX_EV_ERRO ? PEX_FALHOU : PEX_SAIR;
  }
  return PEX_NADA;
}

void playerexterno_cancelar(PlayerExterno *p) {
  p->token = 0; p->fase = PEX_INATIVO;
  memset(&p->fonte, 0, sizeof p->fonte);
}
int playerexterno_ativo(const PlayerExterno *p) { return p->fase != PEX_INATIVO; }
