/* Draw the production Library content with measured, inert text and art. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/biblioteca.c"
#include <assert.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
float gfx_tex_aspect_atual, gfx_varre_atual;
static float fonteMedida = 1;
static int fixada, desenhos, falhas, nTexto;
static GfxRect limite;
static struct { GfxRect r; char texto[256]; } textos[64];
static char strings[128][256];
static int nStrings;

static void recorde(GfxRect r, GLuint id) {
  if (!id || id > (GLuint)nStrings) return;
  assert(nTexto < 64);
  textos[nTexto].r = r;
  snprintf(textos[nTexto++].texto,256,"%s",strings[id-1]);
  if (r.x < limite.x-.1f || r.y < limite.y-.1f ||
      r.x+r.w > limite.x+limite.w+.1f || r.y+r.h > limite.y+limite.h+.1f) {
    printf("outside: %s %.1f,%.1f %.1fx%.1f; bounds %.1f,%.1f %.1fx%.1f\n",
           strings[id-1],r.x,r.y,r.w,r.h,limite.x,limite.y,limite.w,limite.h);
    falhas++;
  }
  desenhos++;
}
static int altura(TxtEstilo e) {
  if (e == TXT_CALLOUT) return NV_FT_CALLOUT;
  if (e == TXT_HEADLINE) return NV_FT_HEADLINE;
  if (e == TXT_HERO_SEC) return NV_FT_HERO_SEC;
  return NV_FT_CAPTION2;
}
const char *i18n(const char *s) { return s; }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)r;(void)g;(void)b;(void)a; assert(nStrings < 128);
  snprintf(strings[nStrings],256,"%s",s);
  return (TxtLinha){.tex=(GLuint)++nStrings,.w=(int)(strlen(s)*altura(e)*.52f*fonteMedida),.h=altura(e)};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float max) {
  if (max <= 0) { printf("nonpositive text budget: %s %.1f\n",s,max);falhas++;max=1; }
  TxtLinha t = txt_linha(e,s,r,g,b,a); if (t.w > max) t.w = (int)max; return t;
}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a) { if(a > .001f)recorde((GfxRect){x,y,t.w,t.h},t.tex); }
void txt_desenhar(TxtLinha t,float x,float y) { txt_desenhar_alpha(t,x,y,1); }
float txt_bloco(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float entre,float a,int max) {
  TxtLinha t=txt_linha(e,s,r,g,b,255); int linhas=(int)ceilf(t.w/w);
  if(max && linhas>max)linhas=max; if(linhas<1)linhas=1;
  if(t.w>w)t.w=(int)w; t.h=altura(e)+(int)(entre*(linhas-1));
  txt_desenhar_alpha(t,x,y,a);return linhas*entre;
}
float txt_bloco_corta(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float passo,float a,int max) {
  return txt_bloco(e,s,r,g,b,x,y,w,passo,a,max);
}
void gfx_rect(GfxRect r,GLuint tex,GfxModo modo,float f,float px,float py,float raio,float cr,float cg,float cb,float a) {
  (void)f;(void)px;(void)py;(void)raio;(void)cr;(void)cg;(void)cb;
  if(modo==GFX_TEXTO && a>.001f)recorde(r,tex);
}
void gfx_cor(GfxRect r,float raio,float cr,float cg,float cb,float a) {(void)r;(void)raio;(void)cr;(void)cg;(void)cb;(void)a;}
void gfx_vidro_pilula_cheia(GfxRect r,float raio,float f,float a) {(void)r;(void)raio;(void)f;(void)a;}
void gfx_vidro_cartao(GfxRect r,float raio,float f,float a) {(void)r;(void)raio;(void)f;(void)a;}
void gfx_esqueleto(GfxRect r,float raio,float cr,float cg,float cb,float a) {(void)r;(void)raio;(void)cr;(void)cg;(void)cb;(void)a;}
void gfx_anel(GfxRect r,float raio,float esp,float cr,float cg,float cb,float a) {(void)r;(void)raio;(void)esp;(void)cr;(void)cg;(void)cb;(void)a;}
int ajustes_vidro(void) {return 0;}
int ajustes_tinta_foco(void) {return 255;}
int ajustes_borda_foco(void) {return 1;}
void ajustes_acento(float *r,float *g,float *b) {*r=1;*g=.6f;*b=.2f;}
void ajustes_area_conteudo(float esq,float dir,float *x,float *w) {if(x)*x=esq;if(w)*w=NV_TELA_W-esq-dir;}
float gfx_escala_ui(void) {return 1;}
int lst_fixada(const LstLista *l) {(void)l;return fixada;}
const char *extras_caminho_marca_nome(const char *s) {(void)s;return NULL;}
const char *posterprov_card_addon(const char *o,const char *i,long t,const char *tipo,const char *url) {(void)o;(void)i;(void)t;(void)tipo;return url;}
GLuint tex_obter_larg(const char *s,float w) {(void)s;(void)w;return 0;}
float tex_aspecto(const char *s) {(void)s;return 2.0f/3;}
int tex_falhou(const char *s) {(void)s;return 1;}
Uint32 SDL_GetTicks(void) {return 1;}
float badge_imdb_largura(int nota) {(void)nota;return 100;}
float badge_imdb(float x,float y,int nota,int escuro,float a) {(void)x;(void)y;(void)nota;(void)escuro;(void)a;return 100;}

static int sobrepoe(GfxRect a,GfxRect b) {
  return a.x < b.x+b.w-.1f && a.x+a.w > b.x+.1f && a.y < b.y+b.h-.1f && a.y+a.h > b.y+.1f;
}
static void validar(void) {
  for(int i=0;i<nTexto;i++)for(int j=i+1;j<nTexto;j++)if(sobrepoe(textos[i].r,textos[j].r)) {
    printf("overlap: %s / %s at %.1f,%.1f %.1fx%.1f / %.1f,%.1f %.1fx%.1f\n",textos[i].texto,textos[j].texto,
      textos[i].r.x,textos[i].r.y,textos[i].r.w,textos[i].r.h,textos[j].r.x,textos[j].r.y,textos[j].r.w,textos[j].r.h);
    falhas++;
  }
}
static void reset(GfxRect r) {limite=r;nStrings=nTexto=0;}
int main(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  LstLista l={.fonte=LST_NUVIO,.itens=1234};
  snprintf(l.titulo,sizeof l.titulo,"Uma lista com nome comprido para caber em duas linhas completas");
  snprintf(l.autor,sizeof l.autor,"Pessoa com um nome de autor bem comprido");
  CatItem ci={.nota=87,.progresso=67};
  snprintf(ci.titulo,sizeof ci.titulo,"Um titulo comprido que deve respeitar as colunas da biblioteca");
  snprintf(ci.sinopse,sizeof ci.sinopse,"Uma sinopse comprida, mas limitada a largura disponivel da linha em foco.");
  snprintf(ci.meta,sizeof ci.meta,"2025");
  snprintf(ci.genero,sizeof ci.genero,"Drama · Misterio");
  int casos=0;
  for(int t=0;t<4;t++)for(int f=0;f<2;f++)for(int p=0;p<2;p++)for(int mix=0;mix<2;mix++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];fonteMedida=f?1.3f:1;fixada=p;temAberta=0;modo=MODO_LISTAS;
    fonte=mix?FONTE_FIXADAS:FONTE_NUVIO;
    GfxRect r={bibX(),600,larguraCartaoLista(),BIB_LC_H};
    reset(r);desenhaCartaoLista(&l,r,0,1);validar();casos++;
    reset((GfxRect){bibX(),600,bibW(),BIB_LL_H});desenhaLinhaLista(&l,600,0,1);validar();casos++;
    reset((GfxRect){bibX(),600,bibW(),BIB_LIN_H});desenhaLinhaTitulo(&ci,600,1,1);validar();casos++;
    GfxRect cartaz={bibX(),600,BIB_CARD_W,BIB_POSTER_H};
    reset((GfxRect){cartaz.x,cartaz.y,cartaz.w,cartaz.h+BIB_TIT_GAP+14+52});
    desenhaCartaz(&ci,cartaz,p,1,NULL,1);validar();casos++;
  }
  printf("biblioteca_content_review: %d actual content cases, %d text operations, %d violations\n",casos,desenhos,falhas);
  return falhas?1:0;
}
