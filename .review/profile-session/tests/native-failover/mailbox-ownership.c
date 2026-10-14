// Actual app worker/mailbox + policy and real perfis.c gate. The canonical
// playback fixture supplies its player/transport doubles unchanged.
#include "fontefalha.h"
static void tracked_release(FonteFalhaSessao *);
#define fontefalha_sessao_liberar tracked_release
#define main original_playback_fixture_main
#include "tests/fontefalha_playback.c"
#undef main
#undef fontefalha_sessao_liberar
#include "perfis.h"

static FonteFalhaSessao *expectedClone;
static int cloneReleases, appliedScoped, enteredEpoch;
static DebridContexto *activeCredentials;
static void tracked_release(FonteFalhaSessao *s) {
  if(s && s==expectedClone) { cloneReleases++; expectedClone=NULL; }
  fontefalha_sessao_liberar(s);
}
const char *sessao_usuario(void) { return "native-mailbox-fixture"; }
int dados_gravar(const char *name, const char *body) { (void)name; (void)body; return 1; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
// The real native credential transport is exercised by request-ownership.c;
// these playback doubles assert the actual app captures/enters its job copy.
void debrid_contexto_capturar(DebridContexto *c) {
  memset(c,0,sizeof *c); c->sessao=perfis_sessao_geracao();
  snprintf(c->chave[1],sizeof c->chave[1],"fixture-profile-%d",perfis_ativo());
}
void debrid_contexto_entrar(const DebridContexto *c) {
  assert(c==&fonteJob.debrid && !activeCredentials);
  activeCredentials=(DebridContexto *)c; enteredEpoch=c->sessao;
}
void debrid_contexto_sair(void) { assert(activeCredentials); activeCredentials=NULL; }
int stream_primeira_boa_sessao(int n, uint64_t epoch) {
  assert(perfis_sessao_atual(epoch)); return stream_primeira_boa(n);
}
int stream_verificar_proxima_vod_sessao(int idx,int n,int *checked,uint64_t epoch) {
  assert(perfis_sessao_atual(epoch)); return stream_verificar_proxima_vod(idx,n,checked);
}
int stream_canal_primeira_viva_sessao(int n,uint64_t epoch) {
  assert(perfis_sessao_atual(epoch)); return stream_canal_primeira_viva(n);
}
int stalker_resolver_sessao(const char *id,char *url,unsigned n,uint64_t epoch) {
  assert(perfis_sessao_atual(epoch)); return stalker_resolver(id,url,n);
}
int stream_falha_aplicar_sessao(int idx,unsigned gen,int service,const char *url,uint64_t epoch) {
  assert(perfis_sessao_atual(epoch)); appliedScoped++;
  return stream_falha_aplicar(idx,gen,service,url);
}
int main(int argc,char **argv) {
  assert(argc==2); fontefalha_dir(argv[1]); perfis_definir_ativo(1);
  resetar(); aguardandoFonte=2;
  assert(!pedirFonteJob(FJOB_FALHA,fontePedidoGeracao,NULL,0));
  uint64_t original=fonteJob.sessaoPerfil;
  assert(fioFonteVivo && fonteJob.falha && fonteJob.falha!=fonteFalha);
  assert(fonteJob.debrid.sessao==original && strstr(fonteJob.debrid.chave[1],"profile-1"));
  expectedClone=fonteJob.falha;
  perfis_definir_ativo(2); perfis_definir_ativo(1);
  assert(!perfis_sessao_atual(original));
  concluirWorker();
  assert(enteredEpoch==original && !activeCredentials);
  assert(atomic_load(&fonteJob.estado)==FJOB_DONE && fonteJob.falha);
  processarFonteJob();
  assert(cloneReleases==1 && !expectedClone && !fonteJob.falha && !fioFonteVivo);
  assert(atomic_load(&fonteJob.estado)==FJOB_IDLE);
  assert(!aplicacoes && !appliedScoped && fonteEscolhida==-2);
  assert(fontefalha_tentativas(fonteFalha)==1);
  puts("PASS native mailbox: stale A→B→A DONE is joined, its owned clone freed exactly once, and no active policy/source is updated");

  assert(!pedirFonteJob(FJOB_FALHA,fontePedidoGeracao,NULL,0));
  uint64_t current=perfis_sessao_geracao(); assert(fonteJob.sessaoPerfil==current);
  concluirWorker(); processarFonteJob();
  assert(!fioFonteVivo && !fonteJob.falha && atomic_load(&fonteJob.estado)==FJOB_IDLE);
  assert(aplicacoes==1 && appliedScoped==1 && fonteEscolhida==1);
  assert(fontefalha_tentativas(fonteFalha)==2 && enteredEpoch==current);
  puts("PASS native mailbox: releasing the stale clone makes the same worker slot usable by a current positive-control job");
  limparFonteVOD(); return 0;
}
