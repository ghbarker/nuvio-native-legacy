/* Review-only stable idle identity/cache for episode/list ownership tests.
 * Use real declarations. This is the optional no-account infrastructure epoch,
 * not a profile-switch model; an unexpected epoch or unbalanced guard aborts.
 * Strong typed companions prevent Mach-O optional imports becoming abort traps.
 */
#include <assert.h>
#include <stdint.h>
#include "perfis.h"
#include "sessao.h"
#include "fontecache.h"
static unsigned held;
const char *sessao_usuario(void) {return "";}
int perfis_ativo(void) {return 0;}
unsigned fontecache_vod_geracao(void) {return 0;}
uint64_t perfis_sessao_geracao(void) {return 0;}
int perfis_sessao_atual(uint64_t epoch) {assert(epoch == 0);return 1;}
int perfis_sessao_travar(uint64_t epoch) {assert(epoch == 0);held++;return 1;}
void perfis_sessao_destravar(void) {assert(held > 0);held--;}
__attribute__((destructor)) static void identity_finished(void) {assert(held == 0);}
