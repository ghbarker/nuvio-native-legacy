/* The body is generated from the frozen upstream tests/homejanelas.c.
 * Only typed catalogue neighbours and the completion observer are adapted;
 * every original assertion remains in order, unchanged.
 */
#include HOME_WINDOW_BODY
#include "cw-splice.inc"
int cat_home_apenas_fixas(void) {
  int fixed=1;
  pthread_mutex_lock(&pubTravaT);
  for(int k=0;k<nPubFils;k++)
    if(strcmp(pubFils[k].chave,"continue_watching") &&
       strcmp(pubFils[k].chave,"social_activity")) fixed=0;
  pthread_mutex_unlock(&pubTravaT);
  return fixed;
}
/* Exact production helper bound to this existing catalogue storage double. */
static int fixture_fileiraContinuar(void) {
  for(int r=0;r<nPubFils;r++)
    if(!strcmp(pubFils[r].chave,"continue_watching")) return r;
  return -1;
}
static _Thread_local int n;
#define fils pubFils
#define nFils nPubFils
#define pubTrava pubTravaT
#define fileiraContinuar fixture_fileiraContinuar
#define catTravar() (pthread_mutex_lock(&pubTravaT), (n = nPub))
#define revisaoSobe() ((void)0)
#define mudou() ((void)0)
#define cat_republicar_fileiras fixture_actual_cat_republicar_fileiras
#include "../startup/discovery/catalog-republish-actual.inc"
#undef cat_republicar_fileiras
#undef fils
#undef nFils
#undef pubTrava
#undef fileiraContinuar
#undef catTravar
#undef revisaoSobe
#undef mudou

/* Thread-local count binds the production helper's catalogue-size global.
 * Refresh it under the storage mutex both before its fast precheck and after
 * acquiring the real helper's lock. Struct member .n is never macro-renamed.
 */
void cat_republicar_fileiras(const CatFileira *rows,int count) {
  pthread_mutex_lock(&pubTravaT); n=nPub; pthread_mutex_unlock(&pubTravaT);
  fixture_actual_cat_republicar_fileiras(rows,count);
}
/* The original reduced test has no on-disk catalogue writer. */
int cat_gravar_cache_se_sessao(const char *dir,const char *user,int profile,uint64_t session) {
  (void)dir; (void)user; (void)profile; return nv_perfil_sessao_atual(session);
}
