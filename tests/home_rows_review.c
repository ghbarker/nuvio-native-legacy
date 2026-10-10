/* Use production row bounds and the stacked ranking renderer without GL. */
#define TESTE_HOME 1
#define SDL_MAIN_HANDLED 1
#define main home_gesture_fixture_main
#include "catalogo_toque.c"
#undef main

static CatItem pilhaItens[6];
static GfxRect pilhaLimite;
static int pilhaDesenhos, pilhaFalhas;
float gfx_tex_aspect_atual;
const CatItem *cat_item(int i) { return i>=0 && i<6 ? &pilhaItens[i] : NULL; }
int cat_n(void) {return 6;}
const char *posterprov_card_addon(const char *o,const char *i,long t,const char *tipo,const char *url) {(void)o;(void)i;(void)t;(void)tipo;return url;}
GLuint tex_obter_larg(const char *u,float w) {(void)u;(void)w;return 1;}
float tex_aspecto(const char *u) {(void)u;return 2.0f/3;}
const char *artehero_url_card_fonte(const CatItem *i,int f,int d) {(void)i;(void)f;(void)d;return NULL;}
const char *artehero_url_metahub_fundo(const CatItem *i) {(void)i;return NULL;}
int ajustes_hero_fonte(void) {return 0;}
int ajustes_hero_arte_diferente(void) {return 0;}
const char *i18n(const char *s) {return s;}
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {(void)e;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){.w=(int)strlen(s)*11,.h=24};}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float max) {TxtLinha t=txt_linha(e,s,r,g,b,a);assert(max>0);if(t.w>max)t.w=(int)max;return t;}
void txt_desenhar(TxtLinha l,float x,float y) {(void)l;(void)x;(void)y;}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) {(void)a;txt_desenhar(l,x,y);}
void gfx_cor(GfxRect r,float raio,float cr,float cg,float cb,float a) {(void)r;(void)raio;(void)cr;(void)cg;(void)cb;(void)a;}
void gfx_rect(GfxRect r,GLuint tex,GfxModo modo,float f,float px,float py,float raio,float cr,float cg,float cb,float a) {
  (void)tex;(void)f;(void)px;(void)py;(void)raio;(void)cr;(void)cg;(void)cb;(void)a;
  if(modo!=GFX_CARD)return;
  pilhaDesenhos++;
  if(r.x<pilhaLimite.x-.01f || r.x+r.w>pilhaLimite.x+pilhaLimite.w+.01f || r.y<pilhaLimite.y-.01f || r.y+r.h>pilhaLimite.y+pilhaLimite.h+.01f) {
    printf("stack art outside target: %.1f,%.1f %.1fx%.1f; target %.1f,%.1f %.1fx%.1f\n",r.x,r.y,r.w,r.h,pilhaLimite.x,pilhaLimite.y,pilhaLimite.w,pilhaLimite.h);pilhaFalhas++;
  }
}
int main(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float tamanhos[]={.85f,1,1.2f};
  const TipoFileira tipos[]={FILEIRA_NORMAL,FILEIRA_CONTINUE,FILEIRA_DESTAQUE,FILEIRA_DESTAQUE_QUADRADO,FILEIRA_TOP10,FILEIRA_TOP10_NUM,FILEIRA_COLECAO,FILEIRA_SERVICO,FILEIRA_SOCIAL,FILEIRA_RETORNO,FILEIRA_CATALOGOS,FILEIRA_LARGA};
  int casos=0;
  for(int i=0;i<6;i++)snprintf(pilhaItens[i].poster,sizeof pilhaItens[i].poster,"poster%d",i);
  nFileiras=1;railTeste=0;posterDpTeste=126;focoHero=0;scrollY=0;
  for(int t=0;t<4;t++)for(int l=0;l<3;l++)for(int z=0;z<3;z++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];layoutTeste=l;
    for(int s=0;s<12;s++) {
      fileiras[0]=(Fileira){.tipo=tipos[s],.escala=tamanhos[z],.n=6,.stackN=tipos[s]==FILEIRA_TOP10?6:0};
      float w=larguraFil(0),h=alturaFil(0);
      assert(w>0&&h>0);
      if(homeRetratoTelefone())assert(w+xOffTipo(tipos[s])<=homeLarguraUtil()+.01f);
      casos++;
      if(tipos[s]==FILEIRA_TOP10) {
        pilhaLimite=(GfxRect){homeConteudoX(),800,w,h};
        desenhaPilha(0,6,pilhaLimite.x,pilhaLimite.y,h,1);
      }
    }
  }
  printf("home_rows_review: %d production row geometry cases, %d actual stack artwork draws, %d violations\n",casos,pilhaDesenhos,pilhaFalhas);
  return pilhaFalhas?1:0;
}
