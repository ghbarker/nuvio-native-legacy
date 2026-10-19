#define _POSIX_C_SOURCE 200809L
#include "esportes.h"
#include "dados.h"
#include "perfis.h"
#include "sessao.h"
#include "perfisessao.h"
#include "rede.h"
#include "idiomacod.h"
#include <ctype.h>
/* Keep the metadata worker independent of the graphics/settings headers. */
int ajustes_idioma(void);
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  unsigned geracao;
  NvPerfilSessao perfil;
  int tipo, modalidade, atualizar, erro, n, dia;
  EspLiga liga;
  EspLiga *ligas;
  EspEvento *eventos;
  char url[500], arquivo[160], time[80], idioma[8];
} EspJob;
static EspLiga *ligas;
static EspEvento *eventos;
static int nLigas, nEventos, erro;
static unsigned geracao, revisao, desligados;
static char favoritos[128][200];
static int nFavoritos;
static NvPerfilSessao perfil;
static char arquivo[160];
static atomic_int pronto;
static int ocupado;
static EspJob *resultado, *fila;
static EspJob *resultadoFav, *filaFav;
static atomic_int prontoFav;
static int ocupadoFav, favoritoCursor, favoritoAtivo, favoritoAtualizar, erroFav, idiomaFav=-1;
static unsigned geracaoFav;
static time_t favoritosDesde;
static int favoritosCompleto;
static EspEvento *eventosFav;
static int nEventosFav;
static void favoritosPasso(void);
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;

