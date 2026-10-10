#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/spotlight.c"
#include <assert.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
float gfx_opacidade_grupo = 1, gfx_tex_aspect_atual;
static float escalaUi = 1, escalaAtiva = 1;
static int ime, voz, celular, nAlvos, chamadasIme, letrasLargas;
static GfxRect clip;
static int recortando;
static PonteiroAlvo alvos[256];
static CatItem item;
static const char *alfabeto = "abcdefghijklmnopqrstuvwxyz0123456789";

float gfx_escala_ui(void) { return escalaUi; }
float gfx_escala(void) { return escalaAtiva; }
void gfx_escala_sair(float s) { escalaAtiva = s; }
int ponteiro_ativo(void) { return 1; }
void ponteiro_camada(void) { nAlvos = 0; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { assert(fn == toqueSpotRolar); }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn f, PonteiroFn a, int i, int j) {
  assert(nAlvos < 256 && w > 0 && h > 0);
  assert(x >= 0 && x+w <= NV_TELA_W+.01f && y >= 0 && y+h <= NV_TELA_H+.01f);
  alvos[nAlvos++] = (PonteiroAlvo){x,y,w,h,f,a,i,j,0,NULL};
}
void ponteiro_alvo_faixa(float x, float y, float w, float h, float top, float bottom,
                         PonteiroFn f, PonteiroFn a, int i, int j) {
  float t = fmaxf(y,top), b = fminf(y+h,bottom);
  if (b > t) ponteiro_alvo(x,t,w,b-t,f,a,i,j);
}
void gfx_recorte(float x, float y, float w, float h) {
  assert(w >= 0 && h >= 0 && x >= 0 && x+w <= NV_TELA_W+.01f);
  assert(y >= 0 && y+h <= NV_TELA_H+.01f);
  clip = (GfxRect){x,y,w,h}; recortando = 1;
}
void gfx_sem_recorte(void) { recortando = 0; }
void gfx_cor(GfxRect r, float radius, float cr, float cg, float cb, float ca) {
  (void)radius; (void)cr; (void)cg; (void)cb; (void)ca; assert(r.w >= 0 && r.h >= 0);
}
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float f, float px, float py, float rad,
              float cr, float cg, float cb, float ca) {
  (void)t; (void)m; (void)f; (void)px; (void)py; gfx_cor(r,rad,cr,cg,cb,ca);
}
void gfx_icone(GfxRect r, const char *s, float cr, float cg, float cb, float ca) { (void)s; gfx_cor(r,0,cr,cg,cb,ca); }
void gfx_vidro_folha(GfxRect r, float rad, float a) { gfx_cor(r,rad,0,0,0,a); }
void gfx_luz_canto(GfxRect r, float rad, float x, float y, float w, float cr, float cg, float cb, float ca) {
  (void)x; (void)y; (void)w; gfx_cor(r,rad,cr,cg,cb,ca);
}
void gfx_esqueleto(GfxRect r, float rad, float cr, float cg, float cb, float ca) { gfx_cor(r,rad,cr,cg,cb,ca); }
const char *i18n(const char *s) { return s; }
int txt_largura(TxtEstilo e, const char *s) { (void)e; return (int)strlen(s)*(letrasLargas?30:12); }
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) {
  (void)r;(void)g;(void)b;(void)a; return (TxtLinha){.w=txt_largura(e,s),.h=24};
}
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float w) {
  assert(w > 0); TxtLinha l=txt_linha(e,s,r,g,b,a); if(l.w>w)l.w=(int)w;return l;
}
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) {
  (void)a; assert(l.w >= 0 && l.h >= 0);
  if (!recortando) { assert(x >= 0 && x+l.w <= NV_TELA_W+.51f); assert(y >= 0 && y+l.h <= NV_TELA_H+.51f); }
  else { assert(clip.w > 0 && clip.h > 0); }
}
float txt_tracking(TxtEstilo e, const char *s, int r, int g, int b, float x, float y, float a, float gap) {
  (void)gap; TxtLinha l=txt_linha(e,s,r,g,b,255);if(x>=0)txt_desenhar_alpha(l,x,y,a);return l.w;
}
float txt_bloco(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float h,float a,int max) {
  TxtLinha l=txt_linha_corta(e,s,r,g,b,255,w); l.h=(int)(h*max);txt_desenhar_alpha(l,x,y,a);return l.h;
}
int ajustes_idioma(void) { return 0; }
Uint32 SDL_GetTicks(void) { return 100; }
int ajustes_vidro(void) { return 0; }
int ajustes_tinta_foco(void) { return 24; }
void ajustes_acento(float *r,float *g,float *b) { *r=.5f;*g=.6f;*b=.8f; }
int st_ime_disponivel(void) { return ime; }
int st_voz_disponivel(void) { return voz; }
int celb_disponivel(void) { return celular; }
int st_dono(void) { return ST_SPOT; }
int st_estado(void) { return ST_PARADO; }
const char *st_aviso(void) { return ""; }
int st_ime_abrir(int owner,const char *value,int max) { assert(owner==ST_SPOT && value==consulta && max==SP_MAX_TXT-1);chamadasIme++;return 1; }
void celb_botao(int owner,GfxRect r,int selected,PonteiroFn f,int i,int j,float a) {
  (void)owner;(void)selected;(void)a;ponteiro_alvo(r.x,r.y,r.w,r.h,f,NULL,i,j);
}
const char *teclado_alfabeto(void) { return alfabeto; }
const CatItem *cat_item(int i) { (void)i; return &item; }
GLuint tex_obter_larg(const char *s,float w) { (void)s;(void)w;return 0; }
float tex_aspecto(const char *s) { (void)s;return 1.5f; }
int tex_falhou(const char *s) { (void)s;return 0; }
void guia_logo_desenhar(const char *url,const char *name,GfxRect r,float w,float h,float brilho,float a) {
  (void)url;(void)name;(void)w;(void)h;(void)brilho;gfx_cor(r,0,0,0,0,a);
}
void ajustes_previa_busca(int op,float x,float y,float w,float h,float a) { (void)op;gfx_cor((GfxRect){x,y,w,h},0,0,0,0,a); }
void ajustes_guia_imagem(int op,float x,float y,float w,float h) { (void)op;gfx_cor((GfxRect){x,y,w,h},0,0,0,0,1); }

