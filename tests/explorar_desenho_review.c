/* Seeded actual Explore drawing; the mood labels come from mapa.c itself. */
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#define NV_MAPA_PURO 1
#include "../src/mapa.c"
#include "../src/explorar.c"
#include <assert.h>

float nv_layout_w=1080,nv_layout_h=2340,gfx_tex_aspect_atual;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
static float fontWide=1;
static float uiScale=1, activeScale=1;
static int longWordFixture;
static int fullBlockGlyphs;
static GfxRect clip;
static int clipped,nTargets,draws,failures,phase;
static PonteiroAlvo targets[80];
static void problem(const char *what,float x,float y,float w,float h) {
  if (failures < 24) fprintf(stderr,"%s phase%d viewport%.0fx%.0f wide%.2f rect %.1f %.1f %.1f %.1f\n",what,phase,nv_layout_w,nv_layout_h,fontWide,x,y,w,h);
  failures++;
}
static void drawn(GfxRect r) {
  r.x*=activeScale;r.y*=activeScale;r.w*=activeScale;r.h*=activeScale;
  if (clipped) {
    float x=fmaxf(r.x,clip.x),y=fmaxf(r.y,clip.y);
    float w=fminf(r.x+r.w,clip.x+clip.w)-x,h=fminf(r.y+r.h,clip.y+clip.h)-y;
    if(w<=0||h<=0)return; r=(GfxRect){x,y,w,h};
  }
  if(r.x<-.01f||r.y<-.01f||r.x+r.w>NV_TELA_W+.01f||r.y+r.h>NV_TELA_H+.01f)problem("draw outside",r.x,r.y,r.w,r.h);
  draws++;
}
int ajustes_vidro(void){return 0;}
int ajustes_borda_foco(void){return 0;}
int ajustes_tinta_foco(void){return 24;}
int ajustes_idioma(void){return 0;}
int ajustes_animacoes_reduzidas(void){return 1;}
float ajustes_conteudo_x(void){return 104;}
void ajustes_acento(float*r,float*g,float*b){*r=.6f;*g=.7f;*b=.8f;}
float gfx_escala(void){return activeScale;}
float gfx_escala_ui(void){return uiScale;}
float gfx_escala_entrar(void){float antes=activeScale;activeScale=uiScale;return antes;}
void gfx_escala_sair(float antes){activeScale=antes;}
int gfx_efeitos_leves(void){return 0;}
int gfx_snap_ok(void){return 0;}
int gfx_snap_ativo(void){return 0;}
unsigned gfx_snap_geracao(void){return 0;}
void gfx_snap_comecar(void){}
void gfx_snap_terminar(void){}
void gfx_snap_desenhar(void){}
int menu_pilula_titulo(void){return 0;}
int cat_n(void){return 20;}
const CatItem *cat_item(int i){(void)i;return NULL;}
const char*i18n(const char*s){
  if(longWordFixture&&!strcmp(s,mapa_clima_nome(4)))return "AdventureMysteryTimeline Across Worlds";
  return s;
}
GLuint tex_obter_larg(const char*s,float w){(void)s;(void)w;return 0;}
float tex_aspecto(const char*s){(void)s;return 2.0f/3;}
void gfx_cor(GfxRect r,float rad,float cr,float cg,float cb,float a){(void)rad;(void)cr;(void)cg;(void)cb;if(a>0)drawn(r);}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float px,float py,float rad,float cr,float cg,float cb,float a){(void)t;(void)f;(void)px;(void)py;(void)rad;(void)cr;(void)cg;(void)cb;if(a>0&&m!=GFX_SOMBRA)drawn(r);}
void gfx_vidro_painel(GfxRect r,float rad,float v,float a){(void)rad;(void)v;if(a>0)drawn(r);}
void gfx_vidro_foco(GfxRect r,float rad,float f,float a){(void)rad;(void)f;if(a>0)drawn(r);}
void gfx_vidro_cartao(GfxRect r,float rad,float f,float a){(void)rad;(void)f;if(a>0)drawn(r);}
void gfx_vidro_pilula_cheia(GfxRect r,float rad,float f,float a){(void)rad;(void)f;if(a>0)drawn(r);}
int gfx_vidro_tinta(float f){(void)f;return 24;}
void gfx_recorte(float x,float y,float w,float h){clip=(GfxRect){x*activeScale,y*activeScale,w*activeScale,h*activeScale};clipped=1;}
void gfx_sem_recorte(void){clipped=0;}
#ifndef EXPLORAR_REVIEW_POINTER_REAL
void ponteiro_rolagem(PonteiroRolagemFn f){assert(f==toqueExplorarRolar);}
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int i,int j){
  x*=activeScale;y*=activeScale;w*=activeScale;h*=activeScale;
  assert(nTargets<80);targets[nTargets++]=(PonteiroAlvo){x,y,w,h,f,a,i,j};
  if(x<0||y<0||x+w>NV_TELA_W+.01f||y+h>NV_TELA_H+.01f)problem("target outside",x,y,w,h);
}
#endif
static int glyphs(const char*s){int n=0;for(;*s;s++)if(((unsigned char)*s&0xc0)!=0x80)n++;return n;}
static int font(TxtEstilo e){return e==TXT_TITULO2?57:e==TXT_TITULO3?48:e==TXT_HEADLINE?38:e==TXT_BODY?25:e==TXT_CAPTION?22:21;}
int txt_largura(TxtEstilo e,const char*s){return (int)(glyphs(s)*font(e)*.55f*fontWide);}
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)r;(void)g;(void)b;(void)a;return(TxtLinha){.w=txt_largura(e,s),.h=(int)(font(e)*1.2f)};}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float w){
  if(w<=0){problem("nonpositive text width",0,0,w,0);return(TxtLinha){0};}
  TxtLinha l=txt_linha(e,s,r,g,b,a);if(l.w>w)l.w=(int)w;return l;
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a){if(a>0&&l.w>0)drawn((GfxRect){x,y,l.w,l.h});}
void txt_desenhar(TxtLinha l,float x,float y){txt_desenhar_alpha(l,x,y,1);}
float txt_tracking(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float a,float tracking){
  TxtLinha l=txt_linha(e,s,r,g,b,255);l.w+=(int)(glyphs(s)*tracking);txt_desenhar_alpha(l,x,y,a);return l.w;
}
float txt_bloco_corta(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float lead,float a,int max){
  TxtLinha l=txt_linha_corta(e,s,r,g,b,255,w);int n=w>0?(int)ceilf(txt_largura(e,s)/w):0;
  if(max>0&&n>max)n=max; l.h=n>0?(int)((n-1)*lead+font(e)*1.2f):0;
  txt_desenhar_alpha(l,x,y,a);return n*lead;
}
size_t txt_token_tam(const char*s){size_t n=0;while(s[n]&&s[n]!=' '&&s[n]!='\n')n++;return n;}
float txt_bloco(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float lead,float a,int max){
  if(telefoneui_ativo() && modo!=MODO_CLIMAS && a>0)assert(font(e)>=38);
  float atual=0;int linhas=0;const char*p=s;
  fullBlockGlyphs=0;
  while(*p){if(*p==' '){p++;continue;}if(*p=='\n'){if(atual>0){linhas++;atual=0;}p++;continue;}size_t n=txt_token_tam(p);char palavra[256];assert(n<sizeof palavra);
    memcpy(palavra,p,n);palavra[n]=0;float largura=txt_largura(e,palavra);
    fullBlockGlyphs+=glyphs(palavra);
    if(largura>w+.01f)problem("word wider than block",x,y,largura,w);
    float espaco=atual>0?txt_largura(e," "):0;
    if(atual>0&&atual+espaco+largura>w){linhas++;atual=0;espaco=0;}
    atual+=espaco+largura;p+=n;
  }
  if(atual>0)linhas++;assert(max<=0||linhas<=max);
  for(int l=0;l<linhas;l++)txt_desenhar_alpha((TxtLinha){.w=(int)fminf(w,atual>0?fmaxf(atual,w*.7f):0),.h=(int)(font(e)*1.2f)},x,y+l*lead,a);
  (void)r;(void)g;(void)b;return linhas*lead;
}
static void obra(MapaObra *o,int i){snprintf(o->titulo,sizeof o->titulo,"Uma historia com um titulo bastante comprido numero%d",i);snprintf(o->imdb,sizeof o->imdb,"tt%d",i);strcpy(o->tipo,"movie");o->ano=2024;o->nota=80;}
int main(void){
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  climas.n=MAPA_CLIMA_N; entrada=1;focoT=0;clFoco=0;
  for(int i=0;i<MAPA_CLIMA_N;i++){
    MapaClima*c=&climas.c[i];c->id=i;c->n=6;c->total=123;c->vistos=42;c->afinidade=65;
    for(int j=0;j<c->n;j++)obra(&c->itens[j],j);
  }
  for(int d=0;d<4;d++)for(int f=0;f<2;f++){
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];fontWide=f?1.25f:1;
    nTargets=0;clipped=0;phase=0;modo=MODO_CLIMAS;clRolar=0;toqueCl=(ToqueRolagem){0};desenharClimas();assert(nTargets>0&&nTargets<=MAPA_CLIMA_N);
    int vistos=0;for(int i=0;i<nTargets;i++)vistos|=1<<targets[i].a;
    clRolar=toqueCl.maximo;toqueCl.livre=1;nTargets=0;desenharClimas();
    for(int i=0;i<nTargets;i++)vistos|=1<<targets[i].a;
    clRolar=toqueCl.maximo*.5f;nTargets=0;desenharClimas();
    for(int i=0;i<nTargets;i++)vistos|=1<<targets[i].a;
    assert(vistos==(1<<MAPA_CLIMA_N)-1);
    for(int i=0;i<MAPA_CLIMA_N;i++){
      phase=i+1;nTargets=0;modo=MODO_CLIMA;clAberto=i;caLinha=caCol[0]=caCol[1]=0;
      caN[0]=caN[1]=3;for(int j=0;j<3;j++){caIdx[0][j]=j;caIdx[1][j]=j+3;}caRolar[0]=caRolar[1]=0;
      desenharClima();assert(nTargets>0);
      memset(&viz,0,sizeof viz);obra(&viz.foco,0);strcpy(viz.generos,"Drama · Misterio");
      for(int g=0;g<4;g++){
        viz.g[g].tipo=g==3?MAPA_GR_AMIGOS:g==2?MAPA_GR_REC:g==1?MAPA_GR_TEMA:MAPA_GR_PESSOA;
        strcpy(viz.g[g].sub,g==3?"Amigos com nomes compridos":"Uma pessoa ou tema com descricao longa");viz.g[g].n=5;
        for(int j=0;j<5;j++){obra(&viz.g[g].itens[j].obra,j);strcpy(viz.g[g].itens[j].motivo,"Uma pessoa ou tema");}
      }
      modo=MODO_VIZ;origem=ORIGEM_CLIMA;nTrilha=1;trilha[0].obra=viz.foco;vzLinha=vzCol=0;
      nTargets=0;phase=20+i;desenharViz();assert(nTargets>1);
    }
  }
  printf("explorar_desenho_review: %d draw operations, %d geometry violations\n",draws,failures);
  return failures?1:0;
}
