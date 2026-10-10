/* Production post-panel layer route: inline keeps only the panel's own
 * targets; genuine context pages keep their modal barrier. */
#define main app_sidebar_main
#include "app_sidebar_toque.c"
#undef main
static int inlineActive;
int ctx_inline_painel_ativo(void) { return contexto && inlineActive; }
int main(void) {
  const float sizes[][2]={{1080,1920},{1080,2340},{2160,1080},{2520,1080}};
  for(int i=0;i<4;i++) {
    nv_layout_w=sizes[i][0];nv_layout_h=sizes[i][1];contexto=inlineActive=1;
    ponteiro_camada();ponteiro_alvo(40,200,180,50,NULL,NULL,2,0);
    desenharCtxDoPainel(5000);assert(nAlvos==1&&ultimoAlvo.a==2);
    inlineActive=0;desenharCtxDoPainel(5000);
    assert(nAlvos==1&&!ultimoAlvo.focar&&!ultimoAlvo.ativar&&ultimoAlvo.a==0&&ultimoAlvo.w==nv_layout_w&&ultimoAlvo.h==nv_layout_h);
  }
  testMobile=0;nv_layout_w=1920;nv_layout_h=1080;contexto=inlineActive=1;
  ponteiro_camada();ponteiro_alvo(40,200,180,50,NULL,NULL,2,0);desenharCtxDoPainel(5000);
  assert(nAlvos==1&&ultimoAlvo.a==0&&ultimoAlvo.w==1920);
  contexto=0;desenharCtxDoPainel(5000);assert(nAlvos==1);
  puts("app_saved_inline: actual post-panel route retains phone inline controls, standalone/TV barriers preserved PASS");
}
