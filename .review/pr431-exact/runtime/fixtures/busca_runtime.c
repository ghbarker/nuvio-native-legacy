// Capture production heading styles and hit targets at the renderer boundary.
#include REVIEW_SOURCE
#include <assert.h>

static int kicks, lines, clipped, ordinary, keyboardClosed;
static GfxRect target;
static TxtEstilo firstStyle;
float ajustes_rail_largura_fixa(void) { return 0; }
int st_ime_disponivel(void) { return 1; }
void st_fechar(int owner) { assert(owner==ST_BUSCA);keyboardClosed++; }
int menu_pilula_rect(float *x,float *y,float *w,float *h) {
  (void)x;(void)y;(void)w;(void)h;return 0;
}
const char *i18n(const char *s) { return s; }
float ajustes_ui_kicker(const char *s,float x,float y,float a) {
  (void)s;(void)x;(void)y;(void)a;kicks++;return 100;
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float max) {
  (void)s;(void)r;(void)g;(void)b;(void)a;(void)max;
  if (!lines++) firstStyle=e;return (TxtLinha){0,100,20,0,0};
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) { (void)l;(void)x;(void)y;(void)a; }
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int i,int j) {
  (void)f;(void)a;(void)i;(void)j;clipped++;target=(GfxRect){x,y,w,h};
}
void ponteiro_alvo_faixa(float x,float y,float w,float h,float top,float bottom,PonteiroFn f,PonteiroFn a,int i,int j) {
  (void)top;(void)bottom;(void)f;(void)a;(void)i;(void)j;
  ordinary++;target=(GfxRect){x,y,w,h};
}

static void tv(void) {
  assert(!buTelefone() && !buUmaColuna() && BU_COL_W==520 && BU_CAMPO_H==76);
  kicks=lines=clipped=ordinary=0;
  fil[0].titulo="Catalog";fil[0].origem="Addon";
  buCabecalho(0,BU_RES_Y,BU_DIR-BU_RES_X);
  assert(kicks==1 && lines==1 && firstStyle==TXT_ILHA_APOIO);
  GfxRect r={BU_RES_X-24,BU_RES_Y+4,248,372};buAlvoResultado(r,NULL,0,0);
  assert(ordinary==1 && !clipped && target.x==r.x && target.w==r.w);
  printf("TV %.3f %.3f %.3f %.3f %d %d %.3f %.3f\n",
         BU_RES_X,BU_RES_Y,BU_DIR,BU_RES_AREA_H,kicks,firstStyle,target.x,target.w);
}

int main(void) {
  layout_modo_definir(0); layout_tela_definir(1920,1080);tv();
#ifdef NV_TOUCH_UI
  const int sizes[][2]={{1080,2340},{1920,1080},{1600,1000},{1080,1080}};
  for(int i=0;i<4;i++) {
    layout_modo_definir(1);layout_tela_definir(sizes[i][0],sizes[i][1]);
    buInterfaceAtualizar();
    assert(buTelefone() && buUmaColuna() && BU_CAMPO_H==104);
    kicks=lines=clipped=ordinary=0;
    buCabecalho(0,BU_RES_Y,BU_DIR-BU_RES_X);
    assert(!kicks && lines==2 && firstStyle==TXT_ILHA_SECAO);
    GfxRect r={BU_RES_X-24,BU_RES_Y+4,248,372};buAlvoResultado(r,NULL,0,0);
    assert(!ordinary && clipped==1 && target.x==BU_RES_X && target.w==224);
    toqueLivreY=toqueLivreX[0]=1;toqueFileira=0;
    scrollY=scrollAlvo=scrollX[0]=velY=velX[0]=150;
    layout_modo_definir(0);buInterfaceAtualizar();
    assert(!toqueLivreY && !toqueLivreX[0] && toqueFileira==-1);
    assert(!scrollY && !scrollAlvo && !scrollX[0] && !velY && !velX[0]);
    assert(keyboardClosed==i+1);
    tv();
  }
#endif
  return 0;
}
