// Owned external-player handoff. This state machine never writes playback
// progress: an Android activity result does not prove that a video played.
#ifndef NV_PLAYEREXTERNO_H
#define NV_PLAYEREXTERNO_H
#include <stdint.h>

enum { PEX_INTERNO, PEX_EXTERNO, PEX_PERGUNTAR };
enum { PEX_INATIVO, PEX_ESCOLHA, PEX_LIBERAR, PEX_ABERTO };
enum {
  PEX_EV_INTERNO = 1, PEX_EV_EXTERNO, PEX_EV_ABRIU,
  PEX_EV_VOLTOU, PEX_EV_CANCELADO, PEX_EV_ERRO
};
enum { PEX_NADA, PEX_CONTINUAR_INTERNO, PEX_LANCAR_EXTERNO, PEX_SAIR, PEX_FALHOU };

typedef struct {
  char url[4096], cabecalhos[2048], titulo[256];
  int64_t posicaoMs;
} PlayerExternoFonte;
typedef struct {
  uint64_t token, sessao;
  int modo, decisao, fase;
  PlayerExternoFonte fonte;
} PlayerExterno;

void playerexterno_iniciar(PlayerExterno *p, int modo, uint64_t sessao);
// 0 = internal, 1 = ask, 2 = release internal resources then launch, -1 = error.
// Required URLs/headers never truncate; the signed URL is copied unchanged.
int playerexterno_pedir(PlayerExterno *p, const char *url, const char *cabecalhos,
                       const char *titulo, double segundos);
int playerexterno_evento(PlayerExterno *p, uint64_t token, uint64_t sessao, int evento);
void playerexterno_cancelar(PlayerExterno *p);
int playerexterno_ativo(const PlayerExterno *p);
#endif
