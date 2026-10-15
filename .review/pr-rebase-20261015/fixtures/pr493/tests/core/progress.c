#include "perfis.h"
#include "syncprog.h"
#include "progresso.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *stored;
static char account[80] = "progress-owner", accountJwt[80] = "progress-jwt";
static pthread_mutex_t accountLock = PTHREAD_MUTEX_INITIALIZER;
char *dados_ler(const char*n){(void)n;return stored?strdup(stored):NULL;}
int dados_gravar(const char*n,const char*t){if(!strcmp(n,"progresso.txt")){free(stored);stored=strdup(t);}return 1;}
int dados_apagar(const char*n){if(!strcmp(n,"progresso.txt")){free(stored);stored=NULL;}return 1;}
const char*dados_cliente_id(void){return "progress-fixture";}
const char*sessao_usuario(void){return account;}
int sessao_identidade_local_copiar(char *u, size_t nu, char *jwt, size_t nj) {
 int ok;
 if (!u || !nu || !jwt || !nj) return 0;
 u[0] = jwt[0] = 0;
 pthread_mutex_lock(&accountLock);
 ok = account[0] && accountJwt[0] && strlen(account) < nu && strlen(accountJwt) < nj;
 if (ok) { snprintf(u, nu, "%s", account); snprintf(jwt, nj, "%s", accountJwt); }
 pthread_mutex_unlock(&accountLock);
 return ok;
}
int sessao_token_copiar(char *jwt, size_t n) {
 char u[80];
 return sessao_identidade_local_copiar(u, sizeof u, jwt, n);
}
void sessao_token_recusado(const char *jwt) {
 (void)jwt; assert(!"Transport always returns 200; no token renewal is expected");
}
static void change_account(const char *u, const char *jwt) {
 pthread_mutex_lock(&accountLock);
 snprintf(account, sizeof account, "%s", u); snprintf(accountJwt, sizeof accountJwt, "%s", jwt);
 pthread_mutex_unlock(&accountLock);
 perfis_invalidar_sessao();
}
void fil_definir_perfil(int p){(void)p;} void fontepref_definir_perfil(int p){(void)p;} void legmem_definir_perfil(int p){(void)p;} void arteesc_definir_perfil(int p){(void)p;} void plugins_perfil_mudou(void){} void servidores_perfil_trocou(void){}
int cat_indice_por_imdb(const char*s){(void)s;return -1;} void cat_aplicar_progresso(int i,double p,double d,int t,int e){(void)i;(void)p;(void)d;(void)t;(void)e;assert(!"Unexpected remote catalog update");}
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
static int block,started,released,pullResult,pushResult;static char lastBody[4096], expectedJwt[80];
static char*transport(const char*f,const char*b,int*st){
 pthread_mutex_lock(&lock);snprintf(lastBody,sizeof lastBody,"%s",b);if(block){started=1;pthread_cond_broadcast(&cond);while(!released)pthread_cond_wait(&cond,&lock);}pthread_mutex_unlock(&lock);*st=200;
 return strdup(strstr(f,"pull")?"[{\"content_id\":\"tt1111111\",\"content_type\":\"movie\",\"position_ms\":999000,\"duration_ms\":1800000,\"updated_at\":9999999999999}]":"{}");
}
char*sessao_rpc(const char*f,const char*b,int*st){(void)f;(void)b;(void)st;assert(!"Authenticated fixture must use its captured JWT");return NULL;}
char*nuvem_rpc_com(const char*f,const char*b,const char*j,int*st){assert(!strcmp(j,expectedJwt));return transport(f,b,st);}
static void*pull(void*u){(void)u;pullResult=syncprog_puxar();return NULL;}static void*push(void*u){(void)u;pushResult=syncprog_empurrar();return NULL;}
static void prepare(void){assert(sessao_token_copiar(expectedJwt,sizeof expectedJwt));pthread_mutex_lock(&lock);block=1;started=released=0;pthread_mutex_unlock(&lock);}static void wait_start(void){pthread_mutex_lock(&lock);while(!started)pthread_cond_wait(&cond,&lock);pthread_mutex_unlock(&lock);}static void release(void){pthread_mutex_lock(&lock);released=1;pthread_cond_broadcast(&cond);pthread_mutex_unlock(&lock);}
int main(void){
 for(int variant=0;variant<3;variant++){
   perfis_definir_ativo(1);syncprog_esquecer();prepare();pthread_t t;assert(!pthread_create(&t,NULL,pull,NULL));wait_start();
   perfis_definir_ativo(variant==2?1:2);if(variant==1)perfis_definir_ativo(1);syncprog_esquecer();release();assert(!pthread_join(t,NULL));assert(pullResult==-1);assert(!syncprog_puxadas());assert(!syncprog_aplicar(NULL));
 }
 perfis_definir_ativo(1);assert(prog_gravar_local("tt2222222",0,0,100,1800,-1));prepare();pthread_t t;assert(!pthread_create(&t,NULL,push,NULL));wait_start();
 assert(strstr(lastBody,"\"p_profile_id\":1"));perfis_definir_ativo(2);assert(prog_gravar_local("tt3333333",0,0,200,1800,-1));release();assert(!pthread_join(t,NULL));assert(pushResult==1);
 ProgRegistro r[4];assert(prog_pendentes(r,4)==1&&!strcmp(r[0].contentId,"tt3333333"));perfis_definir_ativo(1);assert(prog_pendentes(r,4)==0);
 puts("PASS: late progress pulls rejected for A-B, A-B-A and A-A; final outgoing progress keeps original profile and does not acknowledge B records");
 syncprog_esquecer();prepare();assert(!pthread_create(&t,NULL,pull,NULL));wait_start();
 change_account("other-progress-owner","other-progress-jwt");syncprog_esquecer();release();assert(!pthread_join(t,NULL));
 assert(pullResult==-1&&!syncprog_puxadas()&&!syncprog_aplicar(NULL));
 puts("PASS: delayed old-account pull cannot populate the new account mailbox");
 change_account("progress-owner","progress-jwt");prog_esquecer_tudo();
 assert(prog_gravar_local("tt4444444",0,0,100,1800,-1));prepare();assert(!pthread_create(&t,NULL,push,NULL));wait_start();
 change_account("other-progress-owner","other-progress-jwt");prog_esquecer_tudo();
 assert(prog_gravar_local("tt4444444",0,0,100,1800,-1));release();assert(!pthread_join(t,NULL));
 assert(pushResult==1);assert(prog_pendentes(r,4)==1&&!strcmp(r[0].contentId,"tt4444444"));
 puts("PASS: old-account push retains its original JWT and cannot acknowledge the new account record");
 free(stored);
}
