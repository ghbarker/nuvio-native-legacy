// Review-only: actual plugins.c, JSON parser and stream parser; bounded HTTP/JS doubles.
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "plugins.c"

struct RedeGrupo { atomic_ullong versao; };
struct RedeJob { RedeGrupo *grupo; uint64_t versao; };
static uint64_t perfilSessao = 7;
static int perfilAtual=1;
static char usuarioAtual[80]="owner-a";
static char chaveTmdbTeste[64] = "tmdb-owner-a";
static DebridContexto debridTeste;
static pthread_mutex_t contextoTrava=PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t tlsDebrid;
static pthread_once_t tlsUma = PTHREAD_ONCE_INIT;
static atomic_int gravacoes, pedidos, execucoes, entrando, bloqueia, libera, cancelar;
static int modo;
static int temporadaEsperada=2, episodioEsperado=8;
static int jobs;
static OrigemExtra registrada;
static const OrigemPreparoOps *opsRegistradas;
static void tlsCriar(void) { assert(!pthread_key_create(&tlsDebrid, NULL)); }
#ifndef PLUGINS_WEAK_SESSION
uint64_t perfis_sessao_geracao(void) { return __atomic_load_n(&perfilSessao, __ATOMIC_ACQUIRE); }
int perfis_sessao_atual(uint64_t n) { return n == perfis_sessao_geracao(); }
int perfis_sessao_travar(uint64_t n) { return perfis_sessao_atual(n); }
void perfis_sessao_destravar(void) {}
#elif defined(__APPLE__)
// Strict Mach-O closure: exercise the same stable-zero compatibility behavior
// as the absent producer on ELF, without unresolved-link flags. The strong
// fixture above retains its checked profile epoch transition coverage.
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t n) { return n == 0; }
int perfis_sessao_travar(uint64_t n) { return n == 0; }
void perfis_sessao_destravar(void) {}
#endif
void debrid_contexto_capturar(DebridContexto *c) { pthread_mutex_lock(&contextoTrava); *c = debridTeste; pthread_mutex_unlock(&contextoTrava); }
void debrid_contexto_entrar(const DebridContexto *c) { pthread_once(&tlsUma, tlsCriar); assert(!pthread_setspecific(tlsDebrid, c)); }
void debrid_contexto_sair(void) { assert(!pthread_setspecific(tlsDebrid, NULL)); }
const char *desc_chave_tmdb_reserva(void) { return chaveTmdbTeste; }
int perfis_ativo_addons(void) { return perfilAtual; }
const char *sessao_usuario(void) { return usuarioAtual; }
RedeGrupo *rede_grupo_criar(void) { RedeGrupo *g = calloc(1, sizeof *g); assert(g); atomic_init(&g->versao, 1); return g; }
uint64_t rede_grupo_avancar(RedeGrupo *g) { return atomic_fetch_add(&g->versao, 1) + 1; }
RedeJob *rede_job_criar(RedeGrupo *g) { RedeJob *j = malloc(sizeof *j); assert(j); j->grupo=g; j->versao=atomic_load(&g->versao); jobs++; return j; }
void rede_job_soltar(RedeJob *j) { if(j) { jobs--; free(j); } }
RedeErro rede_job_estado(RedeJob *j) { return j && j->versao == atomic_load(&j->grupo->versao) ? REDE_OK : REDE_GERACAO; }
void rede_resposta_limpar(RedeResposta *r) { free(r->corpo); free(r->cabecalhos); memset(r, 0, sizeof *r); }
unsigned plugrede_capacidades(void) { return PR_CAP_REDE | PR_CAP_ABORTAR; }
int dados_gravar(const char *n, const char *c) { (void)n; (void)c; atomic_fetch_add(&gravacoes, 1); return 1; }
int dados_gravar_leve(const char *n, const char *c) { return dados_gravar(n,c); }
char *dados_ler(const char *n) { (void)n; return NULL; }
int dados_apagar(const char *n) { (void)n; return 1; }
void addons_definir_origem_extra(OrigemExtra f, int (*a)(void)) { assert(a == plugins_ligado); registrada=f; }
void addons_definir_origem_extra_preparo(OrigemExtra f, const OrigemPreparoOps *ops) { assert(f==registrada); opsRegistradas=ops; }
int plugrede_pedir(const RedePedido *q, RedeResposta *r) {
  memset(r,0,sizeof *r); atomic_fetch_add(&pedidos,1); assert(q->job && rede_job_estado(q->job)==REDE_OK);
  const char *texto;
  if (strstr(q->url,"/find/")) {
    assert(strstr(q->url,"api_key=tmdb-owner-a"));
    if (modo==4) return 0;
    if(modo==7) atomic_store(&cancelar,1);
    texto=(modo==3||modo==7) ? "{\"tv_results\":[],\"movie_results\":[]}" : "{\"tv_results\":[{\"id\":77}],\"movie_results\":[{\"id\":88}]}";
    if(modo==14) texto="{\"nested\":{\"tv_results\":[]}}";
  } else {
    assert(strstr(q->url,"https://repo-a.invalid/"));
    if(modo==5 && strstr(q->url,"second.js")) return 0;
    texto="function getStreams(){ return []; } // bounded transport fixture";
  }
  r->status=200; r->corpo=strdup(texto); r->n_corpo=strlen(texto); return 1;
}
size_t pj_pilha(void) { return 2u * 1024u * 1024u; }
int pj_executar(const PjPedido *p, PjResultado *r) {
  memset(r,0,sizeof *r); atomic_fetch_add(&execucoes,1);
  const DebridContexto *d=pthread_getspecific(tlsDebrid);
  assert(d && !strcmp(d->chave[1],"torbox-owner-a"));
  assert(d->sessao==7 && d->busca==41);
  assert(d->temporada==p->temporada && d->episodio==p->episodio);
  assert(p->temporada==temporadaEsperada && p->episodio==episodioEsperado);
  assert(!strcmp(p->tmdbId,temporadaEsperada?"77":"88"));
  assert(!strcmp(p->tmdbChave,"tmdb-owner-a"));
  assert(p->grupo && p->job && p->prazoMs>0 && p->redeSegundos>0);
  assert(p->nCodigo>20 && !strcmp(p->ajustesJson,"{}"));
  atomic_fetch_add(&entrando,1);
  while(atomic_load(&bloqueia) && !atomic_load(&libera)) {
    if(__atomic_load_n(p->cancelado,__ATOMIC_ACQUIRE) || rede_job_estado(p->job)!=REDE_OK) { r->cancelado=1; return 0; }
    struct timespec t={0,1000000}; nanosleep(&t,NULL);
  }
  // Force opposite completion order; aggregation must retain the captured order.
  if(!strcmp(p->idScraper,"first")) { struct timespec t={0,5000000}; nanosleep(&t,NULL); }
  if(modo==2 && !strcmp(p->idScraper,"second")) { strcpy(r->erro,"controlled failure"); return 0; }
  if(modo==6 && !strcmp(p->idScraper,"second")) return 0;
  if(modo==1) { r->json=strdup("{\"streams\":[]}"); return 0; }
  if(modo>=8 && modo<=13) {
    const char *invalidos[]={"<html>temporary error</html>","{\"streams\":\"bad\"}","{\"nested\":{\"streams\":[]}}","{\"streams\":[", "{\"streams\":[1]}","{}"};
    r->json=strdup(invalidos[modo-8]); return 0;
  }
  char json[1400];
  snprintf(json,sizeof json,"{\"streams\":[{\"name\":\"%s 1080p\",\"url\":\"https://video.invalid/%s.mp4\",\"behaviorHints\":{\"bingeGroup\":\"series-group\",\"proxyHeaders\":{\"request\":{\"X-Test\":\"owner-a\"}}}}]}",p->idScraper,p->idScraper);
  r->json=strdup(json); return 1;
}
void pj_resultado_soltar(PjResultado *r) { free(r->json); memset(r,0,sizeof *r); }
static void resetar(void) {
  assert(!jobs); memset(&E,0,sizeof E); memset(tmCache,0,sizeof tmCache); tmProx=0;
  ctxPerfil=1; strcpy(ctxUsuario,"owner-a"); E.ligado=1; ligadoAt=1;
  perfilAtual=1; strcpy(usuarioAtual,"owner-a");
  strcpy(chaveTmdbTeste,"tmdb-owner-a"); perfilSessao=7;
  memset(&debridTeste,0,sizeof debridTeste); strcpy(debridTeste.chave[1],"torbox-owner-a");
  debridTeste.sessao=7; debridTeste.busca=41; debridTeste.temporada=2; debridTeste.episodio=7;
  E.nRepos=1; E.repos[0].r.ativo=1; strcpy(E.repos[0].r.url,"https://repo-a.invalid/manifest.json");
  E.nScr=2;
  for(int i=0;i<2;i++) { Scraper *s=&E.scr[i]; snprintf(s->id,sizeof s->id,"%s",i?"second":"first");
    snprintf(s->nome,sizeof s->nome,"%s scraper",s->id); snprintf(s->arquivo,sizeof s->arquivo,"%s.js",s->id);
    snprintf(s->url,sizeof s->url,"https://repo-a.invalid/%s.js",s->id); s->ativo=s->serie=s->filme=1; s->repo=0; }
  modo=0; temporadaEsperada=2; episodioEsperado=8; atomic_store(&gravacoes,0); atomic_store(&pedidos,0); atomic_store(&execucoes,0); atomic_store(&entrando,0);
  atomic_store(&bloqueia,0); atomic_store(&libera,0); atomic_store(&cancelar,0);
  unsetenv("NUVIO_PLUGINS_SO"); unsetenv("NUVIO_PLUGINS_LOG");
}
static int cancela(void *u) { (void)u; return atomic_load(&cancelar); }
static void semMutacao(const Estado *antes,const TmC *tmAntes) { assert(!memcmp(antes,&E,sizeof E)); assert(!memcmp(tmAntes,tmCache,sizeof tmCache)); assert(!atomic_load(&gravacoes)); }
static void basico(void) {
  resetar(); plugins_ligar_aos_addons(); assert(opsRegistradas && registrada);
  PluginsConsulta *c=opsRegistradas->capturar(); assert(c && opsRegistradas->atual(c));
  Estado antes=E; TmC tmAntes[64]; memcpy(tmAntes,tmCache,sizeof tmAntes);
  Stream *s=NULL; int n=opsRegistradas->consultar(c,"tmdb:77:2:8","series",NULL,NULL,NULL,NULL,&s);
  assert(n==2 && s); assert(strstr(s[0].url,"first.mp4") && strstr(s[1].url,"second.mp4"));
  assert(!strcmp(s[0].provedor,"first scraper") && !strcmp(s[0].bingeGroup,"series-group"));
  assert(strstr(s[0].cabecalhos,"X-Test: owner-a"));
  assert(debridTeste.episodio==7); free(s); semMutacao(&antes,tmAntes); opsRegistradas->liberar(c); assert(!jobs);
  assert(!pthread_getspecific(tlsDebrid));
}
static void vaziosFalhas(void) {
  for(int m=1;m<=6;m++) { resetar(); modo=m; PluginsConsulta *c=plugins_consulta_capturar(); assert(c);
    Estado antes=E; TmC tmAntes[64]; memcpy(tmAntes,tmCache,sizeof tmAntes); Stream *s=NULL;
    int n=plugins_consulta_consultar(c,(m==3||m==4)?"tt123:2:8":"tmdb:77:2:8","series",NULL,NULL,NULL,NULL,&s);
    assert(n==((m==1||m==3)?0:-1)); assert(!s); semMutacao(&antes,tmAntes); plugins_consulta_liberar(c); assert(!jobs); }
  resetar(); E.scr[0].serie=E.scr[1].serie=0; PluginsConsulta *c=plugins_consulta_capturar(); Stream *s=NULL;
  assert(!plugins_consulta_consultar(c,"tt123:2:8","series",NULL,NULL,NULL,NULL,&s)); assert(!atomic_load(&pedidos));
  assert(!plugins_consulta_consultar(c,"kitsu:4","anime",NULL,NULL,NULL,NULL,&s)); plugins_consulta_liberar(c);
  for(int m=8;m<=14;m++) { resetar(); modo=m; c=plugins_consulta_capturar();
    assert(plugins_consulta_consultar(c,m==14?"tt123:2:8":"tmdb:77:2:8","series",NULL,NULL,NULL,NULL,&s)==-1);
    assert(!s); plugins_consulta_liberar(c); assert(!jobs); assert(!atomic_load(&gravacoes)); }
}
static void contextoIMDbFilme(void) {
  resetar(); strcpy(tmCache[0].imdb,"tt123"); tmCache[0].tmdb=77; tmCache[0].serie=1;
  PluginsConsulta *c=plugins_consulta_capturar(); tmCache[0].tmdb=99; Stream *s=NULL;
  temporadaEsperada=3; episodioEsperado=9;
  assert(plugins_consulta_consultar(c,"tt123:3:9","series",NULL,NULL,NULL,NULL,&s)==2); assert(atomic_load(&pedidos)==2); free(s);
  temporadaEsperada=episodioEsperado=0;
  assert(plugins_consulta_consultar(c,"tmdb:88","movie",NULL,NULL,NULL,NULL,&s)==2); free(s); plugins_consulta_liberar(c);
  resetar(); modo=7; c=plugins_consulta_capturar();
  assert(plugins_consulta_consultar(c,"tt123:2:8","series",cancela,NULL,NULL,NULL,&s)==-1); assert(!s); plugins_consulta_liberar(c);
}
static void invalidacoes(void) {
  for(int i=0;i<8;i++) { resetar(); PluginsConsulta *c=plugins_consulta_capturar(); assert(plugins_consulta_atual(c));
    switch(i) {
      case 0: assert(!plugins_alternar_scraper(0,0)); break;
      case 1: assert(!plugins_alternar_repo(0)); break;
      case 2: plugins_definir_ligado(0); break;
      case 3: assert(aplicarManifesto(&E,0,"{\"scrapers\":[{\"id\":\"first\",\"filename\":\"updated.js\"}]}",E.repos[0].r.url)==1); break;
      case 4: strcpy(chaveTmdbTeste,"tmdb-owner-b"); break;
      case 5: strcpy(debridTeste.chave[1],"torbox-owner-b"); break;
      case 6:
#ifdef PLUGINS_WEAK_SESSION
        perfilAtual=2; plugins_perfil_mudou();
#else
        perfilSessao++;
#endif
        break;
      case 7: strcpy(usuarioAtual,"owner-b"); plugins_perfil_mudou(); strcpy(usuarioAtual,"owner-a"); plugins_perfil_mudou(); break;
    }
    assert(!plugins_consulta_atual(c)); Stream *s=NULL; assert(plugins_consulta_consultar(c,"tmdb:77:2:8","series",NULL,NULL,NULL,NULL,&s)==-1);
    assert(!s && !atomic_load(&execucoes)); plugins_consulta_liberar(c); assert(!jobs);
  }
  resetar(); PluginsConsulta *c=plugins_consulta_capturar(); debridTeste.episodio=8; assert(plugins_consulta_atual(c)); plugins_consulta_liberar(c);
}
typedef struct { PluginsConsulta *c; int n; Stream *s; } ConsultaTeste;
static void *consultarFio(void *u) { ConsultaTeste *t=u; t->n=plugins_consulta_consultar(t->c,"tmdb:77:2:8","series",cancela,NULL,NULL,NULL,&t->s); return NULL; }
static void cancelamento(void) {
  for(int i=0;i<4;i++) { resetar(); atomic_store(&bloqueia,1); ConsultaTeste t={.c=plugins_consulta_capturar()}; pthread_t f;
    assert(!pthread_create(&f,NULL,consultarFio,&t));
    for(int k=0;k<1000 && !atomic_load(&entrando);k++) { struct timespec ts={0,1000000}; nanosleep(&ts,NULL); }
    assert(atomic_load(&entrando));
    if(i==0) atomic_store(&cancelar,1);
    if(i==1) { pthread_mutex_lock(&trava); E.scr[0].ativo=0; pthread_mutex_unlock(&trava); }
    if(i==2) gerAvancar();
    if(i==3) { pthread_mutex_lock(&contextoTrava); strcpy(debridTeste.chave[1],"torbox-owner-b"); pthread_mutex_unlock(&contextoTrava); }
    assert(!pthread_join(f,NULL)); assert(t.n==-1 && !t.s); plugins_consulta_liberar(t.c); assert(!jobs); assert(!atomic_load(&gravacoes));
  }
}
int main(void) {
  basico(); vaziosFalhas(); contextoIMDbFilme(); invalidacoes(); cancelamento();
  assert(!jobs); free(grupo); grupo=NULL;
  puts("plugins-snapshot: actual query, ordering, TLS, empties, failures, actual account/master/scraper/repository setters and cancellation PASS"); return 0;
}
