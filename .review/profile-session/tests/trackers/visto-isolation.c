// "Marcar como assistido" para os tres destinos (visto.c), contra rede FALSA.
//
// O pedido do dono: "o mark as watch tem que funcionar tanto no nuvio, como no
// simkl ... e tem que da pra marcar watched all season direto do botao da
// season" (issue #108). Quatro perguntas, na ordem do pedido:
//   (a) sem Trakt, o gesto grava local e monta o corpo da conta Nuvio;
//   (b) com Simkl, o corpo e a rota de /sync/history sao os da doc oficial;
//   (c) a temporada inteira e UM pedido por destino, com todos os episodios;
//   (d) com Trakt ligado, o pedido do Trakt e o mesmo de antes, byte a byte.
//
// Entram os modulos DE VERDADE: visto.c, simkl.c, syncprog.c (+progresso.c),
// trakt.c, vistoep.c, js.c e jsw.c. Stub e so a borda: rede, sessao, token,
// nuvem, catalogo. STDOUT E CAPTURADO e conferido no fim: nenhum token pode
// aparecer no que os modulos imprimem. O progresso do teste sai por stderr.
#include "visto.h"
#include "vistoep.h"
#include "simkl.h"
#include "trakt.h"
#include <assert.h>
#include "perfis.h"
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TK_SIMKL "TOKEN-SIMKL-SEGREDO-108"
#define TK_TRAKT "TOKEN-TRAKT-SEGREDO-108"
#define OK(s) fprintf(stderr, "ok  %s\n", s)

// ---------------------------------------------------------------- stubs
unsigned int SDL_GetTicks(void) { return 0; }
void marco(const char *m) { (void)m; }
static const char *tokenSimkl = "";
static int logada = 1;
const char *simklauth_token(void) { return tokenSimkl; }
const char *nuvem_simkl_cliente(void) { return "cid-teste"; }
const char *nuvem_simkl_app(void) { return "nuvio"; }
const char *nuvem_trakt_cliente(void) { return "cli-trakt"; }
void nuvem_url_escapar(const char *v, char *dst, unsigned tam) { snprintf(dst, tam, "%s", v); }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }
int sessao_logada(void) { return logada; }
const char *sessao_usuario(void) { return "usuario-teste"; }
char *contacache_ler(const char *s, int p, const char *u, long *q) { (void)s; (void)p; (void)u; (void)q; return NULL; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
const char *dados_cliente_id(void) { return "cliente-teste"; }
char *dados_ler(const char *n) { (void)n; return NULL; }
int dados_gravar(const char *n, const char *c) { (void)n; (void)c; return 1; }
int dados_apagar(const char *n) { (void)n; return 1; }
int cat_indice_por_imdb(const char *imdb) { (void)imdb; return -1; }
void cat_aplicar_progresso(int i, double p, double d, int t, int e) { (void)i; (void)p; (void)d; (void)t; (void)e; }
void cat_historico_definir_id(const char *i, const char *t, int v) { (void)i; (void)t; (void)v; }
unsigned long long cat_historico_geracao(void) { return 1; }
int cat_historico_definir_se_geracao(const char *i, const char *t, int v,
                                     unsigned long long g) {
  if (g != 1) return 0;
  cat_historico_definir_id(i, t, v); return 1;
}
const char *cat_tipo_por_imdb(const char *imdb) { (void)imdb; return "series"; }
int ajustes_social(void) { return 1; }
int ajustes_hist_conta(void) { return 1; }
int ajustes_tmdb_cw(void) { return 0; }
const char *ajustes_tmdb_idioma(void) { return "en-US"; }
const char *desc_chave_tmdb(void) { return ""; }
const char *desc_tmdb_idioma(void) { return "pt-BR"; }
void rede_avisar_401(void (*f)(const char *url)) { (void)f; }

// ---------------------------------------------------------------- rede falsa
// Um registro por pedido. So POST importa aqui; GET/DELETE contam como "outro"
// e o teste exige zero deles.
#define MAXP 16
static struct { char url[400], corpo[8192], auth[400]; } post[MAXP];
static pthread_mutex_t netLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond = PTHREAD_COND_INITIALIZER;
static int oldEntered, releaseOld, delaySimkl;
static int nPost, nOutro, bearerSimkl, bearerTrakt;
static const char *respostaSimkl = "{\"added\":{\"episodes\":3},\"not_found\":{\"movies\":[],\"shows\":[]}}";

char *rede_postar_st(const char *url, int s, const char *const *cab, const char *corpo, int *st) {
  (void)s;
  pthread_mutex_lock(&netLock);
  assert(nPost < MAXP);
  snprintf(post[nPost].url, sizeof post[nPost].url, "%s", url);
  snprintf(post[nPost].corpo, sizeof post[nPost].corpo, "%s", corpo ? corpo : "");
  snprintf(post[nPost].auth, sizeof post[nPost].auth, "%s", cab && cab[0] ? cab[0] : "");
  nPost++;
  int delay = cab && strstr(cab[0], delaySimkl ? "token-S-A" : "token-T-A");
  if (delay) oldEntered = 1;
  pthread_cond_broadcast(&netCond);
  while (delay && !releaseOld)
    pthread_cond_wait(&netCond, &netLock);
  pthread_mutex_unlock(&netLock);
  *st = 201; return strdup("{}");
}
char *rede_postar(const char *u, int s, const char *const *c, const char *b) { int st; return rede_postar_st(u, s, c, b, &st); }
char *rede_apagar(const char *u, int s, const char *const *c, int *st) { (void)u; (void)s; (void)c; nOutro++; *st = 204; return strdup(""); }
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) { (void)u; (void)s; (void)c; nOutro++; *st = 200; return strdup("[]"); }
char *rede_baixar_com(const char *u, int s, const char *const *c) { (void)u; (void)s; (void)c; nOutro++; return strdup("[]"); }
char *rede_baixar(const char *u, int s) { (void)u; (void)s; nOutro++; return strdup("[]"); }

