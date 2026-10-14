#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "perfis.c"
void teste_tokens(const char *,const char *);
void teste_limpar(void);
static int antesN, perfilAntes, trocaServidores, gravacoes;
static uint64_t epocaAntes;
static atomic_int entrou, terminou;
static int codigoRenovacao=200;
static const char *tokA="a.eyJzdWIiOiJ1MSIsImV4cCI6NDEwMjQ0NDgwMH0.a";
static const char *tokB="b.eyJzdWIiOiJ1MiIsImV4cCI6NDEwMjQ0NDgwMH0.b";
void fil_definir_perfil(int p) { assert(p==perfis_ativo()); }
void fontepref_definir_perfil(int p) { assert(p==perfis_ativo()); }
void legmem_definir_perfil(int p) { assert(p==perfis_ativo()); }
void arteesc_definir_perfil(int p) { assert(p==perfis_ativo()); }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) { trocaServidores++; }
int ajustes_addons_do_principal(void) { return 0; }
char *dados_ler(const char *n) { (void)n; return NULL; }
int dados_gravar(const char *n,const char *s) { (void)n;(void)s; gravacoes++; return 1; }
int dados_apagar(const char *n) { (void)n; return 1; }
void dados_uuid(char *s,unsigned n) { snprintf(s,n,"fixed-review-id"); }
const char *i18n(const char *s) { return s; }
int nuvem_pronta(void) { return 1; }
const char *nuvem_url(void) { return "https://review.invalid"; }
const char *nuvem_base_login(void) { return "https://review.invalid"; }
const char *nuvem_ultimo_erro(void) { return ""; }
void nuvem_falhou(void) {}
void nuvem_ok(void) {}
int nuvem_erro_ausente(const char *s) { (void)s; return 0; }
char *nuvem_rpc_com(const char *f,const char *j,const char *b,int *s) { (void)f;(void)j;(void)b; *s=200; return strdup("[]"); }
char *nuvem_tabela(const char *f,const char *j,const char *b,int *s) { return nuvem_rpc_com(f,j,b,s); }
char *nuvem_post(const char *p,const char *j,const char *b,int *s) {
  (void)j;(void)b; assert(strstr(p,"refresh_token")); *s=codigoRenovacao;
  if(*s!=200) return strdup("{\"error\":\"controlled refusal\"}");
  char body[500]; snprintf(body,sizeof body,"{\"access_token\":\"%s\",\"refresh_token\":\"new-refresh\"}",tokA); return strdup(body);
}
static void *antesSonda(void *u) {
  (void)u;
  assert(!pthread_mutex_trylock(&sessaoTrava));
  assert(perfis_ativo()==perfilAntes && perfis_sessao_geracao()==epocaAntes);
  pthread_mutex_unlock(&sessaoTrava); return NULL;
}
static void antes(void) {
  antesN++; assert(perfis_ativo()==perfilAntes && perfis_sessao_geracao()==epocaAntes);
  pthread_once(&sessaoUmaVez,sessaoTravaIniciar);
  // A separate thread proves the hook runs outside the publication gate.
  pthread_t fio; assert(!pthread_create(&fio,NULL,antesSonda,NULL)); assert(!pthread_join(fio,NULL));
}
static void *invalidarFio(void *u) { (void)u; atomic_store(&entrou,1); perfis_invalidar_sessao(); atomic_store(&terminou,1); return NULL; }
static void definir(int p) { perfilAntes=perfis_ativo(); epocaAntes=perfis_sessao_geracao(); perfis_definir_ativo(p); assert(!perfis_sessao_atual(epocaAntes)); }
int main(void) {
  uint64_t inicial=perfis_sessao_geracao(); assert(inicial && perfis_ativo()==1);
  perfis_ao_abrir_sessao(antes); definir(2); assert(antesN==1 && trocaServidores==1);
  definir(2); assert(antesN==2 && trocaServidores==1);
  uint64_t a=perfis_sessao_geracao(); definir(3); definir(2); assert(!perfis_sessao_atual(a));
  int nAntes=antesN; perfis_definir_ativo(0); assert(antesN==nAntes);
  a=perfis_sessao_geracao(); assert(perfis_sessao_travar(a)); assert(perfis_sessao_travar(a)); perfis_sessao_destravar();
  pthread_t fio; assert(!pthread_create(&fio,NULL,invalidarFio,NULL));
  while(!atomic_load(&entrou)) { struct timespec t={0,1000000}; nanosleep(&t,NULL); }
  struct timespec espera={0,20000000}; nanosleep(&espera,NULL); assert(!atomic_load(&terminou));
  perfis_sessao_destravar(); assert(!pthread_join(fio,NULL)); assert(!perfis_sessao_travar(a));
  atomic_store(&sessaoGeracao,UINT64_MAX); perfis_invalidar_sessao(); assert(perfis_sessao_geracao()==1);
  a=perfis_sessao_geracao(); perfis_esquecer(); assert(!perfis_sessao_atual(a) && perfis_ativo()==1);
  perfis_ao_abrir_sessao(NULL); nAntes=antesN;
  a=perfis_sessao_geracao(); teste_tokens(tokA,"refresh-a"); assert(!perfis_sessao_atual(a));
  a=perfis_sessao_geracao(); teste_tokens(tokA,"refresh-new-same-account"); assert(perfis_sessao_atual(a));
  teste_tokens(tokB,"refresh-b"); teste_tokens(tokA,"refresh-a"); assert(!perfis_sessao_atual(a));
  a=perfis_sessao_geracao(); sessao_token_recusado("old-token"); assert(perfis_sessao_atual(a));
  a=perfis_sessao_geracao(); sessao_token_recusado(tokA); assert(!perfis_sessao_atual(a)); assert(sessao_logada());
  codigoRenovacao=400; a=perfis_sessao_geracao(); sessao_token_recusado(tokA); assert(!perfis_sessao_atual(a) && !sessao_logada());
  teste_tokens(tokA,"refresh-a"); a=perfis_sessao_geracao(); teste_limpar(); assert(!perfis_sessao_atual(a));
  a=perfis_sessao_geracao(); teste_limpar(); assert(perfis_sessao_atual(a)); assert(antesN==nAntes);
  puts("standalone-epoch: actual strong producer, hook ordering, same profile, profile/account ABA, recursive gate, zero skip and token boundaries PASS");
}
