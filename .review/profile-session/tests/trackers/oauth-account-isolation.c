// Actual OAuth modules, controlled worker dispatch, fake accounts/network/disk.
// No HTTP is held while switching profile/account. Fake tokens only.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "sync.h"
static void *(*job)(void *);
static int testCreate(pthread_t *t, const pthread_attr_t *a, void *(*f)(void *), void *u) {
  (void)t; (void)a; (void)u; assert(!job); job=f; return 0;
}
static int testDetach(pthread_t t) { (void)t; return 0; }
#define pthread_create testCreate
#define pthread_detach testDetach
#ifdef TEST_TRAKT
#include "traktauth.c"
#define authForget traktauth_esquecer
#define authLoad traktauth_carregar_perfil
#define authBegin traktauth_comecar
#define authStep traktauth_passo
#define authState traktauth_estado
#define authCode traktauth_codigo
#define WAITING TRA_AGUARDANDO
#define EMPTY TRA_PARADO
#define BANK "trakt-p1.txt"
#else
#include "simklauth.c"
#define authForget simklauth_esquecer
#define authLoad simklauth_carregar_perfil
#define authBegin simklauth_comecar
#define authStep simklauth_passo
#define authState simklauth_estado
#define authCode simklauth_codigo
#define WAITING SMK_AGUARDANDO
#define EMPTY SMK_PARADO
#define BANK "simkl-p1.txt"
#endif
#undef pthread_create
#undef pthread_detach
static char account[80]="ACCOUNT-A";
static struct { char name[48]; char *body; } files[40];
static int pushes, defines, network, identityReads;
int sessao_identidade_local_copiar(char *u,size_t nu,char *j,size_t nj) {
  identityReads++; snprintf(u,nu,"%s",account); snprintf(j,nj,"FAKE-JWT"); return account[0]!=0;
}
const char *sessao_usuario(void) { return account; }
int sessao_logada(void) { return account[0]!=0; }
SyncEstado sync_estado(void) { return SYNC_PRONTO; }
const char *i18n(const char *s) { return s; }
int credfio_iniciar(const char *p,const char *j) { (void)p; (void)j; pushes++; return 1; }
int credfio_resultado(const char *p,int *r) { (void)p; (void)r; return 0; }
static int find(const char *n) {
  int i; for(i=0;i<40;i++) if(files[i].body && !strcmp(files[i].name,n)) return i; return -1;
}
char *dados_ler(const char *n) { int i=find(n); return i<0?NULL:strdup(files[i].body); }
int dados_gravar(const char *n,const char *b) {
  int i=find(n); if(i<0) for(i=0;i<40 && files[i].body;i++); assert(i<40);
  free(files[i].body); snprintf(files[i].name,48,"%s",n); files[i].body=strdup(b); return 1;
}
int dados_apagar(const char *n) { int i=find(n); if(i<0)return 0; free(files[i].body);files[i].body=NULL;return 1; }
static int contains(const char *n,const char *s) { int i=find(n);return i>=0 && strstr(files[i].body,s)!=NULL; }
#ifdef TEST_TRAKT
const char *nuvem_trakt_cliente(void) { return "FAKE-CLIENT"; }
const char *nuvem_trakt_segredo(void) { return "FAKE-SECRET"; }
static int active;
int trakt_definir(const char *t,const char *c) { (void)t; (void)c; active=1; defines++;return 1; }
void trakt_esquecer(void) { active=0; }
int trakt_ativo(void) {return active;}
int trakt_recusada(void) {return 0;}
void trakt_sessao_morta(void) {}
void desc_repetir(void) {}
char *rede_postar_st(const char *u,int s,const char *const *h,const char *b,int *st) {
  (void)s;(void)h;(void)b; network++; *st=200;
  if(strstr(u,"/device/code")) return strdup("{\"device_code\":\"DEVICE\",\"user_code\":\"USER\",\"verification_url\":\"https://fake/activate\",\"expires_in\":600,\"interval\":1}");
  return strdup("{\"access_token\":\"OWNED-TOKEN\",\"refresh_token\":\"ROTATED-REFRESH\",\"created_at\":1791921600,\"expires_in\":7776000}");
}
#else
const char *nuvem_simkl_cliente(void) {return "FAKE-CLIENT";}
const char *nuvem_simkl_app(void) {return "FAKE-APP";}
void nuvem_url_escapar(const char *v,char *d,unsigned n) {snprintf(d,n,"%s",v);}
char *rede_baixar_st(const char *u,int s,const char *const *h,int *st) {
  (void)s;(void)h;network++;*st=200;
  if(strstr(u,"/oauth/pin/"))return strdup("{\"access_token\":\"OWNED-TOKEN\"}");
  return strdup("{\"user_code\":\"USER\",\"verification_url\":\"https://fake/pin\",\"expires_in\":900}");
}
#endif
static unsigned clockMs=1000;
static void runJob(void) { void *(*f)(void *)=job; assert(f);job=NULL;f(NULL);assert(fioPronto); }
static void step(void) { clockMs+=70000;authStep(clockMs); }
static void reset(void) {
  assert(!job); if(fioVivo)step();authForget();snprintf(account,80,"ACCOUNT-A");authLoad(1);
}
static void pollReady(void) {
  int calls=network,reads=identityReads;
  authBegin();assert(job);assert(network==calls && identityReads>reads);runJob();
  assert(authState()==WAITING && authCode()[0]);step();assert(job);
}
int main(void) {
  reset();pollReady();authLoad(2);runJob();step();
  assert(contains(BANK,"OWNED-TOKEN"));assert(!token[0]);assert(authState()==EMPTY);
  puts("ok: delayed OAuth completion stays in original profile within same account");

  reset();pollReady();snprintf(account,80,"ACCOUNT-B");authForget();authLoad(2);runJob();step();
  assert(find(BANK)<0 && !token[0] && authState()==EMPTY);
  puts("ok: old-account OAuth completion cannot recreate bank after logout");

  reset();authBegin();assert(job);snprintf(account,80,"ACCOUNT-B");authForget();authLoad(2);runJob();step();
  assert(!authCode()[0] && authState()==EMPTY && find(BANK)<0);
#ifdef TEST_TRAKT
  assert(find("trakt-fluxo-p1.txt")<0 && find("trakt-fluxo-p2.txt")<0);
#endif
  puts("ok: late device/PIN code cannot appear or persist in new account");

  reset();pollReady();runJob();assert(tokenNovo);int oldPushes=pushes,oldDefines=defines;
  snprintf(account,80,"ACCOUNT-B");step();assert(pushes==oldPushes && defines==oldDefines && find(BANK)<0);
  puts("ok: deferred token publication/store/push rejects account replacement");

#ifdef TEST_TRAKT
  reset();dados_gravar(BANK,"EXPIRED\tFAKE-CLIENT\tOLD-REFRESH\t1\t2\t0\n");authLoad(1);step();assert(job);
  authLoad(2);runJob();step();assert(contains(BANK,"ROTATED-REFRESH") && !token[0]);
  puts("ok: rotated refresh survives ordinary profile switch in original bank");
  reset();dados_gravar(BANK,"EXPIRED\tFAKE-CLIENT\tOLD-REFRESH\t1\t2\t0\n");authLoad(1);step();assert(job);
  snprintf(account,80,"ACCOUNT-B");authForget();authLoad(2);runJob();step();assert(find(BANK)<0 && !token[0]);
  puts("ok: rotated refresh is discarded after account replacement");
#endif
  for(int i=0;i<40;i++)free(files[i].body);
  puts("OAuth account isolation passed.");return 0;
}
