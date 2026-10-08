// Preferencias de perfil com armazenamento em memoria.
#include "../src/ajustes.c"
#include <assert.h>

static char *arquivos[33];
static int numero(const char *n){int p=0;sscanf(n,"ajustes-p%d.txt",&p);return p>0&&p<=32?p:0;}
char *dados_ler(const char *n){int p=numero(n);return p&&arquivos[p]?strdup(arquivos[p]):NULL;}
int dados_gravar(const char *n,const char *v){int p=numero(n);if(p){free(arquivos[p]);arquivos[p]=strdup(v);}return 1;}
int dados_apagar(const char *n){int p=numero(n);if(p){free(arquivos[p]);arquivos[p]=NULL;}return 1;}
void dados_marcar_sujo(int leve){(void)leve;}
void fonteregra_perfil_guardar(int p){(void)p;}
int fonteregra_perfil_restaurar(int p){(void)p;return 0;}
void fonteregra_perfil_esquecer(void){}
int fonteregra_do_blob(const char *j){(void)j;return 0;}
int fonteregra_mesclar(const char *j,char **s){(void)j;(void)s;return 0;}
void selospacote_conta_do_blob(const char *j){(void)j;}
int selospacote_n(void){return 0;}
static PosterProvCfg prov;
const PosterProvCfg *posterprov_cfg(void){return &prov;}
void posterprov_configurar(const PosterProvCfg *p){prov=*p;}

static void padrao(void){
  assert(!ajustes_auto_abertura()&&!ajustes_auto_resumo()&&!ajustes_auto_creditos()&&ajustes_auto_proximo());
}
int main(void){
  padrao();
  for(int i=AJ_AUTO_ABERTURA;i<=AJ_AUTO_PROXIMO;i++){
    assert(dePerfil(i)&&somenteDesteAparelho(i)&&OPCOES[i].n==2);
    assert(!strcmp(uxEscopo(i),"Só nesta TV"));
  }
  valor[AJ_AUTO_ABERTURA]=0;valor[AJ_AUTO_CREDITOS]=0;valor[AJ_AUTO_PROXIMO]=1;
  valor[AJ_HERO]=1;ajustes_perfil_guardar(1);
  assert(strstr(arquivos[1],"autoAberturaLocal 0"));
  valor[AJ_AUTO_ABERTURA]=1;valor[AJ_AUTO_RESUMO]=0;valor[AJ_AUTO_CREDITOS]=1;valor[AJ_AUTO_PROXIMO]=0;
  ajustes_perfil_guardar(2);
  assert(ajustes_perfil_restaurar(1));
  assert(ajustes_auto_abertura()&&!ajustes_auto_resumo()&&ajustes_auto_creditos()&&!ajustes_auto_proximo());
  assert(ajustes_perfil_restaurar(2));
  assert(!ajustes_auto_abertura()&&ajustes_auto_resumo()&&!ajustes_auto_creditos()&&ajustes_auto_proximo());
  const char *blob="{\"autoAberturaLocal\":0,\"autoResumoLocal\":1,\"autoCreditosLocal\":0,\"autoProximoLocal\":1}";
  ajustes_aplicar_blob(blob);
  assert(!ajustes_auto_abertura()&&ajustes_auto_resumo()&&!ajustes_auto_creditos()&&ajustes_auto_proximo());
  char *out=NULL;assert(!ajustes_mesclar_blob(blob,&out)&&!out);
  dados_gravar("ajustes-p3.txt","heroSectionEnabled 1\n");
  assert(ajustes_perfil_restaurar(3));padrao();
  assert(ajustes_perfil_restaurar(1));
  ajustes_auto_restaurar(4);padrao();assert(valor[AJ_HERO]==1); // fallback principal, somente novas chaves resetadas
  ajustes_auto_restaurar(1);assert(ajustes_auto_abertura()&&ajustes_auto_creditos()&&!ajustes_auto_proximo());
  ajustes_perfil_esquecer();padrao();
  assert(!arquivos[1]&&!arquivos[2]&&!arquivos[3]);
  puts("ajustes_auto: defaults, independent controls, A/B/A, old/new profile, cloud exclusion, logout passed");
  return 0;
}
