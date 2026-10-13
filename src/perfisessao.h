// Optional linkage for the existing isolated module fixtures. Every complete
// app links perfis.c; fixtures that omit account infrastructure retain their
// original behaviour. Tests of profile ownership must link the real module.
#ifndef NV_PERFISESSAO_H
#define NV_PERFISESSAO_H
#include <stdint.h>
#if defined(__GNUC__) || defined(__clang__)
extern uint64_t perfis_sessao_geracao(void) __attribute__((weak));
extern int perfis_sessao_atual(uint64_t) __attribute__((weak));
extern int perfis_sessao_travar(uint64_t) __attribute__((weak));
extern void perfis_sessao_destravar(void) __attribute__((weak));
static inline uint64_t nv_perfil_sessao_ler(void) {
  return perfis_sessao_geracao ? perfis_sessao_geracao() : 0;
}
static inline int nv_perfil_sessao_atual(uint64_t g) {
  return perfis_sessao_atual ? perfis_sessao_atual(g) : g == 0;
}
static inline int nv_perfil_sessao_travar(uint64_t g) {
  return perfis_sessao_travar ? perfis_sessao_travar(g) : g == 0;
}
static inline void nv_perfil_sessao_destravar(void) {
  if (perfis_sessao_destravar) perfis_sessao_destravar();
}
#else
#include "perfis.h"
#define nv_perfil_sessao_ler perfis_sessao_geracao
#define nv_perfil_sessao_atual perfis_sessao_atual
#define nv_perfil_sessao_travar perfis_sessao_travar
#define nv_perfil_sessao_destravar perfis_sessao_destravar
#endif
typedef uint64_t NvPerfilSessao;
#endif
