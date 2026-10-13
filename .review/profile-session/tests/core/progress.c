#include "perfis.h"
#include "syncprog.h"
#include "progresso.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *stored;
char *dados_ler(const char*n){(void)n;return stored?strdup(stored):NULL;}
int dados_gravar(const char*n,const char*t){if(!strcmp(n,"progresso.txt")){free(stored);stored=strdup(t);}return 1;}
int dados_apagar(const char*n){(void)n;return 1;}
const char*dados_cliente_id(void){return "progress-fixture";}
const char*sessao_usuario(void){return "progress-owner";}
void fil_definir_perfil(int p){(void)p;} void fontepref_definir_perfil(int p){(void)p;} void legmem_definir_perfil(int p){(void)p;} void arteesc_definir_perfil(int p){(void)p;} void plugins_perfil_mudou(void){} void servidores_perfil_trocou(void){}
int cat_indice_por_imdb(const char*s){(void)s;return -1;} void cat_aplicar_progresso(int i,double p,double d,int t,int e){(void)i;(void)p;(void)d;(void)t;(void)e;assert(!"Unexpected remote catalog update");}
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
static int block,started,released,pullResult,pushResult;static char lastBody[4096];
char*sessao_rpc(const char*f,const char*b,int*st){
 pthread_mutex_lock(&lock);snprintf(lastBody,sizeof lastBody,"%s",b);if(block){started=1;pthread_cond_broadcast(&cond);while(!released)pthread_cond_wait(&cond,&lock);}pthread_mutex_unlock(&lock);*st=200;
 return strdup(strstr(f,"pull")?"[{\"content_id\":\"tt1111111\",\"content_type\":\"movie\",\"position_ms\":999000,\"duration_ms\":1800000,\"updated_at\":9999999999999}]":"{}");
}
char*nuvem_rpc_com(const char*f,const char*b,const char*j,int*st){(void)f;(void)b;(void)j;(void)st;assert(!"Fixture should use its legacy RPC mock");return NULL;}
static void*pull(void*u){(void)u;pullResult=syncprog_puxar();return NULL;}static void*push(void*u){(void)u;pushResult=syncprog_empurrar();return NULL;}
static void prepare(void){pthread_mutex_lock(&lock);block=1;started=released=0;pthread_mutex_unlock(&lock);}static void wait_start(void){pthread_mutex_lock(&lock);while(!started)pthread_cond_wait(&cond,&lock);pthread_mutex_unlock(&lock);}static void release(void){pthread_mutex_lock(&lock);released=1;pthread_cond_broadcast(&cond);pthread_mutex_unlock(&lock);}
int main(void){
 for(int variant=0;variant<3;variant++){
   perfis_definir_ativo(1);syncprog_esquecer();prepare();pthread_t t;assert(!pthread_create(&t,NULL,pull,NULL));wait_start();
   perfis_definir_ativo(variant==2?1:2);if(variant==1)perfis_definir_ativo(1);syncprog_esquecer();release();assert(!pthread_join(t,NULL));assert(pullResult==-1);assert(!syncprog_puxadas());assert(!syncprog_aplicar(NULL));
 }
 perfis_definir_ativo(1);assert(prog_gravar_local("tt2222222",0,0,100,1800,-1));prepare();pthread_t t;assert(!pthread_create(&t,NULL,push,NULL));wait_start();
 assert(strstr(lastBody,"\"p_profile_id\":1"));perfis_definir_ativo(2);assert(prog_gravar_local("tt3333333",0,0,200,1800,-1));release();assert(!pthread_join(t,NULL));assert(pushResult==1);
 ProgRegistro r[4];assert(prog_pendentes(r,4)==1&&!strcmp(r[0].contentId,"tt3333333"));perfis_definir_ativo(1);assert(prog_pendentes(r,4)==0);
 free(stored);puts("PASS: late progress pulls rejected for A-B, A-B-A and A-A; final outgoing progress keeps original profile and does not acknowledge B records");
}
