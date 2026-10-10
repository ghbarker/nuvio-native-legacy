/* Independent review of the actual secondary-sheet layout/reset paths.
 * Reuse only the builder fixture's GL/text boundary doubles. */
#define main playback_builder_fixture_main
#include "telefone_dialogos.c"
#undef main

int main(int argc, char **argv) {
  (void)argc; (void)argv;
#if defined(TESTE_ILHA)
  const float screens[][2] = {{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float scales[] = {1,1.2f,1.3f,1.5f};
  for (int s = 0; s < 4; s++) for (int z = 0; z < 4; z++) {
    nv_layout_w=screens[s][0];nv_layout_h=screens[s][1];zoom=escala=scales[z];
    for (int n = 1; n <= 3; n++) for (int saved = 0; saved < 2; saved++) {
      memset(&modalM,0,sizeof modalM);modalM.nBotoes=n;modalM.salvos=saved;modalAviso=modalAberto=1;modalAtividade=0;
      strcpy(modalM.kicker,"Notice");strcpy(modalM.titulo,"A title which takes more than one line on a narrow phone");strcpy(modalM.icone,"check");
      memset(modalM.texto,'W',sizeof modalM.texto-1);strcpy(modalM.estado,"The final text row must remain reachable");
      for (int b=0;b<n;b++) strcpy(modalM.botao[b],"A translated option with long words");
      modalAvisoH=layoutModalAviso((GfxRect){0,0,telefoneui_largura(MD_W,NV_TELA_W,40),MD_H},0,0);
      GfxRect m=modalAlvo((GfxRect){10,0,300,64},1);modalToqueReiniciar();layoutModalAviso(m,1,1);
      assert(nBotoesTeste==2+n+saved);
      float bodyBottom=recorte.y+recorte.h;
      for(int b=2;b<nBotoesTeste;b++) assert(botoesTeste[b].y>=bodyBottom-.01f);
      GfxRect footer[4];for(int b=2;b<nBotoesTeste;b++)footer[b-2]=botoesTeste[b];
      int f=modalFoco,p=pedido,av=avPedido;
      if(toqueModal.maximo>0) arrastar(&toqueModal,modalToqueRolar,1);
      assert(modalFoco==f&&pedido==p&&avPedido==av);layoutModalAviso(m,1,1);
      for(int b=2;b<nBotoesTeste;b++) assert(!memcmp(&footer[b-2],&botoesTeste[b],sizeof(GfxRect)));
    }
    toqueModalY=100;toqueModal.livre=1;abrirCartao(ILHA_ESTREIA);assert(toqueModalY==0&&!toqueModal.livre);
    ilha_modal_fechar(1);relogioQuer=1;atvVisto=4990;strcpy(atvDetalhes,"The current load");
    toqueModalY=100;toqueModal.livre=1;assert(ilha_modal_abrir());assert(modalAtividade&&toqueModalY==0&&!toqueModal.livre);
    ilha_modal_fechar(1);temCur=1;curAte=6000;memset(&cur,0,sizeof cur);cur.cartao=ILHA_VIVO+1;temCartao[ILHA_VIVO]=1;
    toqueModalY=100;toqueModal.livre=1;assert(abrirDoAviso());assert(!modalAviso&&toqueModalY==0&&!toqueModal.livre);
  }
  nv_layout_w=1920;nv_layout_h=1080;zoom=escala=1;modalAtividade=modalAviso=0;
  GfxRect tv=modalAlvo((GfxRect){1800,24,80,64},1);assert(tv.x==760&&tv.y==24&&tv.w==1120&&tv.h==414);
  puts("Independent modal review: bounded stable footer, all opener resets, drag state and TV geometry pass");
#elif defined(TESTE_DIAG)
  if(argc>1&&!strcmp(argv[1],"intro")) {
    const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}},scales[]={1,1.2f,1.3f,1.5f};
    for(int s=0;s<4;s++)for(int z=0;z<4;z++)for(int first=0;first<2;first++) {
      nv_layout_w=screens[s][0];nv_layout_h=screens[s][1];zoom=escala=scales[z];nBotoesTeste=0;cortando=0;
      sairTela=0;introGlobal=first;d.intro=!first;dgApresentacao(first);assert(nBotoesTeste==3);
      assert(dgIntroPhoneCorpo(0,0,dgIntroPhone.corpo.w,0)>400);
      int writes=introGravada,ds=atomic_load(&d.estado),vs=atomic_load(&vz.estado);
      GfxRect footer[2]={botoesTeste[1],botoesTeste[2]};
      if(dgIntroPhone.rolagem.maximo>0)arrastar(&dgIntroPhone.rolagem,dgIntroPhoneRolar,1);
      assert(introGravada==writes&&atomic_load(&d.estado)==ds&&atomic_load(&vz.estado)==vs);
      dgApresentacao(first);assert(!memcmp(&footer[0],&botoesTeste[1],sizeof(GfxRect))&&!memcmp(&footer[1],&botoesTeste[2],sizeof(GfxRect)));
      dgIntroPhoneAcao(1,first);assert(!d.intro&&!introGlobal&&introGravada==writes);assert(sairTela==!first);
    }
    puts("Independent diagnostic intro review: four viewports/scales, body, footer, drag and dismiss gates PASS");
  } else {
    for(int state=0;state<5;state++) {
      d.intro=introGlobal=sairTela=0;vz.aberto=0;atomic_store(&d.estado,state);
      diagPaginaY=200;diagPaginaEstado=state;toqueDiagPagina.livre=1;dgPhoneReiniciar();
      assert(diagPaginaY==0&&diagPaginaEstado==-1&&!toqueDiagPagina.offset&&!toqueDiagPagina.livre);
    }
    puts("Independent diagnostic reset review: PASS; run with intro to check actual preflight dialog");
  }
#endif
  return 0;
}
