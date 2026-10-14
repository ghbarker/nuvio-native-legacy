#define perfis_ativo legacy_perfis_ativo
#define main legacy_cache_fixture_main
#include "catcache.c"
#undef main
#undef perfis_ativo
#include "perfis.h"
#include <pthread.h>
int dados_gravar(const char*n,const char*t){(void)n;(void)t;return 1;}
void fil_definir_perfil(int p){(void)p;} void fontepref_definir_perfil(int p){(void)p;} void legmem_definir_perfil(int p){(void)p;} void arteesc_definir_perfil(int p){(void)p;} void plugins_perfil_mudou(void){} void servidores_perfil_trocou(void){}
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
static pthread_t worker; static int pauseWriter, started, released, result; static uint64_t original;
void nv_cache_session_mid(void){pthread_mutex_lock(&lock);if(pauseWriter && pthread_equal(worker,pthread_self())){started=1;pthread_cond_broadcast(&cond);while(!released)pthread_cond_wait(&cond,&lock);}pthread_mutex_unlock(&lock);}
static void*write_old(void*u){(void)u;result=cat_gravar_cache_se_sessao(NULL,usuarioAtual,1,original);return NULL;}
int main(void){
 char dir[]="/tmp/nuvio-cache-session-XXXXXX"; assert(mkdtemp(dir)); snprintf(dirDados,sizeof dirDados,"%s",dir);snprintf(usuarioAtual,sizeof usuarioAtual,"owned-cache");cat_cache_perfis(1);
 for(int variant=0;variant<3;variant++){
  perfis_definir_ativo(1);semearCatalogo("old-A");original=perfis_sessao_geracao();started=released=0;pauseWriter=1;assert(!pthread_create(&worker,NULL,write_old,NULL));
  pthread_mutex_lock(&lock);while(!started)pthread_cond_wait(&cond,&lock);pthread_mutex_unlock(&lock);
  perfis_definir_ativo(variant==2?1:2);if(variant==1)perfis_definir_ativo(1);semearCatalogo("new-visit");assert(cat_gravar_cache(NULL));
  pthread_mutex_lock(&lock);released=1;pthread_cond_broadcast(&cond);pthread_mutex_unlock(&lock);assert(!pthread_join(worker,NULL));assert(!result);
  cat_definir_tudo(NULL,0,NULL,0);assert(cat_ler_cache(NULL));assert(strstr(cat_item(0)->titulo,"new-visit"));
  assert(!cat_gravar_cache_se_sessao(NULL,usuarioAtual,1,original));
 }
 assert(cat_apagar_cache());rmdir(dir);puts("PASS: serialization cannot overwrite new cache on A-B, A-B-A or same-profile opening; late worker entry rejected");
}