static void desenhar(int keyboard) {
  kbAberto=keyboard;kbAnim=keyboard;spBW=keyboard?SP_BW_KB:SP_BW_BASE;
  entrada=1;aberto=1;painel=keyboard?P_TECLADO:P_CAMPO;
  corpoH=corpoAlvo();recortando=0;spot_desenhar(100,1);
  assert(nAlvos>1 && escalaAtiva==1);
}
int main(void) {
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  const float zoom[]={1,1.2f,1.3f,1.5f};
  for(int d=0;d<4;d++)for(int z=0;z<4;z++)for(int wide=0;wide<2;wide++)for(int input=0;input<8;input++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];escalaUi=zoom[z];letrasLargas=wide;
    ime=input&1;voz=(input>>1)&1;celular=(input>>2)&1;
    alfabeto=input&1?"abcdefghijklmnopqrstuvwxyz0123456789":"абвгдеёжзийклмнопрстуфхцчшщъыьэюя0123456789";
    kbMontar();kbRol=0;toquerol_limpar(&toqueSpot);toquerol_limpar(&toqueTeclado);
    nLin=16;float rowY=0;for(int i=0;i<nLin;i++) { int type=i==0?L_CAB:(i==1?L_TOPO:i%13);
      lin[i]=(Linha){.tipo=type,.y=rowY,.h=ALTURA[type]};rowY+=lin[i].h;
      snprintf(lin[i].t1,sizeof lin[i].t1,"Long result and translated title that stays inside its viewport");
      snprintf(lin[i].t2,sizeof lin[i].t2,"Long metadata description for the result");entraLin[i]=1;animLin[i]=i==1; }
    strcpy(item.backdrop,"art"); strcpy(item.genero,"Drama adventure thriller science fiction");
    strcpy(consulta,"long search query with spaces");nConsulta=(int)strlen(consulta);
    desenhar(0);
    for(int best=0;best<2;best++) {
      lin[1].tipo=best?L_GUIA:L_AJUSTE;lin[1].ref2=1;lin[1].h=222;
      strcpy(lin[1].base,"Long explanatory text about a setting or help resource, with a highlighted search term.");
      desenhar(0);
    }
    lin[1].tipo=L_TOPO;lin[1].ref2=0;lin[1].h=ALTURA[L_TOPO];
    if(ime) { chamadasIme=0;okCampo();assert(chamadasIme==1 && !kbAberto); }
    desenhar(1);
    assert(toqueTeclado.regiao.y+toqueTeclado.regiao.h <= NV_TELA_H-SP_RODAPE_H+.01f);
    if(toqueTeclado.maximo>0) {
      PonteiroRolagem e={PONT_ROL_INICIO,1,0,0,(toqueTeclado.regiao.x+20)*toqueTeclado.escala,(toqueTeclado.regiao.y+20)*toqueTeclado.escala};
      assert(toqueSpotRolar(&e));int oldF=kbF,oldC=kbC;e.fase=PONT_ROL_MOVER;e.delta=-37.5f;
      assert(toqueSpotRolar(&e));assert(fabsf(kbRol-37.5f/toqueTeclado.escala)<.01f && oldF==kbF && oldC==kbC);
      e.fase=PONT_ROL_INERCIA;e.delta=-1e6f;toqueSpotRolar(&e);assert(kbRol==toqueTeclado.maximo);
      e.fase=PONT_ROL_FIM;toqueSpotRolar(&e);desenhar(1);
    }
    int command=0;for(int i=0;i<nAlvos;i++)if(alvos[i].focar==focarTecla && alvos[i].a==kbFil) {
      command++;alvos[i].focar(alvos[i].a,alvos[i].b);assert(painel==P_TECLADO && kbF==kbFil && kbC==alvos[i].b);
    }
    assert(command==kbNCmd);
    if(ime) { chamadasIme=0;okCampo();assert(chamadasIme==1 && !kbAberto && painel==P_CAMPO); }
  }
  nv_layout_w=1920;nv_layout_h=1080;escalaUi=1;
  assert(!spTelefone() && SP_BY==120);spBW=SP_BW_KB;assert(SP_BW==1240);
  puts("spotlight_retrato: OK");return 0;
}
