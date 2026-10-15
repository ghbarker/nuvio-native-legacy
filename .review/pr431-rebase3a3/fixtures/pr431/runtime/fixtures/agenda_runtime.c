// Exercise the production layout and transition code, without a GL window.
#include REVIEW_SOURCE
#include <assert.h>

float gfx_escala_ui(void) { return 1.0f; }
float ajustes_rail_largura_fixa(void) { return 0.0f; }
int agenda_n(void) { return 0; }
const AgItem *agenda_lista(int i) { (void)i; return NULL; }

static void tv(void) {
  AgC1 l = c1(); AgMes m = agMesMedir();
  assert(!agColunaUnica());
  assert(l.artW > 0 && l.artH > 0 && l.pnX > l.x0 + l.artW);
  assert(m.painel.x > m.x + m.gradeW && m.painel.y < m.gradeY);
  GfxRect modal=agModalR(1200,900,1);
  assert(modal.w==1200 && modal.h==fminf(900,NV_TELA_H-2*NV_MARGEM_Y)); // Current upstream geometry.
  printf("TV %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
         l.artW,l.artH,l.pnX,l.pnW,l.lsH,m.gradeW,m.celH,m.painel.x,m.painel.y,modal.w,modal.h);
}

int main(void) {
  layout_modo_definir(0); layout_tela_definir(1920,1080); tv();
#ifdef NV_TOUCH_UI
  const int sizes[][2] = {{1080,2340},{1920,1080},{1600,1000},{1080,1080}};
  for (int i=0; i<4; i++) {
    layout_modo_definir(1); layout_tela_definir(sizes[i][0],sizes[i][1]);
    agInterfaceAtualizar();
    AgC1 l = c1(); AgMes m = agMesMedir();
    if (fmaxf(NV_TELA_W,NV_TELA_H)/fminf(NV_TELA_W,NV_TELA_H)>=1.7f) {
      assert(agColunaUnica() && l.artW == 0 && l.artH == 0);
      assert(l.pnX == l.x0 && l.pnW > 0 && l.lsW > 0);
      assert(m.painel.x == m.x && m.painel.y > m.gradeY + m.celH*6);
    } else {
      assert(!agColunaUnica() && l.artW>0 && l.artH>0);
    }
    GfxRect modal=agModalR(1200,900,1);
    if (agColunaUnica()) assert(modal.x>=0 && modal.y>=0 && modal.w<=NV_TELA_W && modal.h<=NV_TELA_H);
    escForcada = 1; assert(!agColunaUnica()); escForcada = 0;
    toqueAgenda.livre = toqueCalendario.livre = toqueNoticia.livre = 1;
    toqueManchetes.livre = toqueModal.livre = 1; toqueAgAtiva = &toqueAgenda;
    toqueCalOffset=toqueManchetesOffset=toqueModalOffset=120;
    scrollY=velY=notRol=notRolAlvo=notRolMax=notVel=120;
    escModal=1; calIniFixo=4;
    layout_modo_definir(0); agInterfaceAtualizar();
    assert(!toqueAgenda.livre && !toqueCalendario.livre && !toqueNoticia.livre);
    assert(!toqueManchetes.livre && !toqueModal.livre && !toqueAgAtiva);
    assert(!toqueCalOffset && !toqueManchetesOffset && !toqueModalOffset);
    assert(!scrollY && !velY && !notRol && !notRolAlvo && !notRolMax && !notVel);
    assert(!escModal && calIniFixo == -1);
    PonteiroRolagem e={0}; assert(!toqueAgendaRolar(&e));
    tv();
  }
#endif
  return 0;
}
