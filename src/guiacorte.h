// Tetos do guia de canais e a CONTA DO QUE FICOU DE FORA.
//
// O guia guarda no maximo GUIACORTE_MAX_CANAL canais em GUIACORTE_MAX_CAT
// categorias, na ordem em que o servidor manda. Um provedor com milhares de
// canais era cortado sem aviso e a pessoa so via o comeco da lista (um usuario
// de Shield achou que o app so tinha Reino Unido e EUA). Aqui se conta quem
// entrou e quem nao, e por que, para a tela dizer "Mostrando N de M".
#ifndef NV_GUIACORTE_H
#define NV_GUIACORTE_H
#include <stdio.h>

// Android: grow channel storage on demand, up to a bounded 30,000 channels.
// Other TVs retain their existing 900-channel memory budget. Categories may
// use one slot per channel; no separate 48/128-category cutoff discards them.
#ifdef NV_ANDROID
#define GUIACORTE_MAX_CANAL 30000
#else
#define GUIACORTE_MAX_CANAL 900
#endif
#define GUIACORTE_MAX_CAT GUIACORTE_MAX_CANAL

typedef struct {
  int porCanais;   // perdidos porque o teto de canais encheu
  int porCats;     // perdidos porque o teto de categorias encheu
} GuiaCorte;

// Um canal chega: `nAtual` ja guardados, `catOk` = a categoria dele coube.
// Devolve 1 se entra; se nao, soma aos perdidos pelo motivo certo.
static inline int guiacorte_entra(GuiaCorte *k, int nAtual, int catOk) {
  if (nAtual >= GUIACORTE_MAX_CANAL) { k->porCanais++; return 0; }
  if (!catOk) { k->porCats++; return 0; }
  return 1;
}
// Canais que a fonte tinha mas o corte da fonte ja descartou (o Xtream conta o
// total que o servidor mandou alem do que cabia no pedido).
static inline void guiacorte_perdeu_canais(GuiaCorte *k, int n) { if (n > 0) k->porCanais += n; }

static inline int guiacorte_por_canais(const GuiaCorte *k) { return k->porCanais > 0; }
static inline int guiacorte_por_cats(const GuiaCorte *k)   { return k->porCats > 0; }
static inline int guiacorte_cortou(const GuiaCorte *k)     { return k->porCanais + k->porCats > 0; }
static inline int guiacorte_total(const GuiaCorte *k, int guardados) {
  return guardados + k->porCanais + k->porCats;
}
// Para o log: "canais", "categorias", "canais+categorias" ou "nenhum".
static inline const char *guiacorte_motivo(const GuiaCorte *k) {
  return k->porCanais && k->porCats ? "canais+categorias"
       : k->porCanais ? "canais" : k->porCats ? "categorias" : "nenhum";
}
// "Mostrando %d de %d canais ..." com o modelo ja traduzido (dois %d).
static inline void guiacorte_linha(const GuiaCorte *k, int guardados, const char *modelo,
                                   char *dst, size_t n) {
  snprintf(dst, n, modelo, guardados, guiacorte_total(k, guardados));
}
#endif
