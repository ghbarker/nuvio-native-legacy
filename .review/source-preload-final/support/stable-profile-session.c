// Review-only stable epoch for reduced source/worker fixtures that omit perfis.c.
#include "perfis.h"
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t epoch) { return epoch == 0; }
int perfis_sessao_travar(uint64_t epoch) { return perfis_sessao_atual(epoch); }
void perfis_sessao_destravar(void) {}
