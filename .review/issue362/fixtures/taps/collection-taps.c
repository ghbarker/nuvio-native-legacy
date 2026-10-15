#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include VERTUDO_SOURCE
#include "perfis.h"
#include <assert.h>
#include <sys/stat.h>
static unsigned checks, writes, loads, allLoads;
static int globalView, profile=1, itemCount=6, sourceIndex, hideWatched;
static unsigned catRevision=1; static char watchedId[64];
static const ColFolder *lookup;
static CatItem items[6];
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
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float x,float y,float ra,float a,float b,float c,float d){(void)r;(void)t;(void)m;(void)f;(void)x;(void)y;(void)ra;(void)a;(void)b;(void)c;(void)d;}
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
 for(int i=0;i<6;i++){snprintf(items[i].imdb,sizeof items[i].imdb,"tt%d",i+1);snprintf(items[i].tipo,sizeof items[i].tipo,"%s",i%2?"series":"movie");snprintf(items[i].meta,sizeof items[i].meta,"%s",i==0?"":i%2?"2001":"1990");items[i].progresso=i==1?95:0;}
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
static void drawActual(void){renderN=0;ponteiro_quadro(clockNow);ponteiro_camada();anim=1;ondaArmada=ondaEm=0;vertudo_desenhar(clockNow);ponteiro_desenhar();}
const char *col_banner(const ColFolder*f){(void)f;return "";}
int fundo_modo(void){return FUNDO_FROST;}
void fundo_desenhar_modo(int m,GfxRect a,float r,const char*u,float o){(void)m;(void)a;(void)r;(void)u;(void)o;}
const char *posterprov_card_addon(const char*a,const char*b,long c,const char*d,const char*e){(void)a;(void)b;(void)c;(void)d;return e;}
GLuint tex_obter_larg(const char*a,float b){(void)a;(void)b;return 0;}
float gfx_tex_aspect_atual;
void gfx_veu_base(GfxRect r,float a,float b,float c){(void)r;(void)a;(void)b;(void)c;}
float sinopse_bloco(TxtEstilo e,const char*t,int r,int g,int b,float x,float y,float w,float h,float a,int n){(void)e;(void)t;(void)r;(void)g;(void)b;(void)x;(void)y;(void)w;(void)h;(void)a;(void)n;return 0;}
float txt_bloco(TxtEstilo e,const char*t,int r,int g,int b,float x,float y,float w,float h,float a,int n){return sinopse_bloco(e,t,r,g,b,x,y,w,h,a,n);}
static long tmdbRequested; static char tmdbType[8];
void desc_pedir_titulo_tmdb(long id,const char*tipo){CHECK(!strcmp(tipo,"movie")||!strcmp(tipo,"tv"));snprintf(tmdbType,8,"%s",tipo);tmdbRequested=id;}
int main(int argc,char**argv){
 int view=argc>1?atoi(argv[1]):7,orientation=argc>2?atoi(argv[2]):0,zoom=argc>3?atoi(argv[3]):0,row=argc>4?atoi(argv[4]):0;
 const char *test=argc>5?argv[5]:"open";
 char tmp[]="/tmp/nuvio362-taps-XXXXXX";CHECK(mkdtemp(tmp));CHECK(!setenv("NUVIO_DADOS",tmp,1));dados_iniciar(tmp);setup();
 layout_modo_definir(1);layout_tela_definir(orientation?2340:1080,orientation?1080:2340);uiScale=drawScale=(float[]){1,1.3f,1.5f}[zoom];
 folder.nSources=strstr(test,"drag")||strstr(test,"resume")||strstr(test,"transition")?4:2;globalView=view;gravarEscolha(&folder,view);readonlyOpen();
 if(lay==VT_LAY_VITRINE){for(int i=0;i<folder.nSources*2+2;i++)vitCarregar(clockNow+=100);
#ifdef HAS_VIT_PROJECTION
vitProjetar();
#endif
CHECK(vitN==folder.nSources&&vitLin[0].n==6&&vitLin[1].n==6);vitHeroAtual=0;if(row){vitScroll=vitPasso();vitFR=1;}if(!strcmp(test,"clip-top"))vitScroll=100;if(!strcmp(test,"tmdb"))snprintf(vitItem(0,1)->imdb,64,"tmdb:321");}
 ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);
 if(!strcmp(test,"decades-clip"))scrollY=200;
 drawActual();const PonteiroAlvo*targets;int n=ponteiro_teste_lista(&targets);char expectedId[64];float x,y;int expectRow=0,expectCol=0;
 if(!strcmp(test,"decades-clip")){
  CHECK(lay==VT_LAY_DECADAS&&decN>0);int cell=-1;float top=VT_TOPO+VT_DEC_H-24;
  for(int i=0;i<celN;i++){float cy=VT_TOPO+celY[i]-scrollY;if(cy<top&&cy+celH[i]>top){cell=i;break;}}
  CHECK(cell>=0);x=(vtInicio()+celX[cell]+celW[cell]/2)*vtEscala();y=(VT_TOPO+36)*vtEscala();int hit=ponteiro_achar(targets,n,x,y);printf("decades hidden header x=%g y=%g hit=%d\n",x,y,hit);fflush(stdout);actualTap(x,y);CHECK(vertudo_pediu_abrir()<0&&addedN==0);puts("PASS hidden Decades card does not activate");return 0;
 }
 if(lay==VT_LAY_VITRINE){
  CHECK(renderN>1);int chosen=-1;float e=vtEscala();
  for(int i=0;i<renderN;i++){GfxRect r=rendered[i].r;float cx=(fmaxf(r.x,vtInicio())+fminf(r.x+r.w,NV_TELA_W))*.5f*e,cy=(fmaxf(r.y,NV_SHELF_TOP-8)+fminf(r.y+r.h,NV_TELA_H))*.5f*e;if(cx>=0&&cx<NV_LAYOUT_REAL_W&&cy>=(NV_SHELF_TOP-8)*e&&cy<NV_LAYOUT_REAL_H&&strstr(rendered[i].id,!strcmp(test,"tmdb")?"tmdb:321":row?"tt102":"tt002")){chosen=i;break;}}
  CHECK(chosen>=0);Drawn target=rendered[chosen];snprintf(expectedId,64,"%s",target.id);x=(fmaxf(target.r.x,vtInicio())+fminf(target.r.x+target.r.w,NV_TELA_W))*.5f*e;y=(fmaxf(target.r.y,NV_SHELF_TOP-8)+fminf(target.r.y+target.r.h,NV_TELA_H))*.5f*e;expectRow=row;expectCol=1;
 }else{
  int chosen=-1;for(int i=0;i<n;i++)if(targets[i].focar==ponteiroCartaz&&targets[i].a>0){float cy=targets[i].y+targets[i].h*.5f;if(cy<NV_LAYOUT_REAL_H){chosen=i;break;}}
  CHECK(chosen>=0);PonteiroAlvo target=targets[chosen];CatItem it;CHECK(viewItem(target.a,&it));snprintf(expectedId,64,"%s",it.imdb);x=target.x+target.w/2;y=target.y+target.h/2;
 }
 int hit=ponteiro_achar(targets,n,x,y);printf("view=%d orientation=%d zoom=%d row=%d rendered=%d targets=%d title=%s at=%.1f,%.1f hit=%d\n",view,orientation,zoom,row,renderN,n,expectedId,x,y,hit);fflush(stdout);

 if(!strncmp(test,"stale-",6)){
  CHECK(lay==VT_LAY_VITRINE);fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;
  if(!strcmp(test,"stale-filter")){sourceIndex=row;CatItem old=items[0];items[0]=items[1];items[1]=old;vitGuardar(row);}
  else if(!strcmp(test,"stale-reopen"))readonlyOpen();
  else if(!strcmp(test,"stale-source")){source=1;openSource();}
  else if(!strcmp(test,"stale-session")){profile=2;perfis_invalidar_sessao();}
  else if(!strcmp(test,"stale-mode"))layout_modo_definir(0);
  else if(!strcmp(test,"stale-closed"))vertudo_fechar_seco();
  else if(!strcmp(test,"stale-layout")){globalView=VT_LAY_GRADE;alternarExibicao();}
  else if(!strcmp(test,"stale-key"))snprintf(vitItem(row,1)->imdb,64,"tt999");
  else CHECK(0);
  fingerEvent(SDL_FINGERUP,x,y,1);CHECK(vertudo_pediu_abrir()<0&&addedN==0&&tmdbRequested==0);printf("PASS %s consumes stale title activation\n",test);return 0;
 }
 if(strstr(test,"drag")||strstr(test,"resume")||strstr(test,"transition")){
  CHECK(lay==VT_LAY_VITRINE);int axisY=strstr(test,"-y")!=NULL;float old=axisY?vitScroll:vitSx[row];float oldGrid=scrollY;tabFocus=1;tabCursor=nAbas();
  fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;float dx=axisY?-20:-180,dy=axisY?-180:-20;fingerEvent(SDL_FINGERMOTION,x+dx,y+dy,1);clockNow+=60;fingerEvent(SDL_FINGERUP,x+dx,y+dy,1);
  CHECK(vertudo_pediu_abrir()<0&&addedN==0);float changed=axisY?vitScroll:vitSx[row];printf("gesture %s before=%g after=%g grid=%g tabFocus=%d\n",test,old,changed,scrollY,tabFocus);fflush(stdout);CHECK(changed>old);CHECK(scrollY==oldGrid);CHECK(tabFocus==0);
  for(int i=0;i<6;i++){clockNow+=16;vitAtualizar(1.0f/60.0f,clockNow);}CHECK(fabsf((axisY?vitScroll:vitSx[row])-changed)<.01f);
  if(strstr(test,"transition")){
   layout_modo_definir(0);clockNow+=16;vitAtualizar(1.0f/60.0f,clockNow);float tv=axisY?vitScroll:vitSx[row];CHECK(tv<changed);layout_modo_definir(1);clockNow+=16;vitAtualizar(1.0f/60.0f,clockNow);CHECK((axisY?vitScroll:vitSx[row])<tv);printf("PASS %s restores TV and Mobile focus scrolling\n",test);return 0;
  }
  if(strstr(test,"resume")){SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RIGHT;vertudo_evento(&e);CHECK(!tabFocus);CHECK(vitFC>0);}
  printf("PASS %s changes Showcase offset, survives update, and never opens\n",test);return 0;
 }
 if(!strcmp(test,"clip-top")){
  float hiddenY=(NV_SHELF_TOP-12)*vtEscala();CHECK(ponteiro_achar(targets,n,x,hiddenY)<0);actualTap(x,hiddenY);CHECK(vertudo_pediu_abrir()<0&&addedN==0);
  for(int i=0;i<n;i++)if(targets[i].focar)CHECK(targets[i].y>=(NV_SHELF_TOP-8)*vtEscala());
 }
 if(!strcmp(test,"multi")){
  fingerEvent(SDL_FINGERDOWN,x,y,1);fingerEvent(SDL_FINGERDOWN,x,y,2);clockNow+=60;fingerEvent(SDL_FINGERUP,x,y,2);fingerEvent(SDL_FINGERUP,x,y,1);CHECK(vertudo_pediu_abrir()<0&&addedN==0);puts("PASS multi-finger does not activate");return 0;
 }
 if(!strcmp(test,"hold")){fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=NV_HOLD_MS+50;ponteiro_quadro(clockNow);CHECK(vertudo_pediu_abrir()<0&&addedN==0);fingerEvent(SDL_FINGERUP,x,y,1);}else actualTap(x,y);
 int requested=vertudo_pediu_abrir();printf("after=%d,%d request=%d added=%d id=%s tmdb=%ld\n",vitFR,vitFC,requested,addedN,added.imdb,tmdbRequested);fflush(stdout);
 if(!strcmp(test,"tmdb")){CHECK(requested<0&&addedN==0&&tmdbRequested==321&&!strcmp(tmdbType,"tv"));puts("PASS actual abrirItem TMDB request path");return 0;}
 CHECK(requested>=0);CHECK(addedN==1);CHECK(!strcmp(added.imdb,expectedId));if(lay==VT_LAY_VITRINE)CHECK(vitFR==expectRow&&vitFC==expectCol);
 printf("PASS actual full collection render+pointer title open: %u assertions\n",checks);return 0;
}
