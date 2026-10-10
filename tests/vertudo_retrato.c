#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/vertudo.c"
#include "../src/ctxlista.c"
#include <assert.h>

float nv_layout_w=1080,nv_layout_h=2340,gfx_tex_aspect_atual;
float nv_cor_fundo_viva[3]={.04f,.045f,.06f};
static float rail, fonte=12;
static ColFolder pasta;
static ColSource fontes[COL_SOURCE_MAX];
static CatItem item;
static GfxRect corte,artDesenhada;
static int cortando,nAlvos,nCards,nPainel,paginas;
static PonteiroAlvo alvos[100];
float ajustes_conteudo_x(void) { return 104+rail; }
float ajustes_rail_largura_fixa(void) { return rail; }
float gfx_escala(void) { return 1; }
int ajustes_vidro(void) { return 0; }
int ajustes_borda_foco(void) { return 1; }
float ajustes_raio_poster_px(void) { return 24; }
int ajustes_tinta_foco(void) { return 24; }
int ajustes_tinta_foco2(void) { return 40; }
int ajustes_idioma(void) { return 0; }
void ajustes_acento(float *r,float *g,float *b) { *r=.5f;*g=.6f;*b=.8f; }
const char *i18n(const char *s) { return s; }
int desc_vertudo_n(void) { return 16; }
int desc_vertudo_item(int i,CatItem *out) { if(i<0||i>=16)return 0;*out=item;return 1; }
int desc_vertudo_erro(void) { return 0; }
int desc_vertudo_fim(void) { return 1; }
int desc_vertudo_carregando(void) { return 0; }
void desc_vertudo_mais(void) { paginas++; }
const char *desc_nome_catalogo(const char *base,const char *type,const char *id) { (void)base;(void)type;return id; }
const char *addons_base_por_id(const char *id) { (void)id;return ""; }
void col_nome_fonte(const ColSource *s,const char *manifest,char *out,unsigned n) { (void)manifest;snprintf(out,n,"%s",s->title); }
void col_cor(const ColFolder *f,float *r,float *g,float *b) { (void)f;*r=.5f;*g=.6f;*b=.8f; }
const char *col_banner(const ColFolder *f) { (void)f;return ""; }
const char *posterprov_card_addon(const char *origem,const char *imdb,long tmdb,const char *tipo,const char *url) { (void)origem;(void)imdb;(void)tmdb;(void)tipo;return url; }
GLuint tex_obter_larg(const char *s,float w) { (void)w;return s[0]?1:0; }
GLuint tex_obter_hero(const char *s) { return s[0]?1:0; }
GLuint tex_obter_logo_larg(const char *s,float w) { (void)s;(void)w;return 0; }
float tex_aspecto(const char *s) { (void)s;return 16.0f/9; }
int tex_marca_escura(const char *s) { (void)s;return 0; }
void diretor_pedir(const char *s) { (void)s; }
const char *diretor_foto(const char *s) { (void)s;return "director"; }
int fundo_modo(void) { return FUNDO_FROST; }
void fundo_desenhar_modo(int modo,GfxRect r,float rad,const char *url,float a) { (void)modo;(void)r;(void)rad;(void)url;(void)a; }
uint64_t badges_provedor(const char *s) { (void)s;return 0; }
float badges_desenhar(uint64_t mask,float x,float y,float w,float h,float a) { (void)mask;(void)x;(void)y;(void)w;(void)h;(void)a;return 0; }
int imdbnota_obter(const char *id,int n,int series) { (void)id;(void)n;(void)series;return 0; }
int ponteiro_ativo(void) { return 1; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { assert(fn==toqueVertudoRolar); }
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int i,int j) {
  assert(nAlvos<100 && w>0 && h>0 && x>=0 && x+w<=NV_TELA_W+.01f);
  assert(y>=0 && y+h<=NV_TELA_H+.01f);
  alvos[nAlvos++]=(PonteiroAlvo){x,y,w,h,f,a,i,j,0,NULL};
}
void ponteiro_alvo_faixa(float x,float y,float w,float h,float top,float bot,PonteiroFn f,PonteiroFn a,int i,int j) {
  float t=fmaxf(y,top),b=fminf(y+h,bot);if(b>t)ponteiro_alvo(x,t,w,b-t,f,a,i,j);
}
void gfx_recorte(float x,float y,float w,float h) { assert(w>0&&h>0&&x>=0&&x+w<=NV_TELA_W+.01f);corte=(GfxRect){x,y,w,h};cortando=1; }
void gfx_sem_recorte(void) { cortando=0; }
void gfx_cor(GfxRect r,float rad,float cr,float cg,float cb,float ca) { (void)rad;(void)cr;(void)cg;(void)cb;(void)ca;assert(r.w>=0&&r.h>=0); }
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float px,float py,float rad,float cr,float cg,float cb,float ca) {
  (void)t;(void)f;(void)px;(void)py;gfx_cor(r,rad,cr,cg,cb,ca);
  if(m==GFX_CARD && timeline && cortando) { assert(r.x>=vtInicio() && r.x+r.w<=vtFim());if(!nCards)artDesenhada=r;nCards++; }
  if(m==GFX_CARD && timeline && !cortando)nPainel++;
  if(m==GFX_RETRATO && vtTelefone())assert(r.x>=0&&r.x+r.w<=NV_TELA_W);
}
void gfx_vidro_cartao(GfxRect r,float rad,float f,float a) { (void)f;gfx_cor(r,rad,0,0,0,a); }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)e;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){.w=(int)(strlen(s)*fonte),.h=28};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) {
  assert(w>0);TxtLinha l=txt_linha(e,s,r,g,b,a);if(l.w>w)l.w=(int)w;return l;
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) {
  (void)y;(void)a;if(!cortando)assert(x>=0&&x+l.w<=NV_TELA_W+.01f);
  else if(corte.y>310)assert(x>=vtInicio()&&x+l.w<=vtFim()+.01f);
}
float txt_bloco(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float h,float a,int max) {
  TxtLinha l=txt_linha_corta(e,s,r,g,b,255,w);txt_desenhar_alpha(l,x,y,a);return max*h;
}
static void desenhar(void) { nAlvos=nCards=nPainel=0;cortando=0;vertudo_desenhar(500);assert(nCards>0);if(vtTimelineRetrato())assert(nPainel==0); }
int main(void) {
  anim_politica_reduzida=1;
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  strcpy(pasta.group,"Directors");strcpy(pasta.title,"Long director name and extended filmography heading");
  pasta.sources=fontes;pasta.nSources=16;
  for(int i=0;i<16;i++) { snprintf(fontes[i].title,sizeof fontes[i].title,"Source catalog number%d with a longer label",i);strcpy(fontes[i].type,i%2?"series":"movie"); }
  strcpy(item.titulo,"A very long film title in this director filmography");strcpy(item.meta,"2024");
  strcpy(item.genero,"Drama science fiction thriller");strcpy(item.sinopse,"Long synopsis with enough words to occupy the available card text area.");strcpy(item.backdrop,"art");
  collection=&pasta;timeline=aberta=1;anim=1;orderN=-1;ondaArmada=ondaEm=0;
  for(int d=0;d<4;d++)for(int r=0;r<2;r++)for(int f=0;f<2;f++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];rail=r?140:0;fonte=f?30:12;
    foco=source=tabCursor=tabFocus=0;scrollY=tabRol=0;toquerol_limpar(&toqueAbas);
    snprintf(titulo,sizeof titulo,"%s",pasta.title);desenhar();
    GfxRect origem=celulaRect(0,&item,(const char *[]){NULL});GfxRect esperado=vtTimelineArte(vtTimelineCard(VT_TOPO));
    assert(origem.x==esperado.x&&origem.y==esperado.y&&origem.w==esperado.w&&origem.h==esperado.h);
    assert(origem.x==artDesenhada.x&&origem.y==artDesenhada.y&&origem.w==artDesenhada.w&&origem.h==artDesenhada.h);
    assert(vtTimelineCard(VT_TOPO).x+vtTimelineCard(VT_TOPO).w<=vtFim());
    PonteiroRolagem e={PONT_ROL_INICIO,0,0,0,toqueAbas.regiao.x+20,toqueAbas.regiao.y+20};
    assert(toqueVertudoRolar(&e));e.fase=PONT_ROL_MOVER;e.delta=-37.5f;
    assert(toqueVertudoRolar(&e)&&fabsf(tabRol-37.5f)<.01f&&source==0&&tabCursor==0&&foco==0);
    e.fase=PONT_ROL_SOLTAR;toqueVertudoRolar(&e);desenhar();assert(fabsf(tabRol-37.5f)<.01f);
    e.fase=PONT_ROL_INERCIA;e.delta=-1e6f;toqueVertudoRolar(&e);assert(tabRol==toqueAbas.maximo);
    e.fase=PONT_ROL_FIM;toqueVertudoRolar(&e);desenhar();
    int last=0;for(int i=0;i<nAlvos;i++)if(alvos[i].focar==ponteiroAba&&alvos[i].a==15) { alvos[i].focar(15,0);assert(tabCursor==15&&source==0);last++; }
    assert(last==1);toquerol_limpar(&toqueAbas);desenhar();
    tabCursor=0;desenhar();assert(tabRol==0 && source==0);
    scrollY=toqueVertudoMax();desenhar();
    nv_layout_w=2520;nv_layout_h=1080;desenhar();assert(tabRol<=toqueAbas.maximo);
  }
  nv_layout_w=1920;nv_layout_h=1080;rail=0;
  assert(!vtTelefone());GfxRect original=vtTimelineCard(332);assert(original.x==262&&original.w==1000);
  puts("vertudo_retrato: OK");return 0;
}
