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
static CatItem items[30];
static int catalogLoading;
static int lateAddonReady[3],missingRowMessages;
static Uint32 clockNow=1000;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"CHECK failed line%d: %s\n",__LINE__,#x);abort();}}while(0)
int ajustes_colecao_exibicao(void){return globalView;}
int salvos_perfil_atual(void){return profile;}
const ColFolder *col_por_catalogo(const char*b,const char*t,const char*i){(void)b;(void)t;(void)i;return lookup;}
const char*addons_base_por_id(const char*i){if(i&&!strncmp(i,"late",4)){int n=atoi(i+4);if(n>=0&&n<3&&lateAddonReady[n])return "https://offline.invalid";}return "";}
void col_nome_fonte(const ColSource*s,const char*m,char*d,unsigned n){(void)m;snprintf(d,n,"%s",s->title);}
const char*desc_nome_catalogo(const char*b,const char*t,const char*i){(void)b;(void)t;(void)i;return "Catalog";}
const char*desc_chave_tmdb(void){return "";}
const char*nuvem_trakt_cliente(void){return "";}
void desc_vertudo_fonte(const ColSource*s){sourceIndex=atoi(s->catId+6);loads++;}
void desc_vertudo_filtro(const char*b,const char*t,const char*i,const char*g){CHECK(b&&*b&&t&&i&&g);sourceIndex=atoi(i+6);loads++;}
void desc_vertudo_abrir(const char*b,const char*t,const char*i){CHECK(b&&*b&&t&&i);allLoads++;}
int desc_vertudo_n(void){return itemCount;}
int desc_vertudo_item(int i,CatItem*d){if(i<0||i>=itemCount)return 0;*d=items[i];snprintf(d->imdb,sizeof d->imdb,"tt%03d",sourceIndex*100+i+1);return 1;}
int desc_vertudo_carregando(void){return catalogLoading;}
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
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)e;(void)r;(void)g;(void)b;(void)a;if(strstr(s,"O addon desta coleção não está instalado"))missingRowMessages++;return(TxtLinha){.w=strlen(s)*12,.h=24};}
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

