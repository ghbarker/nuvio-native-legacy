// Real profile-fetch publication barrier, with delayed authenticated replies.
#include "perfis.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static char current_user[80]="account-a", current_jwt[3000]="JWT_A";
static char *disk;
static int writes, requests, renewals;
static pthread_mutex_t transport_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t transport_condition=PTHREAD_COND_INITIALIZER;
static const char *blocked_function;
static int blocking, waiting, released;
static uint64_t old_session;
static _Atomic int finished;
static int old_result;
void fil_definir_perfil(int p){(void)p;}
void fontepref_definir_perfil(int p){(void)p;}
void legmem_definir_perfil(int p){(void)p;}
void arteesc_definir_perfil(int p){(void)p;}
void plugins_perfil_mudou(void){}
void servidores_perfil_trocou(void){}
int ajustes_addons_do_principal(void){return 0;}
const char *sessao_usuario(void){return current_user;}
int sessao_identidade_local_copiar(char *u,size_t nu,char *jwt,size_t nj){snprintf(u,nu,"%s",current_user);snprintf(jwt,nj,"%s",current_jwt);return 1;}
void sessao_token_recusado(const char *jwt){assert(!strcmp(jwt,current_jwt));renewals++;}
char *sessao_rpc(const char *name,const char *body,int *status){(void)name;(void)body;(void)status;assert(!"scoped profile fetch used live session RPC");return NULL;}
const char *nuvem_url(void){return "https://profile.fixture.invalid";}
int dados_gravar(const char *name,const char *body){if(!strcmp(name,"perfis.txt")){free(disk);disk=strdup(body);writes++;}return 1;}
char *dados_ler(const char *name){return !strcmp(name,"perfis.txt")&&disk?strdup(disk):NULL;}
int dados_apagar(const char *name){if(!strcmp(name,"perfis.txt")){free(disk);disk=NULL;}return 1;}
char *nuvem_rpc_com(const char *name,const char *body,const char *bearer,int *status){
  (void)body;assert(bearer);assert(!strcmp(bearer,"JWT_A")||!strcmp(bearer,"JWT_B")||!strcmp(bearer,"JWT_A_NEW"));
  pthread_mutex_lock(&transport_mutex);requests++;
  if(blocking&&!strcmp(bearer,"JWT_A")&&!strcmp(name,blocked_function)){
    waiting=1;pthread_cond_broadcast(&transport_condition);
    while(!released)pthread_cond_wait(&transport_condition,&transport_mutex);
  }
  pthread_mutex_unlock(&transport_mutex);
  *status=200;
  const char *label=!strcmp(bearer,"JWT_A")?"Old A":!strcmp(bearer,"JWT_B")?"B":"New A";
  char reply[256];
  if(!strcmp(name,"get_sync_owner"))snprintf(reply,sizeof reply,"\"owner-%s\"",label);
  else if(!strcmp(name,"sync_pull_profiles"))snprintf(reply,sizeof reply,"[{\"profile_index\":1,\"name\":\"%s\"}]",label);
  else if(!strcmp(name,"sync_pull_profile_locks"))snprintf(reply,sizeof reply,"[{\"profile_id\":1,\"pin_enabled\":%s}]",!strcmp(label,"Old A")?"true":"false");
  else assert(!"unexpected request");
  return strdup(reply);
}
static void *old_fetch(void *unused){(void)unused;old_result=perfis_puxar_sessao(old_session,"account-a","JWT_A");atomic_store(&finished,1);return NULL;}
static void wait_enter(void){for(int i=0;i<3000;i++){pthread_mutex_lock(&transport_mutex);int ready=waiting;pthread_mutex_unlock(&transport_mutex);if(ready)return;usleep(1000);}assert(!"fetch did not block");}
static void block(const char *function){pthread_mutex_lock(&transport_mutex);blocked_function=function;blocking=1;waiting=released=0;pthread_mutex_unlock(&transport_mutex);atomic_store(&finished,0);}
static void release(void){pthread_mutex_lock(&transport_mutex);released=1;pthread_cond_broadcast(&transport_condition);pthread_mutex_unlock(&transport_mutex);}
static void account(const char *user,const char *jwt){perfis_esquecer();snprintf(current_user,sizeof current_user,"%s",user);snprintf(current_jwt,sizeof current_jwt,"%s",jwt);}
static void delayed_account(const char *function){
  account("account-a","JWT_A");old_session=perfis_sessao_geracao();block(function);pthread_t worker;pthread_create(&worker,NULL,old_fetch,NULL);wait_enter();
  // No owner/list/PIN/cache is visible until the complete owned result commits.
  assert(perfis_n()==0&&!perfis_dono()[0]);
  account("account-b","JWT_B");assert(perfis_puxar()==1);assert(!strcmp(perfis_item(0)->nome,"B"));assert(!strcmp(perfis_dono(),"owner-B"));assert(!perfis_item(0)->temPin);
  int saved_writes=writes;char *saved=strdup(disk);release();pthread_join(worker,NULL);
  assert(old_result==0&&atomic_load(&finished));assert(writes==saved_writes&&!strcmp(disk,saved));free(saved);
  assert(!strcmp(perfis_item(0)->nome,"B")&&!strcmp(perfis_dono(),"owner-B")&&!perfis_item(0)->temPin);
  printf("PASS: delayed %s cannot overwrite B owner, names, PIN state or disk cache\n",function);
}
int main(void){
  delayed_account("get_sync_owner");delayed_account("sync_pull_profiles");delayed_account("sync_pull_profile_locks");
  account("account-a","JWT_A");old_session=perfis_sessao_geracao();block("sync_pull_profile_locks");pthread_t worker;pthread_create(&worker,NULL,old_fetch,NULL);wait_enter();
  perfis_definir_ativo(2);perfis_definir_ativo(1);snprintf(current_jwt,sizeof current_jwt,"JWT_A_NEW");
  assert(perfis_puxar_sessao(perfis_sessao_geracao(),"account-a","JWT_A_NEW")==1);int saved_writes=writes;release();pthread_join(worker,NULL);
  assert(old_result==0&&writes==saved_writes&&!strcmp(perfis_item(0)->nome,"New A")&&!perfis_item(0)->temPin);
  puts("PASS: A to B to A rejects the original profile/locks response despite identical account");
  int saved_requests=requests;assert(perfis_puxar_sessao(old_session,"account-a","JWT_A")==0);assert(requests==saved_requests&&!renewals);
  puts("PASS: a stale dispatch snapshot performs no request or token renewal");free(disk);puts("profile fetch isolation: PASS");
}
