// Saved preferences retain their original IDs; the preference is device-local.
#include "../src/ajustes.c"
#include <assert.h>

static void savedRecord(const char *dir,const char *record,int preference) {
  char path[700];snprintf(path,sizeof path,"%s/ajustes.txt",dir);
  FILE *f=fopen(path,"w");assert(f);
  fputs(record,f);fclose(f);
  ajustes_dir(dir);
  assert(valor[AJ_INTERFACE_MODO]==preference);
  assert(ajustes_interface_mobile()==(preference==1)); // Desktop Auto is TV.
  assert(layout_modo_mobile()==(preference==1));
}
static void savedChoice(const char *dir,int preference) {
  char record[60];snprintf(record,sizeof record,"interfaceModoLocal %d\n",preference);
  savedRecord(dir,record,preference);
}

int main(int argc,char **argv) {
  assert(argc==2);const char *dir=argv[1];
  assert(SDL_Init(SDL_INIT_TIMER)==0);
  assert(!setenv("NUVIO_DADOS",dir,1));dados_iniciar(dir);ajustes_dir(dir);
  assert(AJ_INTERFACE_MODO==AJ_LEG_SYNC_AUTO+1);
  assert(valorPadrao[AJ_INTERFACE_MODO]==2 && valor[AJ_INTERFACE_MODO]==2);
  assert(!ajustes_interface_mobile() && !layout_modo_mobile());
  assert(!strcmp(CHAVE[AJ_INTERFACE_MODO],"interfaceModoLocal"));
  assert(somenteDesteAparelho(AJ_INTERFACE_MODO) && !dePerfil(AJ_INTERFACE_MODO));
  assert(OPCOES[AJ_INTERFACE_MODO].n==3);
  assert(!strcmp(V_INTERFACE_MODO[0],"TV") && !strcmp(V_INTERFACE_MODO[1],"Mobile"));
  assert(!strcmp(V_INTERFACE_MODO[2],"Automático"));
  montarTela();assert(uxAvancada(AJ_INTERFACE_MODO));

  // Older previews already saved TV=0 and Mobile=1. Neither becomes Auto.
  savedChoice(dir,0);savedChoice(dir,1);savedChoice(dir,2);
  savedRecord(dir,"interfaceModoLocal 0\ninterfaceModoLocal 99\n",0);
  savedRecord(dir,"interfaceModoLocal 1\ninterfaceModoLocal 99\n",1);
  savedRecord(dir,"interfaceModoLocal 0\ninterfaceModoLocal 1\n",1);
  savedRecord(dir,"interfaceModoLocal 1junk\n",1);
  savedRecord(dir,"interfaceModoLocal 999999999999999999999999999999999999\n",2);
  savedRecord(dir,"interfaceModoLocal invalid\ninterfaceModoLocal -1\n",2);
  savedRecord(dir,"interfaceModoLocal -999999999999999999999999999999999999\n",2);
  savedRecord(dir,"interfaceModoLocal +2suffix\n",2);
  savedRecord(dir,"idioma 1\n",2);
  char oversized[200],records[300];
  snprintf(oversized,sizeof oversized,"interfaceModoLocal %0100d1\n",0);
  savedRecord(dir,oversized,2);
  snprintf(records,sizeof records,"interfaceModoLocal 0\n%s",oversized);
  savedRecord(dir,records,0);
  snprintf(records,sizeof records,"%sinterfaceModoLocal 1\n",oversized);
  savedRecord(dir,records,1);
  const int sizes[][2]={{1080,2340},{1920,1080},{1600,1000},{1080,1080}};
  for(int i=0;i<4;i++) {
    layout_tela_definir(sizes[i][0],sizes[i][1]);
    int tvLayout=i%2;valor[AJ_LAYOUT_AJUSTES]=tvLayout;
    assert(definirValorDireto(AJ_INTERFACE_MODO,1));
    assert(ajustes_interface_mobile() && layout_modo_mobile());
    assert(ajTelaCheia() && !ajLayoutSelecionavel() && ajustes_layout_lista());
    assert(valor[AJ_LAYOUT_AJUSTES]==tvLayout);
    focarOpcao(AJ_INTERFACE_MODO);
    assert(focoOp==AJ_INTERFACE_MODO && visivel(focoItem));
    float shortEdge=(float)(sizes[i][0]<sizes[i][1]?sizes[i][0]:sizes[i][1]);
    assert(NV_TELA_W==1080.0f*sizes[i][0]/shortEdge);
    assert(NV_TELA_H==1080.0f*sizes[i][1]/shortEdge);
    valor[AJ_INTERFACE_MODO]=0;layout_modo_definir(0);ajustes_dir(dir);
    assert(valor[AJ_INTERFACE_MODO]==1 && layout_modo_mobile());
    assert(definirValorDireto(AJ_INTERFACE_MODO,0));
    assert(!layout_modo_mobile() && NV_TELA_W==1920 && NV_TELA_H==1080);
    assert(!ajTelaCheia() && ajLayoutSelecionavel());
    assert(ajustes_layout_lista()==tvLayout && valor[AJ_LAYOUT_AJUSTES]==tvLayout);
    focarOpcao(AJ_INTERFACE_MODO);
    assert(focoOp==AJ_INTERFACE_MODO && visivel(focoItem));
  }
  for(int preference=0;preference<3;preference++) {
    assert(definirValorDireto(AJ_INTERFACE_MODO,preference));
    char *disk=dados_ler("ajustes.txt");assert(disk);
    char line[40];snprintf(line,sizeof line,"interfaceModoLocal %d\n",preference);
    assert(strstr(disk,line));free(disk);
    for(int foreign=0;foreign<3;foreign++) {
      char blob[80];snprintf(blob,sizeof blob,"{\"interfaceModoLocal\":%d}",foreign);
      ajustes_aplicar_blob(blob);assert(valor[AJ_INTERFACE_MODO]==preference);
      char *merged=NULL;assert(ajustes_mesclar_blob(blob,&merged)==0);
      assert(!merged || !strcmp(merged,blob));free(merged);
    }
    ajustes_perfil_guardar(1);
    char *profile=dados_ler("ajustes-p1.txt");assert(profile);
    assert(!strstr(profile,"interfaceModoLocal"));free(profile);
    assert(ajustes_perfil_restaurar(1));assert(valor[AJ_INTERFACE_MODO]==preference);
    assert(layout_modo_mobile()==(preference==1));
  }
  puts("interface_mode: Auto default, legacy IDs, Advanced override, live resize, persistence and account/profile isolation PASS");
  SDL_Quit();
  return 0;
}
