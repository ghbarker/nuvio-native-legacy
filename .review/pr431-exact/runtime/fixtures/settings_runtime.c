// Review-only fixture: actual Settings includes, no window or network.
#include REVIEW_SOURCE
#include <assert.h>

static int scrollHandlers;
float gfx_escala(void) { return 1.0f; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { assert(fn); scrollHandlers++; }

static void tvGeometry(void) {
  assert(!ajTelaCheia() && !ajRetrato());
  assert(NV_LAYOUT_REAL_W == 1920 && NV_LAYOUT_REAL_H == 1080);
  GfxRect menu = aj2MenuR(), list = aj2ListaR(), editor = aj2EditorR();
  printf("TV %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
         menu.x,menu.y,menu.w,menu.h,list.x,list.y,list.w,list.h,
         editor.x,editor.y,editor.w,editor.h);
}

int main(void) {
  assert(valorPadrao[AJ_INTERFACE_MODO] == 2);
  valor[AJ_HOME_LAYOUT] = HOME_LAYOUT_DINAMICA;
  layout_tela_definir(1080,2340); layout_modo_definir(0);
  tvGeometry();
#ifdef NV_TOUCH_UI
  float offset = 777.0f;
  GfxRect region = {10,20,300,400};
  ajToqueN = 1; ajToqueAtivo = AJT_LISTA;
  ajToqueLivre[AJT_LISTA] = 1; ajToqueChave[AJT_LISTA] = 12;
  AjToqueRegiao before[AJT_N]; memcpy(before,ajToqueRegioes,sizeof before);
  assert(ajToqueRegistrar(AJT_LISTA,region,&offset,100,25,99) == 25);
  assert(offset == 777 && ajToqueN == 1 && ajToqueAtivo == AJT_LISTA);
  assert(ajToqueLivre[AJT_LISTA] == 1 && ajToqueChave[AJT_LISTA] == 12);
  assert(!memcmp(before,ajToqueRegioes,sizeof before) && !scrollHandlers);

  layout_modo_definir(1); ajToqueCamada();
  assert(ajTelaCheia() && ajRetrato());
  assert(ajToqueRegistrar(AJT_LISTA,region,&offset,100,25,99) == 25);
  assert(offset == 25 && ajToqueN == 1 && scrollHandlers == 1);
  layout_modo_definir(0); offset = 444; scrollHandlers = 0;
  assert(ajToqueRegistrar(AJT_LISTA,region,&offset,100,40,101) == 40);
  assert(offset == 444 && ajToqueN == 1 && !scrollHandlers);
  tvGeometry();
  fputs("Settings: TV registrar has no side effects; Mobile registers; return-to-TV ignores retained state PASS\n",stderr);
#endif
  return 0;
}
