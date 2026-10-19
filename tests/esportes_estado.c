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
/* Freeze UTC around a provider/date-boundary regression. */
time_t time(time_t *out) { time_t t=1792411200; if(out)*out=t; return t; }
static char salvoNome[160], salvo[27000];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int entrou, liberado, pedidos,pedidosFav,modoFav,segurarFav,entrouFav;
int ajustes_idioma(void) { return 1; }
static const char *eventosNfl="{\"events\":["
 "{\"id\":\"game\",\"date\":\"2026-10-21T12:00Z\",\"name\":\"Team at Other\",\"competitions\":[{\"competitors\":[{\"team\":{\"id\":\"team\",\"displayName\":\"Team\"}},{\"team\":{\"id\":\"other\",\"displayName\":\"Other\"}}]}]},"
 "{\"id\":\"unrelated\",\"date\":\"2026-10-21T12:00Z\",\"name\":\"Wrong at Nope\",\"competitions\":[{\"competitors\":[{\"team\":{\"id\":\"wrong\",\"displayName\":\"Wrong\"}},{\"team\":{\"id\":\"nope\",\"displayName\":\"Nope\"}}]}]}]}";
static const char *eventosSoccer="{\"events\":["
 "{\"id\":\"cup\",\"date\":\"2026-10-22T12:00Z\",\"name\":\"Team at Rival\",\"league\":{\"slug\":\"uefa.champions\",\"name\":\"UEFA Champions League\"},\"competitions\":[{\"competitors\":[{\"team\":{\"id\":\"team\",\"displayName\":\"Team\"}},{\"team\":{\"id\":\"rival\",\"displayName\":\"Rival\"}}]}]},"
 "{\"id\":\"league\",\"date\":\"2026-10-23T12:00Z\",\"name\":\"Team at Rival\",\"league\":{\"slug\":\"eng.1\",\"name\":\"Premier League\"},\"competitions\":[{\"competitors\":[{\"team\":{\"id\":\"team\",\"displayName\":\"Team\"}},{\"team\":{\"id\":\"rival\",\"displayName\":\"Rival\"}}]}]}]}";
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
char *rede_baixar_medido_controle(const char *url,int s,const char *const *h,const RedeControle *c,RedeMedida *m) {
  (void)h;assert(s==8 && c->max_bytes==2*1024*1024 && strstr(url,"lang=en"));
  int fav=strstr(url,"/schedule?")!=NULL || (modoFav==4 && strstr(url,"/scoreboard?"));
  pthread_mutex_lock(&mutex);
  int numero=fav?++pedidosFav:++pedidos;entrou=1;if(fav)entrouFav=1;pthread_cond_broadcast(&cond);
  while(!liberado || (fav && segurarFav))pthread_cond_wait(&cond,&mutex);
  pthread_mutex_unlock(&mutex);m->status=200;
  if(fav) {
    if(modoFav==4 && strstr(url,"/schedule?")){m->status=404;return NULL;}
    if(strstr(url,"/soccer/")){assert(strstr(url,"/soccer/all/teams/") && strstr(url,"fixture=true"));return strdup(eventosSoccer);}
    return strdup(modoFav && modoFav!=5?eventosNfl:"{\"events\":[]}");
  }
  if(strstr(url,"/leagues/dropdown")) {
    assert(strstr(url,"sport=football"));return strdup("{\"leagues\":[{\"slug\":\"nfl\",\"name\":\"NFL\"}]}");
  }
  assert(strstr(url,"/sports/football/nfl/scoreboard?") && strstr(url,"limit=400"));
  const char *d=strstr(url,"dates=");
  if(d) {time_t t=time(NULL)+(numero-4)*86400;struct tm tm;char esperado[16];gmtime_r(&t,&tm);strftime(esperado,sizeof esperado,"%Y%m%d",&tm);assert(!strncmp(d+6,esperado,8));}
  return strdup("{\"events\":[]}");
}
static void pausa(void) { struct timespec t={0,1000000}; nanosleep(&t,NULL); }
static void drenar(void) {
  int i;
  for (i=0;i<10000 && (esportes_carregando() || esportes_favoritos_carregando());i++) { esportes_passo(); pausa(); }
  assert(i<10000); esportes_passo();
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
  assert(pedidos==10);
  /* Combined feed: different sports, soccer cups and duplicate opponents. */
  modoFav=1;sessao++;perfil=3;esportes_abrir_sessao();
  assert(esportes_alternar_favorito(&e,0));
  EspEvento outro=e;strcpy(outro.participantes[0].id,"other");assert(esportes_alternar_favorito(&outro,0));
  EspEvento soccer=e;strcpy(soccer.modalidade,"soccer");strcpy(soccer.ligaId,"eng.1");
  assert(esportes_alternar_favorito(&soccer,0));drenar();
  assert(esportes_favoritos_times()==3 && esportes_favoritos_n()==3 && !esportes_favoritos_erro());
  int cup=0,game=0;
  for(int k=0;k<esportes_favoritos_n();k++) {
    const EspEvento *v=esportes_favoritos_evento(k);
    if(!strcmp(v->id,"cup")){cup++;assert(!strcmp(v->ligaId,"uefa.champions") && !strcmp(v->liga,"UEFA Champions League"));}
    if(!strcmp(v->id,"game"))game++;
  }
  assert(cup==1 && game==1);
  EspEvento college=e;strcpy(college.ligaId,"college-football");assert(!esportes_favorito(&college,0));
  strcpy(soccer.ligaId,"uefa.champions");assert(esportes_favorito(&soccer,0));
  falharGravacao=1;assert(!esportes_alternar_favorito(&soccer,0) && esportes_favorito(&soccer,0));falharGravacao=0;
  assert(esportes_alternar_favorito(&soccer,0));drenar();assert(esportes_favoritos_n()==1);
  assert(esportes_habilitar(1,0));drenar();assert(!esportes_favoritos_n() && !esportes_favoritos_times());
  assert(esportes_habilitar(1,1));drenar();assert(esportes_favoritos_n()==1);
  /* A slow favorites fetch cannot block the foreground league or leak profiles. */
  pthread_mutex_lock(&mutex);segurarFav=1;entrouFav=0;pthread_mutex_unlock(&mutex);
  esportes_pedir_favoritos(1);esportes_passo();
  pthread_mutex_lock(&mutex);while(!entrouFav)pthread_cond_wait(&cond,&mutex);pthread_mutex_unlock(&mutex);
  esportes_pedir_ligas(1,1);
  for(int k=0;k<3000 && esportes_carregando();k++){esportes_passo();pausa();}
  assert(esportes_ligas_n()==1 && esportes_favoritos_carregando());
  sessao++;perfil=4;esportes_abrir_sessao();
  pthread_mutex_lock(&mutex);segurarFav=0;pthread_cond_broadcast(&cond);pthread_mutex_unlock(&mutex);
  drenar();assert(!esportes_favoritos_n() && !esportes_favoritos_times());
  /* Unsupported team APIs fall back to the existing eight bounded scoreboards. */
  modoFav=4;int antesFav=pedidosFav;assert(esportes_alternar_favorito(&e,0));drenar();
  assert(esportes_favoritos_n()==1 && pedidosFav-antesFav==9 && !esportes_favoritos_erro());
  modoFav=5;esportes_pedir_favoritos(1);drenar();assert(!esportes_favoritos_n());
  puts("sports profile settings, cancellation and URL contracts: passed"); return 0;
}
