/* Actual discovery/catalog/collections/perfis integration. The unchanged
 * montage fixture supplies only network, tracker and row-setting neighbours.
 * Target-cache restoration is represented by its unchanged installed addon;
 * this fixture does not run the full Android app/account-sync pipeline.
 */
#include <pthread.h>
#include <errno.h>
#include <stdatomic.h>
static void *montar(void *);
static int creationHook(pthread_t *,const pthread_attr_t *,void *(*)(void *),void *);
#define pthread_create(t,a,f,u) creationHook(t,a,f,u)
#define main original_montagem_main
#include "reopen-neighbours.h"
#undef main
#undef pthread_create
#include "colfileiras.h"

/* The retained test-app overlay has Hide Watched; this regression exercises
 * the ordinary default-off discovery path on both clean and combined code. */
int ajustes_ocultar_filmes_vistos(void) { return 0; }

static int addonOwner = 1, failureBudget, failedCreates;
int addons_perfil_da_lista(void) { return addonOwner; }
void addons_marcar_da_conta(int p) { addonOwner = p; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
unsigned fil_perfil_geracao(void) { return (unsigned)perfis_sessao_geracao(); }
int fil_conta_dono(const char *u) { assert(!strcmp(u,sessao_usuario())); return 0; }
void fil_colecao_catalogo_removido(const char *key) { (void)key; }
int trakt_enfeitar_lote_sessao(CatItem *itens,int n,uint64_t epoch) {
  if (!perfis_sessao_travar(epoch)) return 0;
  int result = trakt_enfeitar_lote(itens,n);
  perfis_sessao_destravar(); return result;
}
int recomenda_social_mesclar_sessao(CatItem *itens,int n,int max,uint64_t epoch) {
  if (!perfis_sessao_travar(epoch)) return 0;
  int result = recomenda_social_mesclar(itens,n,max);
  perfis_sessao_destravar(); return result;
}
static int creationHook(pthread_t *t,const pthread_attr_t *a,void *(*f)(void *),void *u) {
  if (f==montar && failureBudget>0) { failureBudget--; failedCreates++; return EAGAIN; }
  return pthread_create(t,a,f,u);
}

static const char *collection =
  "{\"collections\":[{\"id\":\"destination\",\"title\":\"Destination collection\",\"folders\":["
  "{\"id\":\"destination-folder\",\"title\":\"Destination folder\",\"sources\":["
  "{\"provider\":\"addon\",\"addonId\":\"app.addon.demo\",\"type\":\"movie\",\"catalogId\":\"cat_a\"}]}]}]}";

static atomic_int accountPaused,accountChecks;
static int waitForAccount(void) {
  if (!sessao_usuario()[0] || !atomic_load(&accountPaused)) return 0;
  atomic_fetch_add(&accountChecks,1);
  return 2;
}
static void finishAccountWait(int before) {
  for(int i=0;i<2000 && atomic_load(&accountChecks)==before;i++)usleep(1000);
  assert(atomic_load(&accountChecks)>before);
  atomic_store(&accountPaused,0);
}

static void openTarget(int perfil) {
  uint64_t before = perfis_sessao_geracao();
  unsigned version = addons_versao();
  perfis_definir_ativo(perfil);
  assert(!perfis_sessao_atual(before));
  col_esquecer_perfil();
  desc_abrir_sessao();
  addons_marcar_da_conta(perfil);
  assert(addons_versao()==version && addons_perfil_da_lista()==perfil);
  assert(colfileiras_receber(collection)==1 && col_n()==1);
  desc_remontar_fileiras();
}

static pthread_mutex_t barrier=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrierCv=PTHREAD_COND_INITIALIZER;
static int holdOld,oldArrived,releaseOld;
static void holdAtCW(const char *name) {
  if (strcmp(name,"trakt continuar assistindo")) return;
  pthread_mutex_lock(&barrier);
  if (holdOld) {
    holdOld=0; oldArrived=1; pthread_cond_broadcast(&barrierCv);
    while(!releaseOld) pthread_cond_wait(&barrierCv,&barrier);
  }
  pthread_mutex_unlock(&barrier);
}

int main(int argc,char **argv) {
  int bootFirst = argc>1 && !strcmp(argv[1],"boot-first");
  int signedOut = argc>1 && !strcmp(argv[1],"signedout-bootstrap");
  snprintf(dirDados,sizeof dirDados,"/tmp/nuvio-profile-reopen-XXXXXX");
  assert(mkdtemp(dirDados));
  perfis_definir_ativo(1);
  desc_tmdb(dirDados); homeestado_iniciar();
  desc_espera_addons_definir(waitForAccount);
  if (signedOut) {
    __atomic_store_n(&fixtureDono,"",__ATOMIC_RELEASE);
    desc_iniciar(); esperarQuieto();
    assert(montagens==1 && !desc_montando());
    __atomic_store_n(&fixtureDono,"dono",__ATOMIC_RELEASE);
    openTarget(1); atomic_store(&accountPaused,1);
    desc_sessao_preparada(); finishAccountWait(0); esperarQuieto();
    assert(montagens==2 && cat_n()>0 && !desc_montando());
    puts("PASS: signed-out main bootstrap completes; subsequent signed-in opening starts exactly once");
    return 0;
  }
  desc_abrir_sessao();
  if (!bootFirst) { desc_sessao_preparada(); assert(montagens==0 && !buscando); }
  atomic_store(&accountPaused,1);
  desc_iniciar(); finishAccountWait(0); esperarQuieto();
  assert(montagens==1 && cat_n()>0 && !desc_montando());
  desc_sessao_preparada(); desc_sessao_passo(); usleep(20000);
  assert(montagens==1 && !desc_montando());
  puts("PASS: bootstrap/prepared ordering starts one discovery; ordinary sync ticks do not repeat it");

  // Completed A, unchanged B addons and a successful B collection snapshot.
  int oldRequests=pedidosCatalogo;
  openTarget(2);
  assert(cat_n()==0 && desc_montando());
  desc_iniciar(); // restart attempts before source restoration stay deferred
  assert(!buscando && montagens==1);
  int checksBefore=atomic_load(&accountChecks);
  atomic_store(&accountPaused,1);
  desc_sessao_preparada();
  for(int i=0;i<2000 && atomic_load(&accountChecks)==checksBefore;i++)usleep(1000);
  assert(atomic_load(&accountChecks)>checksBefore && pedidosCatalogo==oldRequests);
  atomic_store(&accountPaused,0); esperarQuieto();
  assert(montagens==2 && cat_n()>0 && col_n()==1 && pedidosCatalogo>oldRequests);
  desc_sessao_preparada(); desc_sessao_passo(); usleep(20000);
  assert(montagens==2 && !desc_montando());
  puts("PASS: B starts with unchanged cached addons/current collection; duplicate prepared ticks are coalesced");
  puts("PASS: B independently waits for its own account sync before requesting catalogues");

  openTarget(2); desc_sessao_preparada(); esperarQuieto();
  assert(montagens==3 && cat_n()>0 && !desc_montando());
  openTarget(1); desc_sessao_preparada(); esperarQuieto();
  assert(montagens==4 && cat_n()>0 && !desc_montando());
  puts("PASS: confirming the same profile and returning A -> B -> A each fetch the fresh session");

  // Hold an actual old montage after its CW assembly, with contTrava held.
  // Fresh reset/collection publication must finish while it remains blocked.
  pthread_mutex_lock(&barrier);holdOld=1;oldArrived=releaseOld=0;pthread_mutex_unlock(&barrier);
  fixtureMarco=holdAtCW;
  desc_repetir();
  pthread_mutex_lock(&barrier);while(!oldArrived)pthread_cond_wait(&barrierCv,&barrier);pthread_mutex_unlock(&barrier);
  assert(montagens==5 && buscando);
  openTarget(2); desc_sessao_preparada(); desc_sessao_passo();
  openTarget(1); desc_sessao_preparada(); desc_sessao_passo();
  assert(montagens==5 && buscando); // original worker is reused, no parallel montage
  pthread_mutex_lock(&barrier);releaseOld=1;pthread_cond_broadcast(&barrierCv);pthread_mutex_unlock(&barrier);
  esperarQuieto(); fixtureMarco=NULL;
  assert(montagens==6 && cat_n()>0 && !desc_montando());
  assert(montagemSessao==perfis_sessao_geracao());
  puts("PASS: old active A worker rejects its result and coalesces A -> B -> A into one current discovery");

  openTarget(2); failureBudget=1;
  desc_sessao_preparada();
  assert(failedCreates==1 && !buscando && desc_montando());
  for(int i=0;i<20;i++)desc_sessao_passo();
  assert(failedCreates==1 && montagens==6 && !buscando);
  usleep(1050000);
  desc_sessao_passo(); esperarQuieto();
  assert(montagens==7 && cat_n()>0 && !desc_montando());
  puts("PASS: failed thread creation keeps the opening pending, retries after cooldown and stops retrying after success");

  openTarget(1);
  __atomic_store_n(&fixtureDono,"",__ATOMIC_RELEASE);
  perfis_invalidar_sessao();
  desc_sessao_passo();
  assert(!buscando && !desc_montando() && montagens==7);
  puts("PASS: account loss invalidates pending opening; an initialized signed-out session cannot start it");
  return 0;
}
