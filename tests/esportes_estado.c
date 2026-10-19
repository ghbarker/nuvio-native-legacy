#define _POSIX_C_SOURCE 200809L
#include "esportes.h"
#include "rede.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t sessao = 1;
static int perfil = 1, falharGravacao;
static char salvoNome[160], salvo[27000];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int entrou, liberado, pedidos;
uint64_t perfis_sessao_geracao(void) { return sessao; }
int perfis_sessao_atual(uint64_t s) { return s == sessao; }
int perfis_ativo(void) { return perfil; }
const char *sessao_usuario(void) { return "test-account"; }
char *dados_ler(const char *nome) { return !strcmp(nome,salvoNome) ? strdup(salvo) : NULL; }
int dados_gravar(const char *nome, const char *t) {
  if (falharGravacao) return 0;
  snprintf(salvoNome,sizeof salvoNome,"%s",nome); snprintf(salvo,sizeof salvo,"%s",t); return 1;
}
int dados_gravar_leve(const char *nome, const char *t) { (void)nome; (void)t; return 1; }
char *rede_baixar_medido_controle(const char *url, int s, const char *const *h, const RedeControle *c, RedeMedida *m) {
  (void)h; assert(s == 8 && c->max_bytes == 2*1024*1024);
  assert(strstr(url,"https://site.api.espn.com/apis/site/v2/"));
  pthread_mutex_lock(&mutex); pedidos++; entrou = 1; pthread_cond_broadcast(&cond);
  while (!liberado) pthread_cond_wait(&cond,&mutex);
  pthread_mutex_unlock(&mutex); m->status = 200;
  if (strstr(url,"/leagues/dropdown")) {
    assert(strstr(url,"sport=football"));
    return strdup("{\"leagues\":[{\"slug\":\"nfl\",\"name\":\"NFL\"}]}");
  }
  assert(strstr(url,"/sports/football/nfl/scoreboard?"));
  assert(strstr(url,"limit=400")); return strdup("{\"events\":[]}");
}
static void pausa(void) { struct timespec t={0,1000000}; nanosleep(&t,NULL); }
static void drenar(void) {
  int i;
  for (i=0;i<3000 && esportes_carregando();i++) { esportes_passo(); pausa(); }
  assert(i<3000); esportes_passo();
}
int main(void) {
  esportes_abrir_sessao();
  assert(esportes_habilitado(1)); assert(esportes_habilitar(1,0));
  assert(!esportes_habilitado(1));
  esportes_pedir_ligas(1,1); assert(!esportes_carregando()); assert(!pedidos);
  falharGravacao=1; assert(!esportes_habilitar(1,1)); assert(!esportes_habilitado(1)); falharGravacao=0;
  sessao++; esportes_abrir_sessao(); assert(!esportes_habilitado(1));
  assert(esportes_habilitar(1,1));
  EspEvento e={0}; strcpy(e.modalidade,"football"); strcpy(e.ligaId,"nfl");
  e.nParticipantes=1; strcpy(e.participantes[0].id,"team");
  assert(esportes_alternar_favorito(&e,0)); assert(esportes_favorito(&e,0));
  falharGravacao=1; assert(!esportes_alternar_favorito(&e,0)); assert(esportes_favorito(&e,0)); falharGravacao=0;
  sessao++; esportes_abrir_sessao(); assert(esportes_favorito(&e,0));
  /* Old public catalog results cannot populate the next profile. */
  esportes_pedir_ligas(1,1); esportes_passo();
  pthread_mutex_lock(&mutex); while(!entrou) pthread_cond_wait(&cond,&mutex); pthread_mutex_unlock(&mutex);
  sessao++; perfil=2; esportes_abrir_sessao(); assert(!esportes_favorito(&e,0));
  pthread_mutex_lock(&mutex); liberado=1; pthread_cond_broadcast(&cond); pthread_mutex_unlock(&mutex);
  drenar(); assert(!esportes_ligas_n());
  esportes_pedir_ligas(1,1); drenar(); assert(esportes_ligas_n()==1);
  EspLiga l=*esportes_liga(0); assert(!strcmp(l.id,"nfl"));
  esportes_pedir_eventos(1,&l,0); drenar(); assert(!esportes_erro() && !esportes_eventos_n());
  esportes_pedir_ligas(1,1); esportes_cancelar(); drenar();
  assert(pedidos==9);
  puts("sports profile settings, cancellation and URL contracts: passed"); return 0;
}
