/* Real Search row height, scroll-end and published People targets. The
 * existing drawing doubles avoid a window, provider or account request. */
#define TESTE_BUSCA 1
#define main catalogoToqueFixtureMain
#include "catalogo_toque.c"
#undef main

int main(void) {
  const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  int cases=0;
  for(int s=0;s<4;s++)for(int ime=0;ime<2;ime++)for(int wide=0;wide<2;wide++)for(int rail=0;rail<2;rail++) {
    nv_layout_w=screens[s][0];nv_layout_h=screens[s][1];
    imeBuscaTeste=ime;vozBuscaTeste=celBuscaTeste=0;buscaRailTeste=rail?140:0;
    fonteLargaTeste=wide;pilulaBuscaTeste=0;resultadosBuscaTeste=1;recentesBuscaTeste=0;
    memset(fil,0,sizeof fil);memset(pess,0,sizeof pess);memset(animRes,0,sizeof animRes);
    memset(scrollX,0,sizeof scrollX);memset(filEntraEm,0,sizeof filEntraEm);memset(filNova,0,sizeof filNova);
    toqueBuscaLimpar();sugestao=0;nFil=8;nPess=1;nConsulta=3;strcpy(consulta,"abc");
    for(int r=0;r<nFil;r++) {
      fil[r].n=1;fil[r].itens[0]=0;fil[r].titulo="Search results";
      fil[r].origem="A long source name used by the responsive heading";animRes[r][0]=1;
    }
    fil[0].melhor=1;fil[nFil-1].pessoas=1;
    snprintf(pess[0].nome,sizeof pess[0].nome,"A deliberately long performer name with multiple words");
    memset(&itensBuscaTeste[0],0,sizeof itensBuscaTeste[0]);
    strcpy(itensBuscaTeste[0].titulo,"A long movie title before the People row");
    painel=1;focoRes.fileira=nFil-1;focoRes.coluna=0;pedido=-1;
    /* The real maximum offset must expose the complete selected pill,
     * including the 8px focus margin below its avatar. */
    scrollY=scrollAlvo=toqueBuscaMaxY();assert(scrollY>0);
    alvosResultadosTeste=desenhosResultadosTeste=0;desenhaResultados(1000);
    assert(alvosResultadosTeste>0&&!recorteBuscaAtivo&&pedido==-1);
    GfxRect p=alvosBuscaTeste[alvosResultadosTeste-1];
    perto(p.h,BU_PESS_AV+16);
    assert(p.y>=BU_RES_Y-20-.01f&&p.y+p.h<=NV_TELA_H-48+.01f);
    assert(p.x>=BU_RES_X-.01f&&p.x+p.w<=BU_DIR+.01f);
    cases++;
  }
  resultadosBuscaTeste=0;imeBuscaTeste=1;buscaRailTeste=0;
  testMobile = 0;
  const float unchanged[][2]={{1920,1080},{1728,1080}};
  for(int s=0;s<2;s++) {
    nv_layout_w=unchanged[s][0];nv_layout_h=unchanged[s][1];assert(!buTelefone());
    perto(filAlt(nFil-1),filKickH(nFil-1)+BU_PESS_AV);
  }
  printf("Search People: %d real scroll-end/draw target cases PASS; TV/tablet height unchanged\n",cases);
  return 0;
}
