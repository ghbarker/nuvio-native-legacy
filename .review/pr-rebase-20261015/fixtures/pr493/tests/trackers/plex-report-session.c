// Review-only omitted neighbour for the reduced Plex final-report fixture.
// server-network.h fixes perfis_ativo() at1; its changeProfile() changes Plex's
// own generation/account, which the original report assertions exercise.
// There is no application profile-session producer in this isolated link.
// Supply exactly the optional absent-producer contract from perfisessao.h:
// epoch0 is current and every nonzero epoch is rejected. Real profile ownership
// fixtures and complete application builds continue linking real perfis.c.
#include "perfis.h"
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t generation) { return generation == 0; }
int perfis_sessao_travar(uint64_t generation) { return generation == 0; }
void perfis_sessao_destravar(void) { }