static unsigned hash(const char *s) {
  unsigned h = 2166136261u;
  for (; s && *s; s++) { h ^= (unsigned char)*s; h *= 16777619u; }
  return h;
}
static void soltar(EspJob *j) {
  if (!j) return;
  free(j->ligas); free(j->eventos); free(j);
}
void esportes_cancelar(void) {
  geracao++;soltar(fila);fila=NULL;
  if(ocupadoFav||filaFav||favoritoAtivo)favoritosCompleto=0;
  geracaoFav++;soltar(filaFav);filaFav=NULL;favoritoAtivo=0;
}
void esportes_abrir_sessao(void) {
  NvPerfilSessao atual = nv_perfil_sessao_ler();
  if (arquivo[0] && atual == perfil) return;
  esportes_cancelar(); perfil = atual; desligados = 0; nFavoritos = 0;
  free(ligas); ligas = NULL; free(eventos); eventos = NULL;
  free(eventosFav);eventosFav=NULL;nEventosFav=erroFav=0;favoritosDesde=0;
  nLigas = nEventos = erro = 0; revisao++;
  snprintf(arquivo, sizeof arquivo, "esportes-%08x-p%d.txt", hash(sessao_usuario()), perfis_ativo());
  char *buf = dados_ler(arquivo), *salvar = NULL;
  if (!buf) return;
  for (char *l = strtok_r(buf, "\n", &salvar); l; l = strtok_r(NULL, "\n", &salvar)) {
    if (!strncmp(l, "off=", 4)) desligados = (unsigned)strtoul(l + 4, NULL, 16);
    else if (!strncmp(l, "fav=", 4) && strlen(l + 4) < sizeof favoritos[0] && nFavoritos < 128)
      snprintf(favoritos[nFavoritos++], sizeof favoritos[0], "%s", l + 4);
  }
  free(buf);
}
static int gravar(void) {
  char *buf = calloc(1, 27000); if (!buf) return 0;
  size_t usado = (size_t)snprintf(buf, 27000, "off=%x\n", desligados);
  for (int i = 0; i < nFavoritos && usado < 26500; i++)
    usado += (size_t)snprintf(buf + usado, 27000 - usado, "fav=%s\n", favoritos[i]);
  int ok = nv_perfil_sessao_atual(perfil) && dados_gravar(arquivo, buf);
  free(buf); return ok;
}
int esportes_habilitado(int m) { return esportes_modalidade(m) && !(desligados & (1u << m)); }
int esportes_habilitar(int m, int on) {
  if (!esportes_modalidade(m)) return 0;
  unsigned antes = desligados;
  if (on) desligados &= ~(1u << m); else desligados |= 1u << m;
  if (!gravar()) { desligados = antes; return 0; }
  esportes_pedir_favoritos(1);return 1;
}
static int chaveFav(const EspEvento *e, int p, char *out, size_t n) {
  if (!e || p < 0 || p >= e->nParticipantes || !e->participantes[p].id[0]) return 0;
  snprintf(out, n, "%s|%s|%s", e->modalidade, e->ligaId, e->participantes[p].id); return 1;
}
static int favoritoIndice(const char *c) {
  for(int i=0;i<nFavoritos;i++) {
    if(!strcmp(c,favoritos[i]))return i;
    /* Soccer team IDs span league/cup competitions. Other sports may reuse
     * numeric IDs between professional, college and women's leagues. */
    if(!strncmp(c,"soccer|",7) && !strncmp(favoritos[i],"soccer|",7)) {
      const char *a=strrchr(c,'|'),*b=strrchr(favoritos[i],'|');
      if(a && b && !strcmp(a,b))return i;
    }
  }
  return -1;
}
int esportes_favorito(const EspEvento *e,int p) {
  char c[200];return chaveFav(e,p,c,sizeof c) && favoritoIndice(c)>=0;
}
int esportes_alternar_favorito(const EspEvento *e, int p) {
  char c[200]; if (!chaveFav(e, p, c, sizeof c)) return 0;
  int i=favoritoIndice(c);if(i<0)i=nFavoritos;
  /* Removal from another competition must restore the original key on failure. */
  if (i<nFavoritos) snprintf(c,sizeof c,"%s",favoritos[i]);
  if (i < nFavoritos) {
    char ultimo[200]; memcpy(ultimo, favoritos[nFavoritos-1], sizeof ultimo);
    memcpy(favoritos[i], ultimo, sizeof ultimo); nFavoritos--;
    if (!gravar()) { memcpy(favoritos[nFavoritos++], ultimo, sizeof ultimo); snprintf(favoritos[i], sizeof favoritos[i], "%s", c); return 0; }
  } else {
    if (nFavoritos >= 128) return 0;
    snprintf(favoritos[nFavoritos++], sizeof favoritos[0], "%s", c);
    if (!gravar()) { nFavoritos--; return 0; }
  }
  esportes_pedir_favoritos(1);revisao++; return 1;
}
static int ordem(const void *a, const void *b) {
  const EspEvento *x = a, *y = b;
  int gx = x->estado == 1 ? 0 : x->estado == 0 ? 1 : 2;
  int gy = y->estado == 1 ? 0 : y->estado == 0 ? 1 : 2;
  if (gx != gy) return gx - gy;
  return x->inicio < y->inicio ? -1 : x->inicio > y->inicio;
}
static void *buscar(void *arg) {
  EspJob *j = arg;
  char *cache = j->atualizar ? NULL : dados_ler(j->arquivo), *corpo = NULL;
  if (cache) {
    char *nl = strchr(cache, '\n');
    long long t = strtoll(cache, NULL, 10), idade = (long long)time(NULL) - t;
    unsigned chave = 0; char *espaco = strchr(cache, ' ');
    if (espaco) chave = (unsigned)strtoul(espaco + 1, NULL, 16);
    if (nl && chave == hash(j->url) && idade >= 0 && idade < (j->tipo == 1 ? 86400 : 60)) corpo = nl + 1;
  }
  char *baixado = NULL;
  if (!corpo) {
    RedeMedida medida = {0}; RedeControle controle = {2 * 1024 * 1024, NULL};
    baixado = rede_baixar_medido_controle(j->url, 8, NULL, &controle, &medida);
    if (medida.status == 200 && baixado) corpo = baixado;
  }
  if (corpo) {
    if (j->tipo == 1) {
      j->ligas = calloc(ESP_MAX_LIGAS, sizeof *j->ligas);
      if (j->ligas) j->n = esportes_ligas_ler(corpo, j->ligas, ESP_MAX_LIGAS);
      j->erro = !j->ligas || !strstr(corpo, "\"leagues\"");
    } else {
      j->eventos = calloc(ESP_MAX_EVENTOS, sizeof *j->eventos);
      if (j->eventos) j->n = esportes_eventos_ler(corpo, esportes_modalidade(j->modalidade)->id, &j->liga, j->eventos, ESP_MAX_EVENTOS);
      j->erro = !j->eventos || !strstr(corpo, "\"events\"");
      if (j->n) qsort(j->eventos, (size_t)j->n, sizeof *j->eventos, ordem);
    }
    if (baixado && !j->erro) {
      size_t n = strlen(corpo) + 48; char *s = malloc(n);
      if (s) { snprintf(s, n, "%lld %08x\n%s", (long long)time(NULL), hash(j->url), corpo); dados_gravar_leve(j->arquivo, s); free(s); }
    }
  } else j->erro = 1;
  printf("[esportes] ESPN %s: %d itens, erro=%d\n", j->tipo == 1 ? "ligas" : "eventos", j->n, j->erro);
  free(baixado); free(cache);
  pthread_mutex_lock(&trava);
  if (j->tipo>=3) resultadoFav=j;else resultado=j;
  pthread_mutex_unlock(&trava);
  atomic_store_explicit(j->tipo>=3?&prontoFav:&pronto,1,memory_order_release);return NULL;
}
static void agendar(EspJob *j) {
  if (!j) { erro = 1; return; }
  geracao++; j->geracao = geracao; j->perfil = perfil;
  soltar(fila); fila = j; erro = 0;
}
void esportes_pedir_ligas(int m, int atualizar) {
  if (!esportes_habilitado(m)) return;
  EspJob *j = calloc(1, sizeof *j); if (!j) { erro = 1; return; }
  j->tipo = 1; j->modalidade = m; j->atualizar = atualizar;
  snprintf(j->idioma,sizeof j->idioma,"%s",idioma_iso(ajustes_idioma()));
  snprintf(j->url, sizeof j->url, "https://site.api.espn.com/apis/site/v2/leagues/dropdown?lang=%s&region=us&sport=%s",j->idioma,esportes_modalidade(m)->id);
  snprintf(j->arquivo, sizeof j->arquivo, "esportes-ligas-%s.json", esportes_modalidade(m)->id);
  free(ligas); ligas = NULL; nLigas = 0; revisao++; agendar(j);
}
void esportes_pedir_eventos(int m, const EspLiga *l, int atualizar) {
  if (!l || !esportes_habilitado(m)) return;
  EspJob *j = calloc(1, sizeof *j); if (!j) { erro = 1; return; }
  j->tipo = 2; j->modalidade = m; j->atualizar = atualizar; j->liga = *l;
  snprintf(j->idioma,sizeof j->idioma,"%s",idioma_iso(ajustes_idioma()));
  snprintf(j->url, sizeof j->url, "https://site.api.espn.com/apis/site/v2/sports/%s/%s/scoreboard?limit=%d&lang=%s", esportes_modalidade(m)->id, l->id, ESP_MAX_EVENTOS,j->idioma);
  snprintf(j->arquivo, sizeof j->arquivo, "esportes-eventos-cache.json");
  if (!atualizar) { free(eventos); eventos = NULL; nEventos = 0; revisao++; }
  agendar(j);
}
void esportes_passo(void) {
  if (atomic_load_explicit(&pronto, memory_order_acquire)) {
    pthread_mutex_lock(&trava); EspJob *j = resultado; resultado = NULL; pthread_mutex_unlock(&trava);
    atomic_store_explicit(&pronto, 0, memory_order_release); ocupado = 0;
    if (j && j->geracao == geracao && j->perfil == perfil && nv_perfil_sessao_atual(j->perfil)) {
      erro = j->erro;
      if (!j->erro && j->tipo == 1) { free(ligas); ligas = j->ligas; j->ligas = NULL; nLigas = j->n; }
      else if (!j->erro) {
        if (!eventos) eventos = calloc(ESP_MAX_EVENTOS, sizeof *eventos);
        if (eventos) {
          for (int k=0;k<j->n;k++) {
            int pos;
            for (pos=0;pos<nEventos;pos++) if (!strcmp(eventos[pos].id,j->eventos[k].id)) break;
            if (pos < ESP_MAX_EVENTOS) { eventos[pos]=j->eventos[k]; if (pos==nEventos) nEventos++; }
          }
          qsort(eventos,(size_t)nEventos,sizeof *eventos,ordem);
        } else erro = 1;
        /* Default scoreboard first, then one date per request: several ESPN
         * leagues reject ranges. Publish every day's results immediately;
         * user navigation/profile changes discard the remaining work. */
        if (!erro && !j->atualizar && j->dia < 7 && nEventos < ESP_MAX_EVENTOS) {
          EspJob *proximo=calloc(1,sizeof *proximo);
          if (proximo) {
            *proximo=*j; proximo->ligas=NULL; proximo->eventos=NULL; proximo->n=0; proximo->dia++;
            char dia[16]; struct tm tm; time_t t=time(NULL)+(proximo->dia-1)*86400;
            gmtime_r(&t,&tm); strftime(dia,sizeof dia,"%Y%m%d",&tm);
            snprintf(proximo->url,sizeof proximo->url,"https://site.api.espn.com/apis/site/v2/sports/%s/%s/scoreboard?dates=%s&limit=%d&lang=%s",esportes_modalidade(j->modalidade)->id,j->liga.id,dia,ESP_MAX_EVENTOS,j->idioma);
            soltar(fila); fila=proximo;
          }
        }
      }
      revisao++;
    }
    soltar(j);
  }
  if (!ocupado && fila) {
    EspJob *j = fila; fila = NULL; pthread_t fio; ocupado = 1;
    if (pthread_create(&fio, NULL, buscar, j)) { ocupado = 0; erro = 1; soltar(j); }
    else pthread_detach(fio);
  }
  favoritosPasso();
}
int esportes_carregando(void) { return ocupado || fila != NULL; }
int esportes_erro(void) { return erro; }
unsigned esportes_revisao(void) { return revisao; }
int esportes_ligas_n(void) { return nLigas; }
const EspLiga *esportes_liga(int i) { return i >= 0 && i < nLigas ? &ligas[i] : NULL; }
int esportes_eventos_n(void) { return nEventos; }
const EspEvento *esportes_evento(int i) { return i >= 0 && i < nEventos ? &eventos[i] : NULL; }

