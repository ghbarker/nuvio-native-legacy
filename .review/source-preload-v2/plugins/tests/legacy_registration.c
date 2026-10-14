// Review-only additive linkage for original foreground-only plugins_fontes.c.
// The legacy test never asks for a captured query; real snapshot tests use
// their explicit, checked credential/TLS transport implementations.
#include "addons.h"
#include "debrid.h"
#include "perfis.h"
#include <string.h>
void addons_definir_origem_extra_preparo(OrigemExtra f, const OrigemPreparoOps *ops) { (void)f; (void)ops; }
void debrid_contexto_capturar(DebridContexto *c) { memset(c, 0, sizeof *c); }
void debrid_contexto_entrar(const DebridContexto *c) { (void)c; }
void debrid_contexto_sair(void) {}

// Legacy foreground recipes omit perfis.c and never switch account epochs.
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t g) { return g == 0; }
int perfis_sessao_travar(uint64_t g) { return g == 0; }
void perfis_sessao_destravar(void) {}
