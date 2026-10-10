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
    assert(ajustes_teste_opcao_visivel(TELA[i].op)==visivel(i));
    if(TELA[i].op==AJ_BUSCA_CINEMETA)cineCatalog=n;
    n++;
  }
  int cine=indice(AJ_BUSCA_CINEMETA),icon=indice(AJ_ICONE_APP);
  assert(n>200&&cineCatalog>=0);
  for(int addon=0;addon<=1;addon++)for(int valorCine=0;valorCine<=1;valorCine++) {
    temCinemeta=addon;valor[AJ_BUSCA_CINEMETA]=valorCine;
    assert(visivel(cine)==(addon||valorCine));
    assert(ajustes_teste_opcao_visivel(AJ_BUSCA_CINEMETA)==(addon||valorCine));
    assert(ajustes_busca_cinemeta()==!valorCine);
    /* Inventory still enumerates the slot when the normal menu hides it. */
    assert(ajustes_teste_cena_item(cineCatalog,&op,&sec,&key,&rot));
    assert(op==AJ_BUSCA_CINEMETA&&!strcmp(key,"buscaCinemetaLocal")&&!strcmp(rot,"Buscar no Cinemeta"));
  }
  assert(!visivel(icon)&&!ajustes_teste_opcao_visivel(AJ_ICONE_APP));
  apoiador=1;assert(visivel(icon)&&ajustes_teste_opcao_visivel(AJ_ICONE_APP));
  assert(!visivel(indice(AJ_PERFIL_PESQ))&&!visivel(indice(AJ_PERFIL_EDITAR)));
  recomendaTeste=1;assert(visivel(indice(AJ_PERFIL_PESQ))&&visivel(indice(AJ_PERFIL_EDITAR)));
  assert(ajustes_teste_opcao_visivel(AJ_PERFIL_PESQ)&&ajustes_teste_opcao_visivel(AJ_PERFIL_EDITAR));
  assert(!ajustes_teste_opcao_visivel(-1)&&!ajustes_teste_opcao_visivel(AJ_N));
  const float telas[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080},{1920,1080},{1600,1000},{1000,1600}};
  int layout=indice(AJ_LAYOUT_AJUSTES), salvo=valor[AJ_LAYOUT_AJUSTES];
  for(int t=0;t<7;t++)for(int v=0;v<2;v++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];valor[AJ_LAYOUT_AJUSTES]=v;
    assert(ajustes_layout_lista()==(t<4||v));
    assert(visivel(layout)==(t>=4)&&ajustes_teste_opcao_visivel(AJ_LAYOUT_AJUSTES)==(t>=4));
    assert(focavel(layout)==(t>=4)&&valor[AJ_LAYOUT_AJUSTES]==v);
  }
  valor[AJ_LAYOUT_AJUSTES]=salvo;
  nv_layout_w=2400;nv_layout_h=1080;
  printf("settings_visibility_review: %d registered slots mapped to actual rows; Cinemeta/addon/value, supporter and social visibility PASS\n",n);
}
