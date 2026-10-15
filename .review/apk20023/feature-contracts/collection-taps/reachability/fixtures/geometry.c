#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include VERTUDO_SOURCE
#include "perfis.h"
#include <assert.h>
#include <sys/stat.h>
static unsigned checks, writes, loads, allLoads;
static int globalView, profile=1, itemCount=60, sourceIndex, hideWatched;
static unsigned catRevision=1; static char watchedId[64];
static const ColFolder *lookup;
static CatItem items[60];
static Uint32 clockNow=1000;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"CHECK failed line%d: %s\n",__LINE__,#x);abort();}}while(0)
int ajustes_colecao_exibicao(void){return globalView;}
int salvos_perfil_atual(void){return profile;}
const ColFolder *col_por_catalogo(const char*b,const char*t,const char*i){(void)b;(void)t;(void)i;return lookup;}
const char*addons_base_por_id(const char*i){(void)i;return "";}
void col_nome_fonte(const ColSource*s,const char*m,char*d,unsigned n){(void)m;snprintf(d,n,"%s",s->title);}
const char*desc_nome_catalogo(const char*b,const char*t,const char*i){(void)b;(void)t;(void)i;return "Catalog";}
const char*desc_chave_tmdb(void){return "";}
const char*nuvem_trakt_cliente(void){return "";}
void desc_vertudo_fonte(const ColSource*s){sourceIndex=atoi(s->catId+6);loads++;}
void desc_vertudo_filtro(const char*b,const char*t,const char*i,const char*g){CHECK(b&&*b&&t&&i&&g);sourceIndex=atoi(i+6);loads++;}
void desc_vertudo_abrir(const char*b,const char*t,const char*i){CHECK(b&&*b&&t&&i);allLoads++;}
int desc_vertudo_n(void){return itemCount;}
int desc_vertudo_item(int i,CatItem*d){if(i<0||i>=itemCount)return 0;*d=items[i];snprintf(d->imdb,sizeof d->imdb,"tt%03d",sourceIndex*100+i+1);return 1;}
int desc_vertudo_carregando(void){return 0;}
int desc_vertudo_fim(void){return 1;}
int desc_vertudo_erro(void){return 0;}
void desc_vertudo_mais(void){}
int cat_visto(const CatItem*c){return watchedId[0]&&!strcmp(c->imdb,watchedId);}
void tex_orcamento_info(int*m,long*mem,int*f,int*s){*m=256;*mem=4096;*f=0;*s=100;}
int ptv_gpu_fraca_atual(void){return 0;}
float ajustes_conteudo_x(void){return 104;}
float ajustes_rail_largura_fixa(void){return 0;}
static float uiScale=1,drawScale=1;float gfx_escala_ui(void){return uiScale;}float gfx_escala(void){return drawScale;}void gfx_escala_sair(float x){drawScale=x;}
int ajustes_tinta_foco(void){return 20;}int ajustes_tinta_foco2(void){return 214;}
void col_cor(const ColFolder*f,float*r,float*g,float*b){(void)f;*r=.2f;*g=.3f;*b=.7f;}
void gfx_recorte(float x,float y,float w,float h){(void)x;(void)y;(void)w;(void)h;}void gfx_sem_recorte(void){}
void ajustes_acento(float*r,float*g,float*b){*r=.2f;*g=.3f;*b=.7f;}
const char*i18n(const char*s){return s;}
float gfx_opacidade_grupo=1;
void gfx_cor(GfxRect r,float a,float b,float c,float d,float e){(void)r;(void)a;(void)b;(void)c;(void)d;(void)e;}
static GfxRect logos[64];static int logoN;
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float x,float y,float ra,float a,float b,float c,float d){(void)t;(void)f;(void)x;(void)y;(void)ra;(void)a;(void)b;(void)c;(void)d;if(m==GFX_TEXTO||m==GFX_MARCA){CHECK(logoN<64);logos[logoN++]=r;}}
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)e;(void)r;(void)g;(void)b;(void)a;return(TxtLinha){.w=strlen(s)*12,.h=24};}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float w){TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=w;return t;}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a){(void)t;(void)x;(void)y;(void)a;}
Uint32 SDL_GetTicks(void){return clockNow;}
int SDL_GetNumTouchDevices(void){return 1;}
SDL_Window*SDL_GL_GetCurrentWindow(void){return NULL;}
SDL_Window*SDL_GetWindowFromID(Uint32 id){(void)id;return NULL;}
SDL_Window*SDL_GetMouseFocus(void){return NULL;}
Uint32 SDL_GetMouseState(int*x,int*y){if(x)*x=0;if(y)*y=0;return 0;}
void SDL_GetWindowSize(SDL_Window*w,int*x,int*y){(void)w;*x=1080;*y=2340;}
void SDL_GL_GetDrawableSize(SDL_Window*w,int*x,int*y){SDL_GetWindowSize(w,x,y);}
int SDL_ShowCursor(int q){(void)q;return 0;}
int collection_real_dados_gravar(const char*n,const char*t);
int dados_gravar(const char*n,const char*t){writes++;return collection_real_dados_gravar(n,t);}
static ColSource sources[COL_SOURCE_MAX];
static ColFolder folder,other;
static void setup(void){
 memset(&folder,0,sizeof folder);snprintf(folder.id,sizeof folder.id,"one");snprintf(folder.groupId,sizeof folder.groupId,"family");snprintf(folder.title,sizeof folder.title,"One");snprintf(folder.group,sizeof folder.group,"Awards");folder.sources=sources;folder.nSources=1;
 for(int i=0;i<COL_SOURCE_MAX;i++){snprintf(sources[i].base,sizeof sources[i].base,"https://offline.invalid");snprintf(sources[i].catId,sizeof sources[i].catId,"source%d",i);snprintf(sources[i].type,sizeof sources[i].type,"movie");snprintf(sources[i].title,sizeof sources[i].title,"Source%d",i);}
 other=folder;snprintf(other.id,sizeof other.id,"two");snprintf(other.title,sizeof other.title,"Two");
 for(int i=0;i<60;i++){snprintf(items[i].imdb,sizeof items[i].imdb,"tt%d",i+1);snprintf(items[i].tipo,sizeof items[i].tipo,"%s","movie");snprintf(items[i].meta,sizeof items[i].meta,"%d",1980+(i/12)*10);items[i].progresso=i==1?95:0;}
}
static int expected(int global,int own,int director){return own>0?own:global>0?global:director?VT_LAY_LINHA:VT_LAY_GRADE;}
static void readonlyOpen(void){unsigned before=writes;vertudo_colecao(&folder);CHECK(writes==before);CHECK(aberta);}

