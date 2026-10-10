/* Production failed-source card and touch actions preserve Android #409:
 * either decision clears the failed source and reopens source selection. */
#define main miniature_fixture_main
#include "player_mini_toque.c"
#undef main
static int sheet,tracks,episodes;
int stream_folha_aberta(void) { return sheet; }
int faixas_aberta(void) { return tracks; }
int episodios_aberto(void) { return episodes; }
int plrui_tinta(void) { return 0; }
float txt_bloco_corta(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float lead,float a,int max) {
  assert(w>0);TxtLinha t=txt_linha(e,s,r,g,b,255);int n=(int)ceilf(t.w/w);
  if(max>0&&n>max)n=max;t.w=(int)fminf(t.w,w);t.h=(int)(n*lead);
  if(a>0)txt_desenhar_alpha(t,x,y,a);return n*lead;
}
void plrui_pilula_foco(GfxRect r,float a) { plrui_botao_repouso(r,a); }
float plrui_botao(float x,float y,const char *rot,const char *ico,float f,float a) {
  (void)ico;(void)f;(void)a;float w=txt_linha(TXT_G21B,rot,0,0,0,255).w+80;bounds((GfxRect){x,y,w,60});return w;
}
static void resetError(void) {
  aberto=erroFonte=1;canalSessao=mini=saindo=pedFontes=pediuSair=0;
  esperandoFonte=tocando=0;
  sheet=tracks=episodes=0;erroBotao=0;nTargets=layers=0;
  permitido=1;player_toque_modal_guarda(pode);
  strcpy(erroTitulo,"The selected source failed to open after the provider returned an error");
  strcpy(erroDica,"Choose a different source or return to the title page to try again");
}
#ifndef NV_PLAYER_ERROR_DOUBLES_ONLY
int main(void) {
  const float sizes[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float scales[]={1,1.2f,1.3f,1.5f};
  for(int t=0;t<4;t++)for(int s=0;s<4;s++) {
    nv_layout_w=sizes[t][0];nv_layout_h=sizes[t][1];testUI=testScale=scales[s];
    resetError();float width=erroLargura(),height=alturaErro();
    GfxRect body={96,116,width,height};bounds(body);corpoErro(body,1,NULL);
    assert(layers==1&&nTargets==3&&!targets[0].focar&&!targets[0].ativar);
    assert(targets[1].a==0&&targets[2].a==1&&targets[1].ativar==erroToqueAcao&&targets[2].ativar==erroToqueAcao);
    assert(targets[1].x+targets[1].w<targets[2].x&&targets[2].y+targets[2].h<=body.y+body.h);
    PonteiroAlvo open=targets[1],back=targets[2];int stop=stops,orient=orientationCalls;
    open.ativar(open.a,0);assert(pedFontes&&!erroFonte&&esperandoFonte&&!saindo&&!pediuSair&&stops==stop&&orientationCalls==orient);
    resetError();back.ativar(back.a,0);assert(pedFontes&&!erroFonte&&esperandoFonte&&!saindo&&!pediuSair&&stops==stop&&orientationCalls==orient);
    for(int block=0;block<9;block++) {
      resetError();
      if(block==0)mini=1;if(block==1)aberto=0;if(block==2)saindo=1;if(block==3)erroFonte=0;
      if(block==4)canalSessao=1;if(block==5)sheet=1;if(block==6)tracks=1;if(block==7)episodes=1;
      if(block==8)permitido=0;
      int out=saindo;open.ativar(0,0);back.ativar(1,0);assert(!pedFontes&&saindo==out&&!pediuSair);
      corpoErro(body,1,NULL);assert(!nTargets&&!layers);
    }
  }
  testMobile=0;
  testUI=testScale=1;nv_layout_w=1920;nv_layout_h=1080;resetError();
  corpoErro((GfxRect){96,116,880,alturaErro()},1,NULL);assert(!nTargets&&!layers&&erroLargura()==880);
  testMobile=1;
  puts("player_error_toque: bounded production card, Android OpenSources/Back recovery and stale/overlay/TV guards PASS");
}
#endif