static void tick(void){clockNow+=16;vertudo_atualizar(1.0f/60.0f,clockNow);}
static void key(SDL_Keycode k){SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=k;vertudo_evento(&e);}
static void visible(const char *id,float*x,float*y){
 int found=-1;for(int i=0;i<renderN;i++)if(!strcmp(rendered[i].id,id)){GfxRect r=rendered[i].r;float left=fmaxf(r.x,vtInicio()),right=fminf(r.x+r.w,NV_TELA_W),top=fmaxf(r.y,NV_SHELF_TOP-8),bottom=fminf(r.y+r.h,NV_TELA_H);if(right>left&&bottom>top){*x=(left+right)*.5f*vtEscala();*y=(top+bottom)*.5f*vtEscala();found=i;break;}}
 CHECK(found>=0);const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);CHECK(ponteiro_achar(v,n,*x,*y)>=0);
}
static void swipe(float x,float y,int eixoY){
 fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;fingerEvent(SDL_FINGERMOTION,x+(eixoY?-10:-180),y+(eixoY?-180:-10),1);clockNow+=60;fingerEvent(SDL_FINGERUP,x+(eixoY?-10:-180),y+(eixoY?-180:-10),1);
 CHECK(addedN==0&&vertudo_pediu_abrir()<0);actualTap(x,y);CHECK(addedN==0&&vertudo_pediu_abrir()<0);drawActual();
}
int main(int argc,char**argv){
 const char*test=argc>5?argv[5]:"late-y";int tv=!strcmp(test,"tv-follow"),reorder=strstr(test,"reorder")!=NULL||strstr(test,"regroup")!=NULL;
 char tmp[]="/tmp/nuvio362-async-XXXXXX";CHECK(mkdtemp(tmp));CHECK(!setenv("NUVIO_DADOS",tmp,1));dados_iniciar(tmp);setup();
 layout_modo_definir(!tv);layout_tela_definir(tv?1920:1080,tv?1080:2340);uiScale=drawScale=1;

 if(!strcmp(test,"late-addon-recovery")||!strcmp(test,"late-addon-inflight")){
  folder.nSources=3;globalView=VT_LAY_VITRINE;gravarEscolha(&folder,VT_LAY_VITRINE);
  for(int r=0;r<3;r+=2){sources[r].base[0]=0;snprintf(sources[r].addonId,sizeof sources[r].addonId,"late%d",r);}
  readonlyOpen();for(int i=0;i<12;i++)tick();CHECK(vitCarga==3&&vitLin[0].estado==VIT_VAZIA&&vitLin[0].semFonte&&vitLin[1].n==6&&vitLin[2].estado==VIT_VAZIA&&vitLin[2].semFonte);CHECK(loads==1);
  lateAddonReady[0]=1;clockNow+=1600;
  if(!strcmp(test,"late-addon-inflight")){
   tick();CHECK(vitCarga==0&&vitLin[0].estado==VIT_PEND);tick();CHECK(vitLin[0].estado==VIT_CARREGANDO&&loads==2);catalogLoading=1;lateAddonReady[2]=1;
   for(int i=0;i<180;i++){tick();CHECK(vitCarga==0&&vitLin[0].estado==VIT_CARREGANDO&&loads==2&&vitLin[2].n==0);}
   catalogLoading=0;
  }
  for(int i=0;i<8;i++)tick();CHECK(vitLin[0].n==6&&vitLin[1].n==6);CHECK(loads>=2);
  printf("after addon0: carga=%d row0=%d/%d row1=%d/%d row2=%d/%d loads=%u\n",vitCarga,vitLin[0].estado,vitLin[0].n,vitLin[1].estado,vitLin[1].n,vitLin[2].estado,vitLin[2].n,loads);fflush(stdout);
  lateAddonReady[2]=1;clockNow+=1600;for(int i=0;i<180;i++)tick();
  ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);drawActual();
  printf("after addon2: carga=%d n=%d row0=%d/%d row1=%d/%d row2=%d/%d semFonte2=%d readyBase2=%d loads=%u drawn=%d missingRowMessages=%d\n",vitCarga,vitN,vitLin[0].estado,vitLin[0].n,vitLin[1].estado,vitLin[1].n,vitLin[2].estado,vitLin[2].n,vitLin[2].semFonte,baseDaFonte(&sources[2])[0]!=0,loads,renderN,missingRowMessages);fflush(stdout);
  CHECK(vitLin[2].n==6);CHECK(vitCarga==vitN);CHECK(loads==3);CHECK(missingRowMessages==0);puts("PASS late addon0 and2 both recover across already-ready row1");return 0;
 }
 folder.nSources=reorder?1:8;globalView=VT_LAY_VITRINE;gravarEscolha(&folder,VT_LAY_VITRINE);
 if(reorder){itemCount=24;for(int i=0;i<24;i++){items[i]=items[0];snprintf(items[i].tipo,sizeof items[i].tipo,"movie");snprintf(items[i].meta,sizeof items[i].meta,"%d",1990+(i/6)*10);}}
 readonlyOpen();for(int i=0;i<(reorder?2:4);i++)tick();CHECK(lay==VT_LAY_VITRINE);CHECK(vitLin[0].n>0&&vitLin[1].n>0);
 if(!reorder){CHECK(vitCarga==2);catalogLoading=1;tick();CHECK(vitLin[2].estado==VIT_CARREGANDO);}
 ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);drawActual();
 if(tv){
  key(SDLK_DOWN);for(int i=0;i<3;i++)key(SDLK_RIGHT);CHECK(vitFR==1&&vitFC==3);for(int i=0;i<90;i++){tick();drawActual();}float before=vitScroll;CHECK(fabsf(before-vitPasso())<.1f);catalogLoading=0;
  for(int i=0;i<120;i++){int cargaAntes=vitCarga;tick();drawActual();CHECK(vitFR==1&&vitFC==3);CHECK(vitScroll>before-.1f);if(vitCarga>cargaAntes){float v=0,want=anim_mola2(&v,0,528,1.0f/60.0f,NV_MOLA2_SCROLL);CHECK(fabsf(vitSx[1]-want)<.01f);}}CHECK(vitCarga==vitN);CHECK(fabsf(vitSx[1]-528)<.1f);key(SDLK_RETURN);CHECK(vertudo_pediu_abrir()>=0&&addedN==1&&!strcmp(added.imdb,"tt104"));puts("PASS TV focus scroll and title survive async rows");return 0;
 }
 float x,y;visible(reorder?"tt020":"tt002",&x,&y);
 if(!strncmp(test,"stale-",6)){
  fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;
  if(reorder){items[24]=items[0];snprintf(items[24].meta,sizeof items[24].meta,"2030");itemCount=25;}else catalogLoading=0;
  tick();if(strstr(test,"after-redraw"))drawActual();fingerEvent(SDL_FINGERUP,x,y,1);CHECK(addedN==0&&vertudo_pediu_abrir()<0&&tmdbRequested==0);printf("PASS %s rejects old rendered title\n",test);return 0;
 }


 if(!strcmp(test,"regroup-active-x")){
  swipe(x,y,1);visible("tt020",&x,&y);float oldY=vitScroll;fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;fingerEvent(SDL_FINGERMOTION,x-80,y-5,1);float first=vitSx[0];CHECK(first>70);
  items[24]=items[0];snprintf(items[24].meta,sizeof items[24].meta,"2030");itemCount=25;tick();drawActual();clockNow+=60;fingerEvent(SDL_FINGERMOTION,x-160,y-10,1);printf("active regroup oldX=%g row0X=%g row1X=%g oldY=%g newY=%g\n",first,vitSx[0],vitSx[1],oldY,vitScroll);fflush(stdout);
  CHECK(vitN==5&&vitSx[0]==0);CHECK(vitSx[1]>first+70);CHECK(fabsf(vitScroll-(oldY+vitPasso()))<.01f);fingerEvent(SDL_FINGERUP,x-160,y-10,1);CHECK(addedN==0&&vertudo_pediu_abrir()<0);puts("PASS active horizontal gesture follows regrouped source row");return 0;
 }
 if(!strcmp(test,"reset-active")){
  fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;fingerEvent(SDL_FINGERMOTION,x-5,y-80,1);CHECK(vitScroll>70);readonlyOpen();tick();drawActual();clockNow+=60;fingerEvent(SDL_FINGERMOTION,x-10,y-160,1);fingerEvent(SDL_FINGERUP,x-10,y-160,1);CHECK(vitScroll==0&&vitSx[0]==0);CHECK(addedN==0&&vertudo_pediu_abrir()<0);puts("PASS reopening collection cancels old active gesture");return 0;
 }
 if(!strncmp(test,"active-",7)){
  int eixoY=!strcmp(test,"active-y");fingerEvent(SDL_FINGERDOWN,x,y,1);clockNow+=60;fingerEvent(SDL_FINGERMOTION,x+(eixoY?-5:-80),y+(eixoY?-80:-5),1);float first=eixoY?vitScroll:vitSx[0];CHECK(first>70);
  catalogLoading=0;tick();drawActual();clockNow+=60;fingerEvent(SDL_FINGERMOTION,x+(eixoY?-10:-160),y+(eixoY?-160:-10),1);float second=eixoY?vitScroll:vitSx[0];printf("%s active delta before=%g after=%g loaded=%d\n",test,first,second,vitCarga);fflush(stdout);CHECK(second>first+70);fingerEvent(SDL_FINGERUP,x+(eixoY?-10:-160),y+(eixoY?-160:-10),1);CHECK(addedN==0&&vertudo_pediu_abrir()<0);printf("PASS %s continues across completed row\n",test);return 0;
 }
 if(!strcmp(test,"regroup-xy")){
  swipe(x,y,0);visible("tt020",&x,&y);swipe(x,y,1);visible("tt020",&x,&y);float anchorX=x,anchorY=y,beforeY=vitScroll,beforeX=vitSx[0];CHECK(beforeY>100&&beforeX>100);
  items[24]=items[0];snprintf(items[24].meta,sizeof items[24].meta,"2030");itemCount=25;tick();drawActual();visible("tt020",&x,&y);printf("regroup beforeY=%g afterY=%g oldX=%g row0X=%g row1X=%g anchorBefore=%g,%g anchorAfter=%g,%g\n",beforeY,vitScroll,beforeX,vitSx[0],vitSx[1],anchorX,anchorY,x,y);fflush(stdout);
  CHECK(vitN==5);CHECK(fabsf(vitScroll-(beforeY+vitPasso()))<.01f);CHECK(fabsf(vitSx[1]-beforeX)<.01f);CHECK(vitSx[0]==0);CHECK(fabsf(x-anchorX)<.01f&&fabsf(y-anchorY)<.01f);
  float keptY=vitScroll;for(int i=0;i<120;i++){tick();drawActual();CHECK(fabsf(vitScroll-keptY)<.01f);CHECK(fabsf(vitSx[1]-beforeX)<.01f);}puts("PASS regroup preserves source group viewport and X offset");return 0;
 }
 int wantX=!strcmp(test,"late-x")||!strcmp(test,"late-xy"),wantY=!strcmp(test,"late-y")||!strcmp(test,"late-xy");
 if(wantX){swipe(x,y,0);CHECK(vitSx[0]>100);visible("tt002",&x,&y);}
 if(wantY){swipe(x,y,1);CHECK(vitScroll>100);}
 float beforeY=vitScroll,beforeX=vitSx[0];printf("%s before late row: Y=%g X0=%g focus=%d,%d loaded=%d\n",test,beforeY,beforeX,vitFR,vitFC,vitCarga);fflush(stdout);catalogLoading=0;
 for(int i=0;i<120;i++){tick();drawActual();if(i==0||i==119){printf("frame%d: Y=%g X0=%g focus=%d,%d loaded=%d\n",i,vitScroll,vitSx[0],vitFR,vitFC,vitCarga);fflush(stdout);}CHECK(fabsf(vitScroll-beforeY)<.01f);CHECK(fabsf(vitSx[0]-beforeX)<.01f);CHECK(addedN==0&&vertudo_pediu_abrir()<0);}
 CHECK(vitCarga==vitN);printf("PASS %s keeps settled Showcase offsets through all late rows\n",test);return 0;
}

/* New pagination API, same fixed offline first-page catalog boundary. */
void desc_vertudo_filtro_desde(const char *b,const char *t,const char *i,const char *g,int pagina) { CHECK(pagina == 0); desc_vertudo_filtro(b,t,i,g); }
int desc_vertudo_pagina(void) { return 0; }