/* Typed headless rendering/catalog boundaries; pointer, collection event routing,
 * layout, local storage, ctxhold, abrirItem are actual product source. */
static CatItem added; static int addedN, renderN;
typedef struct { char id[64]; GfxRect r; } Drawn;
static Drawn rendered[256];
int ajustes_ocultar_filmes_vistos(void){return hideWatched;}
unsigned cat_revisao_itens(void){return catRevision;}
unsigned desc_vertudo_revisao(void){return 1;}
int cat_copiar_por_id(const char*i,const char*t,CatItem*d){(void)i;(void)t;(void)d;return 0;}
int desc_vertudo_n_bruto(void){return desc_vertudo_n();}
int desc_vertudo_item_bruto(int i,CatItem*d){return desc_vertudo_item(i,d);}
int cat_indice_por_imdb(const char *s){(void)s;return -1;}
int cat_acrescentar(const CatItem *it){added=*it;addedN++;return 200+addedN;}
int ajustes_borda_foco(void){return 1;}
int ajustes_vidro(void){return 0;}
float ajustes_raio_poster_px(void){return 12;}
int ajustes_foco_cresce(void){return 0;}
void home_vitrine_medida(float*w,float*h,float*p,float*r){*w=248;*h=372;*p=264;*r=40;}
int home_vitrine_arte_pronta(const CatItem *i){(void)i;return 1;}
int home_vitrine_arte(const CatItem *i,float a){(void)i;(void)a;return 1;}
void home_vitrine_texto(const CatItem*i,float a){(void)i;(void)a;}
void home_vitrine_cartao(const CatItem*i,GfxRect b,GfxRect c,float a){(void)c;CHECK(a>0);CHECK(renderN<256);snprintf(rendered[renderN].id,64,"%s",i->imdb);rendered[renderN++].r=b;}
static void fingerEvent(Uint32 type,float x,float y,SDL_FingerID id){SDL_Event e;SDL_zero(e);e.type=type;e.tfinger.touchId=1;e.tfinger.fingerId=id;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y/NV_LAYOUT_REAL_H;CHECK(ponteiro_evento(&e,vertudo_evento));}
static void actualTap(float x,float y){fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;fingerEvent(SDL_FINGERUP,x,y,1);clockNow+=400;}
static void drawActual(void){renderN=0;logoN=0;ponteiro_quadro(clockNow);ponteiro_camada();anim=1;ondaArmada=ondaEm=0;vertudo_desenhar(clockNow);ponteiro_desenhar();}
const char *col_banner(const ColFolder*f){(void)f;return "";}
int fundo_modo(void){return FUNDO_FROST;}
void fundo_desenhar_modo(int m,GfxRect a,float r,const char*u,float o){(void)m;(void)a;(void)r;(void)u;(void)o;}
const char *posterprov_card_addon(const char*a,const char*b,long c,const char*d,const char*e){(void)a;(void)b;(void)c;(void)d;return e;}
GLuint tex_obter_larg(const char*a,float b){(void)a;(void)b;return 0;}
float gfx_tex_aspect_atual;
void gfx_veu_base(GfxRect r,float a,float b,float c){(void)r;(void)a;(void)b;(void)c;}
float sinopse_bloco(TxtEstilo e,const char*t,int r,int g,int b,float x,float y,float w,float h,float a,int n){(void)e;(void)t;(void)r;(void)g;(void)b;(void)x;(void)y;(void)w;(void)h;(void)a;(void)n;return 0;}
static long tmdbRequested;
void desc_pedir_titulo_tmdb(long id,const char*tipo){CHECK(!strcmp(tipo,"movie"));tmdbRequested=id;}
GLuint tex_obter_logo_larg(const char*u,float w){(void)u;(void)w;return 1;}
float tex_aspecto(const char*u){(void)u;return 4;}
int tex_marca_escura(const char*u){(void)u;return 0;}
static void finger(int type,float x,float y){SDL_Event e;SDL_zero(e);e.type=type;e.tfinger.touchId=1;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y/NV_LAYOUT_REAL_H;CHECK(ponteiro_evento(&e,vertudo_evento));}
static void dragActual(float x,float y,float dx,float dy){finger(SDL_FINGERDOWN,x,y);clockNow+=20;finger(SDL_FINGERMOTION,x+dx,y+dy);clockNow+=140;finger(SDL_FINGERUP,x+dx,y+dy);clockNow+=300;vertudo_atualizar(.016f,clockNow);drawActual();}
static int titleVisible(int index){const PonteiroAlvo*t;int n=ponteiro_teste_lista(&t);for(int i=0;i<n;i++)if(t[i].focar==ponteiroCartaz&&t[i].a==index){float l=fmaxf(0,t[i].x),r=fminf(NV_LAYOUT_REAL_W,t[i].x+t[i].w);if(r-l>2&&t[i].y<NV_LAYOUT_REAL_H&&t[i].y+t[i].h>0)return 1;}return 0;}
static unsigned taps,drags,fullTargets;
static void checkBounds(void){const PonteiroAlvo*t;int n=ponteiro_teste_lista(&t);float e=vtEscala();for(int j=0;j<n;j++)if(t[j].focar==ponteiroCartaz){CHECK(t[j].x>=vtInicio()*e-.05f);CHECK(t[j].x+t[j].w<=vtFim()*e+.05f);if(t[j].h>=143.9f){CHECK(t[j].w>=143.9f);fullTargets++;}}for(int k=0;k<logoN;k++){CHECK(logos[k].w>0);CHECK(logos[k].x>=vtInicio()-.1f);CHECK(logos[k].x+logos[k].w<=vtFim()+.1f);}}
static int pointFor(int index,float*x,float*y){const PonteiroAlvo*t;int n=ponteiro_teste_lista(&t);for(int j=0;j<n;j++)if(t[j].focar==ponteiroCartaz&&t[j].a==index){float e=vtEscala();float lo=fmaxf(t[j].y,(VT_TOPO+celY[index]-scrollY)*e),hi=fminf(t[j].y+t[j].h,(VT_TOPO+celY[index]-scrollY+celH[index])*e);if(hi-lo<fminf(144,celH[index]*e)-.1f)continue;float l=fmaxf(0,t[j].x),rr=fminf(NV_LAYOUT_REAL_W,t[j].x+t[j].w);if(rr-l<144-.1f)continue;for(int yy=1;yy<5;yy++)for(int xx=1;xx<5;xx++){float px=l+(rr-l)*xx/5,py=lo+(hi-lo)*yy/5;if(ponteiro_achar(t,n,px,py)==j){*x=px;*y=py;return 1;}}}return 0;}
int main(int argc,char**argv){
 int view=argc>1?atoi(argv[1]):3,zoom=argc>2?atoi(argv[2]):2,orientation=argc>3?atoi(argv[3]):0,tv=argc>4?atoi(argv[4]):0;
 char tmp[]="/tmp/nuvio362-geometry-XXXXXX";CHECK(mkdtemp(tmp));CHECK(!setenv("NUVIO_DADOS",tmp,1));dados_iniciar(tmp);setup();
 snprintf(items[0].logo,sizeof items[0].logo,"logo0");snprintf(items[7].logo,sizeof items[7].logo,"logo7");
 layout_modo_definir(!tv);layout_tela_definir(orientation?2340:1080,orientation?1080:2340);uiScale=drawScale=(float[]){1,1.3f,1.5f}[zoom];
 folder.nSources=1;globalView=view;readonlyOpen();CHECK(lay==view||(!view&&lay==VT_LAY_GRADE));
 ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);drawActual();
 if(tv){for(int i=0;i<celN;i++){CatItem it;CHECK(viewItem(i,&it));printf("CELL %d %d %.6f %.6f %.6f %.6f %d %d %s\n",view,i,celX[i],celY[i],celW[i],celH[i],celLin[i],celCol[i],it.imdb);}return 0;}
 checkBounds();
 float x=(vtInicio()+100)*vtEscala(),y=(VT_TOPO+celY[0]+80)*vtEscala();
 dragActual(x,y,-fminf(250,x-8),0);drags++;CHECK(vertudo_pediu_abrir()==-1);checkBounds();
 for(int chosen=0;chosen<itemCount;chosen++){
  float px,py;int tries=0;
  while(!pointFor(chosen,&px,&py)&&tries++<60){float center=(VT_TOPO+celY[chosen]+celH[chosen]*.5f-scrollY)*vtEscala();float delta=center<NV_LAYOUT_REAL_H*.5f?500:-500;float before=scrollY;dragActual(NV_LAYOUT_REAL_W*.5f,NV_LAYOUT_REAL_H*.6f,0,delta);drags++;CHECK(vertudo_pediu_abrir()==-1);CHECK(fabsf(scrollY-before)>1);checkBounds();}
  CHECK(pointFor(chosen,&px,&py));CatItem expectedItem;CHECK(viewItem(chosen,&expectedItem));actualTap(px,py);CHECK(vertudo_pediu_abrir()>=0);CHECK(!strcmp(added.imdb,expectedItem.imdb));taps++;drawActual();checkBounds();
 }
 CHECK(taps==(unsigned)itemCount);CHECK(addedN==itemCount);CHECK(drags>1);CHECK(fullTargets>0);
 // Orientation and UI-size changes reflow the same open collection without replacing its items.
 float priorArea=celAreaW;layout_tela_definir(orientation?1080:2340,orientation?2340:1080);drawActual();checkBounds();CHECK(celAreaW!=priorArea);CHECK(celAreaW==vtFim()-vtInicio()&&celTelaH==NV_TELA_H);layout_tela_definir(orientation?2340:1080,orientation?1080:2340);uiScale=(zoom==2?1.f:1.5f);drawActual();checkBounds();CHECK(celAreaW==vtFim()-vtInicio()&&celTelaH==NV_TELA_H);for(int i=0;i<celN;i++){GfxRect r=lay==VT_LAY_LINHA?vtTimelineCard(0):(GfxRect){vtInicio()+celX[i],0,celW[i],celH[i]};CHECK(r.x>=vtInicio()-.05f);CHECK(r.x+r.w<=vtFim()+.05f);}
 printf("PASS view=%d orientation=%d zoom=%d titles=%u real_drags=%u usable48dp_targets=%u checks=%u\n",view,orientation,zoom,taps,drags,fullTargets,checks);return 0;
}
