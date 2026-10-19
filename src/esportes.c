#define _POSIX_C_SOURCE 200809L
#include "esportes.h"
#include "dados.h"
#include "perfis.h"
#include "sessao.h"
#include "perfisessao.h"
#include "rede.h"
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
  char url[500], arquivo[160];
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
void esportes_cancelar(void) { geracao++; soltar(fila); fila = NULL; }
void esportes_abrir_sessao(void) {
  NvPerfilSessao atual = nv_perfil_sessao_ler();
  if (arquivo[0] && atual == perfil) return;
  esportes_cancelar(); perfil = atual; desligados = 0; nFavoritos = 0;
  free(ligas); ligas = NULL; free(eventos); eventos = NULL;
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
  return 1;
}
static int chaveFav(const EspEvento *e, int p, char *out, size_t n) {
  if (!e || p < 0 || p >= e->nParticipantes || !e->participantes[p].id[0]) return 0;
  snprintf(out, n, "%s|%s|%s", e->modalidade, e->ligaId, e->participantes[p].id); return 1;
}
int esportes_favorito(const EspEvento *e, int p) {
  char c[200]; if (!chaveFav(e, p, c, sizeof c)) return 0;
  for (int i = 0; i < nFavoritos; i++) if (!strcmp(c, favoritos[i])) return 1;
  return 0;
}
int esportes_alternar_favorito(const EspEvento *e, int p) {
  char c[200]; if (!chaveFav(e, p, c, sizeof c)) return 0;
  int i; for (i = 0; i < nFavoritos; i++) if (!strcmp(c, favoritos[i])) break;
  if (i < nFavoritos) {
    char ultimo[200]; memcpy(ultimo, favoritos[nFavoritos-1], sizeof ultimo);
    memcpy(favoritos[i], ultimo, sizeof ultimo); nFavoritos--;
    if (!gravar()) { memcpy(favoritos[nFavoritos++], ultimo, sizeof ultimo); snprintf(favoritos[i], sizeof favoritos[i], "%s", c); return 0; }
  } else {
    if (nFavoritos >= 128) return 0;
    snprintf(favoritos[nFavoritos++], sizeof favoritos[0], "%s", c);
    if (!gravar()) { nFavoritos--; return 0; }
  }
  revisao++; return 1;
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
  pthread_mutex_lock(&trava); resultado = j; pthread_mutex_unlock(&trava);
  atomic_store_explicit(&pronto, 1, memory_order_release); return NULL;
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
  snprintf(j->url, sizeof j->url, "https://site.api.espn.com/apis/site/v2/leagues/dropdown?lang=en&region=us&sport=%s", esportes_modalidade(m)->id);
  snprintf(j->arquivo, sizeof j->arquivo, "esportes-ligas-%s.json", esportes_modalidade(m)->id);
  free(ligas); ligas = NULL; nLigas = 0; revisao++; agendar(j);
}
void esportes_pedir_eventos(int m, const EspLiga *l, int atualizar) {
  if (!l || !esportes_habilitado(m)) return;
  EspJob *j = calloc(1, sizeof *j); if (!j) { erro = 1; return; }
  j->tipo = 2; j->modalidade = m; j->atualizar = atualizar; j->liga = *l;
  snprintf(j->url, sizeof j->url, "https://site.api.espn.com/apis/site/v2/sports/%s/%s/scoreboard?limit=%d", esportes_modalidade(m)->id, l->id, ESP_MAX_EVENTOS);
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
        if (!erro && !j->atualizar && j->dia < 6 && nEventos < ESP_MAX_EVENTOS) {
          EspJob *proximo=calloc(1,sizeof *proximo);
          if (proximo) {
            *proximo=*j; proximo->ligas=NULL; proximo->eventos=NULL; proximo->n=0; proximo->dia++;
            char dia[16]; struct tm tm; time_t t=time(NULL)+proximo->dia*86400;
            gmtime_r(&t,&tm); strftime(dia,sizeof dia,"%Y%m%d",&tm);
            snprintf(proximo->url,sizeof proximo->url,"https://site.api.espn.com/apis/site/v2/sports/%s/%s/scoreboard?dates=%s&limit=%d",esportes_modalidade(j->modalidade)->id,j->liga.id,dia,ESP_MAX_EVENTOS);
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
}
int esportes_carregando(void) { return ocupado || fila != NULL; }
int esportes_erro(void) { return erro; }
unsigned esportes_revisao(void) { return revisao; }
int esportes_ligas_n(void) { return nLigas; }
const EspLiga *esportes_liga(int i) { return i >= 0 && i < nLigas ? &ligas[i] : NULL; }
int esportes_eventos_n(void) { return nEventos; }
const EspEvento *esportes_evento(int i) { return i >= 0 && i < nEventos ? &eventos[i] : NULL; }