static int modalidadeDe(const char *id) {
  for(int i=0;i<esportes_modalidades_n();i++) if(!strcmp(esportes_modalidade(i)->id,id))return i;
  return -1;
}
static int idSeguro(const char *id) {
  if(!id[0])return 0;
  for(;*id;id++)if(!isalnum((unsigned char)*id) && *id!='-' && *id!='.' && *id!='_')return 0;
  return 1;
}
static int favoritoAtual(const EspEvento *e) {
  int m=modalidadeDe(e->modalidade);
  return m>=0 && esportes_habilitado(m) && (esportes_favorito(e,0)||esportes_favorito(e,1));
}
static int eventoDisponivel(const EspEvento *e,time_t agora) {
  return e->estado==1 || (e->estado==0 && e->inicio>=agora-6*3600);
}
static void podarFavoritos(void) {
  int n=0;time_t agora=time(NULL);
  for(int i=0;i<nEventosFav;i++)if(favoritoAtual(&eventosFav[i]) && eventoDisponivel(&eventosFav[i],agora))eventosFav[n++]=eventosFav[i];
  nEventosFav=n;
}
void esportes_pedir_favoritos(int atualizar) {
  int idioma=ajustes_idioma();time_t agora=time(NULL);
  if(!atualizar && idioma==idiomaFav && favoritosDesde && agora>=favoritosDesde && agora-favoritosDesde<60 && (favoritosCompleto||favoritoAtivo))return;
  geracaoFav++;soltar(filaFav);filaFav=NULL;
  favoritosCompleto=0;favoritoCursor=0;favoritoAtivo=1;favoritoAtualizar=atualizar;erroFav=0;
  idiomaFav=idioma;favoritosDesde=agora;podarFavoritos();revisao++;
}
static void urlFavorito(EspJob *j) {
  const char *mod=esportes_modalidade(j->modalidade)->id;
  if(j->tipo==3) {
    snprintf(j->url,sizeof j->url,"https://site.api.espn.com/apis/site/v2/sports/%s/%s/teams/%s/schedule?fixture=true&lang=%s",mod,!strcmp(mod,"soccer")?"all":j->liga.id,j->time,j->idioma);
  } else if(j->dia) {
    time_t t=time(NULL)+(j->dia-1)*86400;struct tm tm;char dia[16];
    gmtime_r(&t,&tm);strftime(dia,sizeof dia,"%Y%m%d",&tm);
    snprintf(j->url,sizeof j->url,"https://site.api.espn.com/apis/site/v2/sports/%s/%s/scoreboard?dates=%s&limit=%d&lang=%s",mod,j->liga.id,dia,ESP_MAX_EVENTOS,j->idioma);
  } else snprintf(j->url,sizeof j->url,"https://site.api.espn.com/apis/site/v2/sports/%s/%s/scoreboard?limit=%d&lang=%s",mod,j->liga.id,ESP_MAX_EVENTOS,j->idioma);
}
static EspJob *proximoFavorito(void) {
  while(favoritoAtivo && favoritoCursor<nFavoritos) {
    const char *chave=favoritos[favoritoCursor++];char mod[32],ligaId[80],timeId[80],extra;
    if(sscanf(chave,"%31[^|]|%79[^|]|%79[^|]%c",mod,ligaId,timeId,&extra)!=3 ||
       !idSeguro(ligaId)||!idSeguro(timeId))continue;
    int m=modalidadeDe(mod);if(m<0 || !esportes_habilitado(m))continue;
    EspJob *j=calloc(1,sizeof *j);if(!j){erroFav=1;return NULL;}
    j->tipo=(!strcmp(mod,"tennis")||!strcmp(mod,"mma"))?4:3;
    j->modalidade=m;j->geracao=geracaoFav;j->perfil=perfil;j->atualizar=favoritoAtualizar;
    snprintf(j->liga.id,sizeof j->liga.id,"%s",ligaId);
    snprintf(j->liga.nome,sizeof j->liga.nome,"%s",ligaId);
    for(int k=0;k<nLigas;k++)if(!strcmp(ligas[k].id,ligaId)){j->liga=ligas[k];break;}
    snprintf(j->time,sizeof j->time,"%s",timeId);
    snprintf(j->idioma,sizeof j->idioma,"%s",idioma_iso(idiomaFav));
    /* One bounded public cache slot; profile-owned favorites never go here. */
    snprintf(j->arquivo,sizeof j->arquivo,"esportes-favoritos-cache.json");
    urlFavorito(j);return j;
  }
  favoritoAtivo=0;favoritosCompleto=1;return NULL;
}
static void publicarFavoritos(const EspJob *j) {
  if(!eventosFav)eventosFav=calloc(ESP_MAX_EVENTOS,sizeof *eventosFav);
  if(!eventosFav){erroFav=1;return;}
  if(j->tipo==3) {
    int n=0;const char *mod=esportes_modalidade(j->modalidade)->id;
    for(int i=0;i<nEventosFav;i++) {
      const EspEvento *e=&eventosFav[i];int pertence=0;
      if(!strcmp(e->modalidade,mod) && (!strcmp(mod,"soccer") || !strcmp(e->ligaId,j->liga.id)))
        for(int p=0;p<e->nParticipantes;p++)if(!strcmp(e->participantes[p].id,j->time))pertence=1;
      if(!pertence)eventosFav[n++]=*e;
    }
    nEventosFav=n;
  }
  for(int k=0;k<j->n;k++) {
    const EspEvento *e=&j->eventos[k];if(!favoritoAtual(e))continue;
    int pos;for(pos=0;pos<nEventosFav;pos++)
      if(!strcmp(eventosFav[pos].modalidade,e->modalidade) && !strcmp(eventosFav[pos].ligaId,e->ligaId) && !strcmp(eventosFav[pos].id,e->id))break;
    if(pos==ESP_MAX_EVENTOS) {
      qsort(eventosFav,(size_t)nEventosFav,sizeof *eventosFav,ordem);
      if(ordem(e,&eventosFav[pos-1])>=0)continue;
      pos--;
    }
    eventosFav[pos]=*e;if(pos==nEventosFav)nEventosFav++;
  }
  podarFavoritos();qsort(eventosFav,(size_t)nEventosFav,sizeof *eventosFav,ordem);revisao++;
}
static void favoritosPasso(void) {
  if(atomic_load_explicit(&prontoFav,memory_order_acquire)) {
    pthread_mutex_lock(&trava);EspJob *j=resultadoFav;resultadoFav=NULL;pthread_mutex_unlock(&trava);
    atomic_store_explicit(&prontoFav,0,memory_order_release);ocupadoFav=0;
    if(j && j->geracao==geracaoFav && j->perfil==perfil && nv_perfil_sessao_atual(j->perfil)) {
      if(j->erro && j->tipo==3) {
        j->tipo=4;j->dia=0;j->erro=0;free(j->eventos);j->eventos=NULL;j->n=0;urlFavorito(j);
        filaFav=j;j=NULL; /* Unsupported team schedule: existing league metadata path. */
      } else if(!j->erro) {
        publicarFavoritos(j);
        if(j->tipo==4 && j->dia<7) {
          EspJob *p=calloc(1,sizeof *p);
          if(p){*p=*j;p->eventos=NULL;p->ligas=NULL;p->n=0;p->dia++;urlFavorito(p);filaFav=p;}
          else erroFav=1;
        }
      } else {erroFav=1;revisao++;}
    }
    soltar(j);
  }
  if(!ocupadoFav) {
    EspJob *j=filaFav;filaFav=NULL;if(!j)j=proximoFavorito();
    if(j) {
      pthread_t fio;ocupadoFav=1;
      if(pthread_create(&fio,NULL,buscar,j)){ocupadoFav=0;erroFav=1;soltar(j);revisao++;}
      else pthread_detach(fio);
    }
  }
}
int esportes_favoritos_times(void) {
  int n=0;
  for(int i=0;i<nFavoritos;i++) {
    char mod[32];if(sscanf(favoritos[i],"%31[^|]",mod)!=1)continue;
    int m=modalidadeDe(mod);if(m>=0 && esportes_habilitado(m))n++;
  }
  return n;
}
int esportes_favoritos_n(void){return nEventosFav;}
const EspEvento *esportes_favoritos_evento(int i){return i>=0 && i<nEventosFav?&eventosFav[i]:NULL;}
int esportes_favoritos_carregando(void){return ocupadoFav||filaFav||favoritoAtivo;}
int esportes_favoritos_erro(void){return erroFav;}
