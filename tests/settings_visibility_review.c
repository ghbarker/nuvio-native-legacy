/* Registered Settings slots and live visibility use different predicates. */
#define TESTE_AJUSTES 1
#define AJUSTES_TESTE 1
#define SDL_MAIN_HANDLED 1
#define main settings_gesture_fixture_main
#include "menus_toque.c"
#undef main

static int temCinemeta, apoiador, recomendaTeste;
int addons_n(void) { return temCinemeta ? 1 : 0; }
const char *addons_base(int i) { assert(i==0&&temCinemeta);return "https://v3-cinemeta.strem.io"; }
int apoiador_ativo(void) {return apoiador;}
int recomenda_ativo(void) {return recomendaTeste;}
static int indice(int op) {for(int i=0;i<AJ_N_TELA;i++)if(TELA[i].tipo==IT_OPC&&TELA[i].op==op)return i;assert(0);return -1;}

int main(void) {
  montarTela();valor[AJ_AVANCADAS]=0;recomendaTeste=0;apoiador=0;
  int op,sec,n=0,cineCatalog=-1;const char *key,*rot;
  for(int i=0;i<AJ_N_TELA;i++)if(TELA[i].tipo==IT_OPC&&TELA[i].op>=0&&TELA[i].op<AJ_N) {
    if(TELA[i].op==AJ_BUSCA_CINEMETA)cineCatalog=n;
    n++;
  }
  int cine=indice(AJ_BUSCA_CINEMETA),icon=indice(AJ_ICONE_APP);
  assert(n>200&&cineCatalog>=0);
  for(int addon=0;addon<=1;addon++)for(int valorCine=0;valorCine<=1;valorCine++) {
    temCinemeta=addon;valor[AJ_BUSCA_CINEMETA]=valorCine;
    assert(visivel(cine)==(addon||valorCine));
    assert(ajustes_busca_cinemeta()==!valorCine);
    /* Inventory still enumerates the slot when the normal menu hides it. */
    assert(ajustes_teste_cena_item(cineCatalog,&op,&sec,&key,&rot));
    assert(op==AJ_BUSCA_CINEMETA&&!strcmp(key,"buscaCinemetaLocal")&&!strcmp(rot,"Buscar no Cinemeta"));
  }
  assert(!visivel(icon));apoiador=1;assert(visivel(icon));
  assert(!visivel(indice(AJ_PERFIL_PESQ))&&!visivel(indice(AJ_PERFIL_EDITAR)));
  recomendaTeste=1;assert(visivel(indice(AJ_PERFIL_PESQ))&&visivel(indice(AJ_PERFIL_EDITAR)));
  printf("settings_visibility_review: %d registered slots; Cinemeta/addon/value, supporter and social visibility PASS\n",n);
}