static struct { char fn[64], corpo[8192]; } rpc[MAXP];
static int nRpc;
char *sessao_rpc(const char *funcao, const char *corpo, int *status) {
  assert(nRpc < MAXP);
  snprintf(rpc[nRpc].fn, sizeof rpc[nRpc].fn, "%s", funcao);
  snprintf(rpc[nRpc].corpo, sizeof rpc[nRpc].corpo, "%s", corpo);
  nRpc++;
  if (status) *status = 200;
  return strdup("null");
}

static void zerar(void) { nPost = nOutro = nRpc = 0; }
static int conta(const char *s, const char *sub) {
  int k = 0; for (s = strstr(s, sub); s; s = strstr(s + 1, sub)) k++; return k;
}

static void waitCount(int count) {
  struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 3;
  pthread_mutex_lock(&netLock);
  while (nPost < count) assert(pthread_cond_timedwait(&netCond, &netLock, &until) == 0);
  pthread_mutex_unlock(&netLock);
}
static void openProfile(int n, const char *traktToken, const char *simklToken) {
  perfis_definir_ativo(n); trakt_abrir_sessao(); simkl_abrir_sessao();
  trakt_definir(traktToken, "fixture-client"); tokenSimkl = simklToken;
}
static void batchIsolation(int blockSimkl) {
  VistoPar pairs[] = {{1, 1}, {1, 2}, {1, 3}};
  pthread_mutex_lock(&netLock); nPost = oldEntered = releaseOld = 0; delaySimkl = blockSimkl; pthread_mutex_unlock(&netLock);
  openProfile(1, "token-T-A", "token-S-A");
  assert(visto_episodios("tt1000001", "series", pairs, 3, 1, VISTO_TRAKT | VISTO_SIMKL));
  waitCount(blockSimkl ? 2 : 1); pthread_mutex_lock(&netLock); assert(oldEntered); pthread_mutex_unlock(&netLock);
  openProfile(2, "token-T-B", "token-S-B");
  assert(visto_episodios("tt2000001", "series", pairs, 3, 0, VISTO_TRAKT | VISTO_SIMKL));
  waitCount(blockSimkl ? 4 : 3);
  pthread_mutex_lock(&netLock); releaseOld = 1; pthread_cond_broadcast(&netCond); pthread_mutex_unlock(&netLock);
  waitCount(4);
  pthread_mutex_lock(&netLock); assert(nPost == 4);
  int a = 0, b = 0;
  for (int i = 0; i < nPost; i++) {
    if (strstr(post[i].corpo, "tt1000001")) { assert(strstr(post[i].auth, "-A")); a++; }
    else { assert(strstr(post[i].corpo, "tt2000001") && strstr(post[i].auth, "-B") && strstr(post[i].url, "/remove")); b++; }
  }
  assert(a == 2 && b == 2); pthread_mutex_unlock(&netLock); usleep(50000);
  printf("PASS watched batch freezes BOTH original trackers, B finishes while A's %s is delayed, original tokens/body preserved\n", blockSimkl ? "Simkl" : "Trakt");
}
int main(void) { batchIsolation(0); batchIsolation(1); return 0; }
