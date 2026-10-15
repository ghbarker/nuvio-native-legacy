#include "perfis.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdint.h>
const char *sessao_usuario(void){return "session-fixture";}
int dados_gravar(const char*n,const char*t){(void)n;(void)t;return 1;}
void fil_definir_perfil(int p){(void)p;}
void fontepref_definir_perfil(int p){(void)p;}
void legmem_definir_perfil(int p){(void)p;}
void arteesc_definir_perfil(int p){(void)p;}
void plugins_perfil_mudou(void){}
void servidores_perfil_trocou(void){}
static int callbackOwner, callbackCount;
static void outgoing(void){callbackOwner=perfis_ativo(); callbackCount++;}
static uint64_t delayed;
static atomic_int ready, releaseWorker, staleAccepted;
static void *old_worker(void*u){(void)u; atomic_store(&ready,1); while(!atomic_load(&releaseWorker)){} if(perfis_sessao_travar(delayed)){atomic_store(&staleAccepted,1);perfis_sessao_destravar();}return NULL;}
int main(void){
  perfis_ao_abrir_sessao(outgoing);
  perfis_definir_ativo(1); uint64_t a=perfis_sessao_geracao();
  perfis_definir_ativo(2); assert(callbackOwner==1); uint64_t b=perfis_sessao_geracao();
  assert(b>a && !perfis_sessao_atual(a));
  perfis_definir_ativo(1); assert(callbackOwner==2); assert(!perfis_sessao_atual(a));
  uint64_t revisit=perfis_sessao_geracao();
  perfis_definir_ativo(1); assert(perfis_sessao_geracao()>revisit && callbackOwner==1);
  assert(perfis_sessao_travar(perfis_sessao_geracao()));
  assert(perfis_sessao_travar(perfis_sessao_geracao()));
  perfis_sessao_destravar(); perfis_sessao_destravar();
  pthread_t t; delayed=perfis_sessao_geracao(); assert(!pthread_create(&t,NULL,old_worker,NULL));
  while(!atomic_load(&ready)){}
  perfis_definir_ativo(2); perfis_definir_ativo(1);
  atomic_store(&releaseWorker,1); assert(!pthread_join(t,NULL)); assert(!atomic_load(&staleAccepted));
  assert(callbackCount==6);
  uint64_t priorAccount=perfis_sessao_geracao(); int selected=perfis_ativo();
  perfis_invalidar_sessao(); assert(perfis_ativo()==selected && !perfis_sessao_atual(priorAccount));
  assert(callbackCount==6);
  puts("PASS: same-profile and A-B-A openings advance ownership; outgoing callback uses original profile; delayed old worker rejected; nested publication guards work");
}
