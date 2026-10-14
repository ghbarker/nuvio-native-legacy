#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((int)((B)-(A)) <= 0)
#endif
#include "ajustes.c"
#include <assert.h>
static char testDir[500];
static int testProfile = 1, modeSaveFails;
static const char *testUser = "account-a";
const char *sessao_usuario(void) { return testUser; }
int perfis_ativo(void) { return testProfile; }
char *dados_caminho(char *dst, unsigned n, const char *nome) {
  snprintf(dst,n,"%s/%s",testDir,nome); return dst;
}
char *dados_ler(const char *nome) {
  char p[640]; dados_caminho(p,sizeof p,nome); FILE *f=fopen(p,"rb");
  if(!f) return NULL;
  fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
  char *r=calloc((size_t)n+1,1); assert(r);
  assert(fread(r,1,(size_t)n,f)==(size_t)n); fclose(f); return r;
}
int dados_gravar(const char *nome,const char *text) {
  if (modeSaveFails && !strncmp(nome,"provider-scope-",15)) return 0;
  char p[640]; dados_caminho(p,sizeof p,nome); FILE *f=fopen(p,"w");
  if(!f) return 0;
  int ok=fputs(text,f)>=0; return fclose(f)==0&&ok;
}
void dados_fs_travar(void) {} void dados_fs_liberar(void) {}
void dados_marcar_sujo(int leve) { (void)leve; }
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 1; }
const char *ling_opcao_codigo(int i) { (void)i;return ""; }
void ling_conta_legenda(const char *s) { (void)s; }
void ling_conta_legenda2(const char *s) { (void)s; }
void ling_conta_audio(const char *s) { (void)s; }
const char *ling_conta_legenda_valor(void) { return ""; }
const char *ling_conta_legenda2_valor(void) { return ""; }
const char *ling_conta_audio_valor(void) { return ""; }
void ling_local_legenda(const char *s) { (void)s; }
void ling_local_legenda2(const char *s) { (void)s; }
void ling_local_audio(const char *s) { (void)s; }
static void keyIs(const char *s) {
  DebridContexto c; debrid_contexto_capturar(&c); assert(!strcmp(c.chave[DEBRID_TB],s));
}
static void openProfile(int p, uint64_t visit) {
  testProfile=p; debrid_abrir_sessao(visit); ajustes_provedores_abrir_sessao();
}
static void bankIs(int tb, int addon, const char *addonKey) {
  assert(fontefalha_provedores());
  assert(fontefalha_addons());
  assert(fontefalha_provedor_limite(DEBRID_TB)==tb);
  assert(fontefalha_addon_limite(addonKey)==addon);
  assert(valor[AJ_FALHA_TB_LIMITE]==tb);
  assert(valor[AJ_FALHA_PROVEDORES]==0);
  assert(valor[AJ_FALHA_ADDONS]==0);
}
int main(void) {
  char d[]="/tmp/nuvio-provider-pr-XXXXXX",name[128],path[640],addon[FONTEFALHA_CHAVE];
  assert(mkdtemp(d)); strcpy(testDir,d); fontefalha_dir(testDir);
  assert(dados_gravar("debrid.txt","torbox=SHARED-INITIAL\n"));
  assert(fontefalha_definir_provedores(1)); assert(fontefalha_definir_addons(1));
  assert(fontefalha_provedor_mover(DEBRID_TB,-1));
  assert(fontefalha_provedor_definir_limite(DEBRID_TB,7));
  fontefalha_addon_chave("https://example.invalid/addon-a",addon,sizeof addon);
  assert(fontefalha_addon_definir_limite(addon,4));

  openProfile(1,1); assert(ajustes_provedores_compartilhados());
  keyIs("SHARED-INITIAL"); bankIs(7,4,addon);
  assert(ajustes_definir_provedores_compartilhados(0));
  assert(!ajustes_provedores_compartilhados());
  keyIs("SHARED-INITIAL"); bankIs(7,4,addon);
  assert(ajustes_provedores_arquivo(name,sizeof name,"debrid"));
  assert(!strcmp(name,"debrid-account-a-p1.txt"));
  dados_caminho(path,sizeof path,name); struct stat st;
  assert(stat(path,&st)==0&&(st.st_mode&0777)==0600);
  assert(ajustes_provedores_arquivo(name,sizeof name,"fontefalha"));
  assert(!strcmp(name,"fontefalha-account-a-p1.txt"));
  dados_caminho(path,sizeof path,name);
  assert(stat(path,&st)==0&&(st.st_mode&0777)==0600);
  debDefinir(AJ_DEBRID_TB,"PRIVATE-P1"); keyIs("PRIVATE-P1");

  // A different profile updates the shared bank. A's private order and caps survive.
  openProfile(2,2); assert(ajustes_provedores_compartilhados()); keyIs("SHARED-INITIAL");
  debDefinir(AJ_DEBRID_TB,"SHARED-UPDATED"); keyIs("SHARED-UPDATED");
  assert(fontefalha_provedor_definir_limite(DEBRID_TB,2));
  assert(fontefalha_provedor_mover(DEBRID_PM,-1));
  assert(fontefalha_provedor_mover(DEBRID_PM,-1));
  assert(fontefalha_addon_definir_limite(addon,6));
  openProfile(1,3); assert(!ajustes_provedores_compartilhados());
  keyIs("PRIVATE-P1"); bankIs(7,4,addon);
  assert(fontefalha_provedor_posicao(DEBRID_TB)==0);

  assert(ajustes_definir_provedores_compartilhados(1));
  keyIs("SHARED-UPDATED"); bankIs(2,6,addon);
  assert(fontefalha_provedor_posicao(DEBRID_PM)==0);
  assert(ajustes_definir_provedores_compartilhados(0));
  keyIs("PRIVATE-P1"); bankIs(7,4,addon);
  assert(fontefalha_provedor_posicao(DEBRID_TB)==0);
  // Reopening the same profile reloads its on-disk bank and mirrored values.
  openProfile(1,4); keyIs("PRIVATE-P1"); bankIs(7,4,addon);

  // A different account with the same profile number gets a distinct private copy.
  testUser="account-b"; openProfile(1,5);
  assert(ajustes_provedores_compartilhados()); keyIs("SHARED-UPDATED");
  assert(ajustes_definir_provedores_compartilhados(0)); keyIs("SHARED-UPDATED");
  bankIs(2,6,addon); assert(fontefalha_provedor_definir_limite(DEBRID_TB,9));
  debDefinir(AJ_DEBRID_TB,"PRIVATE-ACCOUNT-B"); keyIs("PRIVATE-ACCOUNT-B");
  testUser="account-a"; openProfile(1,6); keyIs("PRIVATE-P1"); bankIs(7,4,addon);

  // Empty local credentials are deliberate, never recloned when switching modes.
  debDefinir(AJ_DEBRID_TB,""); keyIs("");
  assert(ajustes_definir_provedores_compartilhados(1));
  assert(ajustes_definir_provedores_compartilhados(0)); keyIs(""); bankIs(7,4,addon);
  openProfile(3,7); debrid_definir_chave("premiumize","CLOUD-P3");
  assert(ajustes_definir_provedores_compartilhados(0));
  DebridContexto c; debrid_contexto_capturar(&c);
  assert(!strcmp(c.chave[DEBRID_PM],"CLOUD-P3")); assert(debrid_origem("premiumize")==2);

  // Device settings and the scope mode survive restoring personal defaults.
  valor[AJ_HERO]=1; valor[AJ_RESOLUCAO]=RES_4K; valor[AJ_ADDONS_PRINCIPAL]=0;
  ajustes_perfil_padrao(); assert(valor[AJ_HERO]==valorPadrao[AJ_HERO]);
  assert(valor[AJ_RESOLUCAO]==RES_4K&&valor[AJ_ADDONS_PRINCIPAL]==0);
  assert(!ajustes_provedores_compartilhados());

  // Failure to save mode never publishes the staged private accounts or bank.
  openProfile(4,8); modeSaveFails=1;
  assert(!ajustes_definir_provedores_compartilhados(0));
  assert(ajustes_provedores_compartilhados()); keyIs("SHARED-UPDATED"); bankIs(2,6,addon);
  modeSaveFails=0; assert(ajustes_definir_provedores_compartilhados(0)); bankIs(2,6,addon);

  // A private-bank filesystem error also leaves the shared setup active.
  openProfile(5,9); assert(debNomePerfil(name,sizeof name,"fontefalha"));
  dados_caminho(path,sizeof path,name); assert(mkdir(path,0700)==0);
  assert(!ajustes_definir_provedores_compartilhados(0));
  assert(ajustes_provedores_compartilhados()); keyIs("SHARED-UPDATED"); bankIs(2,6,addon);

  // A failed first credential copy does not lose the shared setup.
  openProfile(6,10); strcat(testDir,"/missing");
  assert(!ajustes_definir_provedores_compartilhados(0));
  assert(ajustes_provedores_compartilhados()); keyIs("SHARED-UPDATED");
  puts("provider scope: upstream native/addon order and caps, A-B-A, same-profile reopening, shared/private reload, accounts, empty credentials, cloud clone, 0600, defaults, save failures PASS");
  return 0;
}
