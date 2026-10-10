/* Use production row bounds and the stacked ranking renderer without GL. */
#define TESTE_HOME 1
#define TESTE_HOME_ROWS_REVIEW 1
#define SDL_MAIN_HANDLED 1
static int deitadosTeste;
static float uiTeste = 1;
static int homeRowsDeitados(void) { return deitadosTeste; }
static float homeRowsEscala(void) { return uiTeste; }
/* The common fixture declares/stubs these with (void); production calls
   them with (). Keep its defaults under a separate name, and route actual
   Home call sites to this matrix's mutable platform settings. */
#define ajustes_posteres_deitados(...) homeRowsDeitados##__VA_ARGS__()
#define gfx_escala_ui(...) homeRowsEscala##__VA_ARGS__()
#define main home_gesture_fixture_main
#include "catalogo_toque.c"
#undef main
#undef gfx_escala_ui
#undef ajustes_posteres_deitados
#include "../src/gifcolecao.c"

static CatItem pilhaItens[6];
static GfxRect pilhaLimite;
static int pilhaDesenhos, pilhaFalhas;
static ColFolder pastaTeste;
static int pastaDesenhando, pastaDesenhos, pastaCasos;
static int referenciaDesenhando, referenciaDesenhos;
static GfxRect referenciaMedida;
float gfx_tex_aspect_atual, gfx_card_forcar_cover_atual, gfx_varre_atual;
float gfx_escala_ui(void) { return uiTeste; }
void gfx_mascara_opaca(GfxRect r,float raio) { (void)r;(void)raio; }
GLuint gfx_desfocado(GLuint tex,const char *s) { (void)s;return tex; }
int tex_falhou(const char *s) { (void)s;return 0; }
int ajustes_cw_desfocar_proximo(void) { return 0; }
int ajustes_cw_ligado(void) { return 0; }
int trakt_e_a_seguir(const char *s) { (void)s;return 0; }
int simkl_e_a_seguir(const char *s) { (void)s;return 0; }
int cwo_conta_a_seguir(const char *s) { (void)s;return 0; }
const char *artehero_url_episodio(const CatItem *c) { (void)c;return NULL; }
void gfx_esqueleto(GfxRect r,float raio,float cr,float cg,float cb,float a) { (void)r;(void)raio;(void)cr;(void)cg;(void)cb;(void)a; }
const ColFolder *col_folder(int i) { assert(i == 0); return &pastaTeste; }
const char *col_capa(const ColFolder *p) { assert(p == &pastaTeste); return p->cover; }
GLuint tex_obter_passageira(const char *u,float w) { (void)u;(void)w;return 0; }
int tex_magica(const char *u,unsigned char m[4]) { (void)u;memset(m,0,4);return 0; }
const char *tex_arquivo(const char *u) { (void)u;return NULL; }
int gif_pode_animar(void) { return 0; }
int gif_animado(const char *p) { (void)p;return 0; }
GLuint gif_textura(const char *p,int w) { (void)p;(void)w;return 0; }
int gif_recusou(const char *p) { (void)p;return 0; }
void gif_parar(void) { }
int ajustes_animacoes_reduzidas(void) { return 1; }
int ajustes_vidro(void) { return 0; }
float ajustes_raio_poster_px(void) { return 24; }
void ajustes_acento(float *r,float *g,float *b) { *r=1;*g=.6f;*b=.2f; }
int ajustes_profundidade(void) { return 0; }
int ajustes_profundidade_posters(void) { return 0; }
float ajustes_profundidade_borda(void) { return 0; }
float ajustes_profundidade_brilho(void) { return 0; }
float ajustes_profundidade_cobertura(void) { return 0; }
void gfx_brilho_topo(GfxRect r,float raio,float alcance,float cr,float cg,float cb,float a) { (void)r;(void)raio;(void)alcance;(void)cr;(void)cg;(void)cb;(void)a; }
void gfx_vidro_cartao(GfxRect r,float raio,float f,float a) { (void)r;(void)raio;(void)f;(void)a; }
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
  if(pastaDesenhando||referenciaDesenhando) {
    /* The real folder renderer must use the measured row rectangle. Its
       registered target is precisely the visible part of that artwork. */
    float x=homeRetratoTelefone()?fmaxf(r.x,homeConteudoX()):r.x;
    float direita=homeRetratoTelefone()?fminf(r.x+r.w,homeConteudoDireita()):r.x+r.w;
    float y=fmaxf(r.y,corteFileiras());
    assert(alvosCard==1);
    perto(alvoCartao.x,x);perto(alvoCartao.y,y);
    perto(alvoCartao.w,direita-x);perto(alvoCartao.h,r.y+r.h-y);
    if(pastaDesenhando) { perto(r.w,larguraFil(0));perto(r.h,alturaFil(0)); }
    else { perto(r.w,referenciaMedida.w);perto(r.h,referenciaMedida.h); }
    assert(r.w>0&&r.h>0&&raio>=0&&raio<=.5f);
    if(pastaDesenhando)pastaDesenhos++;else referenciaDesenhos++;
    return;
  }
  pilhaDesenhos++;
  if(r.x<pilhaLimite.x-.01f || r.x+r.w>pilhaLimite.x+pilhaLimite.w+.01f || r.y<pilhaLimite.y-.01f || r.y+r.h>pilhaLimite.y+pilhaLimite.h+.01f) {
    printf("stack art outside target: %.1f,%.1f %.1fx%.1f; target %.1f,%.1f %.1fx%.1f\n",r.x,r.y,r.w,r.h,pilhaLimite.x,pilhaLimite.y,pilhaLimite.w,pilhaLimite.h);pilhaFalhas++;
  }
}
static void testaPastas(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080},{1920,1080},{1728,1080},{1080,1728}};
  const float ui[]={1,1.2f,1.3f,1.5f}, tamanhos[]={.85f,1,1.2f};
  const int posterDp[]={72,126,200};
  snprintf(pastaTeste.cover,sizeof pastaTeste.cover,"folder.jpg");
  nFileiras=1;focoHero=0;foco.fileira=-1;scrollY=0;
  memset(scrollX,0,sizeof scrollX);memset(animFoco,0,sizeof animFoco);
  for(int t=0;t<7;t++)for(int u=0;u<4;u++)for(int l=0;l<3;l++)
    for(int rail=0;rail<2;rail++)for(int dp=0;dp<3;dp++)for(int z=0;z<3;z++)for(int deitado=0;deitado<2;deitado++) {
      nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];uiTeste=ui[u];
      layoutTeste=l;railTeste=rail?140:0;posterDpTeste=posterDp[dp];
      /* Home paints in the raw canvas at every global UI size. Its row
         preference still scales artwork, with the existing portrait cap. */
      assert(gfx_escala_ui()==ui[u]);
      deitadosTeste=0;
      fileiras[0]=(Fileira){.tipo=FILEIRA_NORMAL,.escala=1,.n=1};
      float posterW=larguraFil(0),posterH=alturaFil(0);
      deitadosTeste=1;
      fileiras[0].escala=tamanhos[z];
      float largoW=larguraFil(0),largoH=alturaFil(0);
      /* Compare against real ordinary-art measurement AND drawing, for
         both title preferences. Landscape reference includes its border
         envelope (LAND_H), rather than the inset artwork (LAND_ART). */
      for(int modo=0;modo<2;modo++) {
        deitadosTeste=modo;
        assert(homeRowsDeitados()==modo);
        fileiras[0]=(Fileira){.tipo=FILEIRA_NORMAL,.escala=tamanhos[z],.n=1};
        referenciaMedida=(GfxRect){0,0,larguraFil(0),alturaFil(0)};
        if(modo) {
          perto(referenciaMedida.w,largoW);perto(referenciaMedida.h,largoH);
          perto(referenciaMedida.w/referenciaMedida.h,NV_CARD_LAND_W/NV_CARD_LAND_H);
          assert(NV_CARD_LAND_H>NV_CARD_LAND_ART);
        }
        for(int recorte=0;recorte<2;recorte++)for(int focoTeste=0;focoTeste<2;focoTeste++) {
          scrollX[0]=recorte?referenciaMedida.w*.4f:0;
          referenciaMedida.x=homeFileiraX(0,0);referenciaMedida.y=corteFileiras()+(recorte?-42:32);
          alvosCard=0;alvoCard(referenciaMedida.x,referenciaMedida.y,referenciaMedida.w,referenciaMedida.h,0,0);
          int antes=referenciaDesenhos;referenciaDesenhando=1;
          desenhaArteCard(referenciaMedida,FILEIRA_NORMAL,"ordinary.jpg",1,NULL,(float)focoTeste,raioDe(referenciaMedida.w,referenciaMedida.h),1,0);
          referenciaDesenhando=0;assert(referenciaDesenhos==antes+1&&alvosCard==1);
        }
        scrollX[0]=0;
      }
      deitadosTeste=deitado;
      float basePosterW=posterW,basePosterH=posterH;
      if(l!=HOME_LAYOUT_PADRAO||deitado) {
        /* Preserve the existing upright shape bases, including Standard
           switching off its larger portrait preset when titles are wide. */
        float e=escalaDoAjuste()*escalaPosterTelefone();
        basePosterW=e*NV_CARD_W;basePosterH=e*NV_CARD_H;
      }
      for(int forma=0;forma<COL_FORMA_N;forma++) {
        fileiras[0]=(Fileira){.tipo=FILEIRA_CATALOGOS,.forma=forma,.escala=tamanhos[z],.n=1};
        float w=larguraFil(0),h=alturaFil(0),w0,h0;
        medidaColecao(forma,&w0,&h0);
        /* Square and poster keep the chosen upright bases even while
           titles use wide cards. Landscape equals the wide outer card. */
        float esperadoH=forma==COL_FORMA_PAISAGEM?largoH:basePosterH*tamanhos[z];
        float esperadoW=forma==COL_FORMA_POSTER?basePosterW*tamanhos[z]
                       :forma==COL_FORMA_QUADRADO?esperadoH:largoW;
        if(homeRetratoTelefone()&&esperadoW>homeLarguraUtil()*.9f) {
          esperadoH*=homeLarguraUtil()*.9f/esperadoW;
          esperadoW=homeLarguraUtil()*.9f;
        }
        if(telefoneui_ativo()||forma!=COL_FORMA_PAISAGEM) {
          /* The accepted old phone implementation fails here: portrait
             poster height made its landscape folder about 1.78x wider
             and taller than the actual ordinary wide-title envelope. */
          if(fabsf(w-esperadoW)>=.01f||fabsf(h-esperadoH)>=.01f)
            fprintf(stderr,"folder size mismatch: %.0fx%.0f UI%.0f layout%d rail%d dp%d row%.2f titles-wide%d shape%d actual %.3fx%.3f expected %.3fx%.3f; actual wide-title %.3fx%.3f, portrait base %.3fx%.3f\n",
                   nv_layout_w,nv_layout_h,ui[u]*100,l,rail,posterDp[dp],tamanhos[z],deitado,forma,w,h,esperadoW,esperadoH,largoW,largoH,posterW,posterH);
          perto(w,esperadoW);perto(h,esperadoH);
        } else { perto(w,360*tamanhos[z]);perto(h,203*tamanhos[z]); }
        if(forma==COL_FORMA_PAISAGEM)perto(w/h,telefoneui_ativo()?NV_CARD_LAND_W/NV_CARD_LAND_H:360.0f/203);
        else if(forma==COL_FORMA_QUADRADO)perto(w,h);
        else perto(w/h,w0/h0);
        if(homeRetratoTelefone())assert(w<=homeLarguraUtil()*.9f+.01f);
        perto(passoFil(0),w+gapDe(FILEIRA_CATALOGOS));
        perto(homeFileiraX(0,1)-homeFileiraX(0,0),passoFil(0));
        for(int recorte=0;recorte<2;recorte++)for(int focoTeste=0;focoTeste<2;focoTeste++) {
          scrollX[0]=recorte?w*.4f:0;animFoco[0][0]=(float)focoTeste;
          float y=corteFileiras()+(recorte?-42:32);
          alvosCard=0;pastaDesenhando=1;
          int desenhosAntes=pastaDesenhos;
          desenhaAtalhos(0,y);
          pastaDesenhando=0;
          assert(pastaDesenhos==desenhosAntes+1&&alvosCard==1);
          pastaCasos++;
        }
        scrollX[0]=0;animFoco[0][0]=0;
      }
    }
  printf("home_rows_review: %d actual folder art/target cases, %d actual upright/wide ordinary-art/target draws, all three shapes, titles-wide off/on, UI100/120/130/150, phone and TV/tablet bounds\n",pastaCasos,referenciaDesenhos);
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
  testaPastas();
  return pilhaFalhas?1:0;
}
