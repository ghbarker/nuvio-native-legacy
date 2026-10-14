// Review-only adapters into the selected tree's actual session boundaries.
#include "sessao.c"
void teste_tokens(const char *a, const char *r) { pthread_mutex_lock(&tokTrava); gravarTokensL(a,r,0); pthread_mutex_unlock(&tokTrava); }
void teste_limpar(void) { pthread_mutex_lock(&tokTrava); limpar(); pthread_mutex_unlock(&tokTrava); }
