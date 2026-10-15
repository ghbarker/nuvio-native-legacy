#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include VERTUDO_SOURCE
#include <assert.h>
#include <sys/stat.h>
static unsigned checks, writes, loads, allLoads;
static int globalView, profile=1, itemCount=6;
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
void desc_vertudo_fonte(const ColSource*s){(void)s;loads++;}
void desc_vertudo_filtro(const char*b,const char*t,const char*i,const char*g){CHECK(b&&*b&&t&&i&&g);loads++;}
void desc_vertudo_abrir(const char*b,const char*t,const char*i){CHECK(b&&*b&&t&&i);allLoads++;}
int desc_vertudo_n(void){return itemCount;}
int desc_vertudo_item(int i,CatItem*d){if(i<0||i>=itemCount)return 0;*d=items[i];return 1;}
int desc_vertudo_carregando(void){return 0;}
int desc_vertudo_fim(void){return 1;}
int desc_vertudo_erro(void){return 0;}
void desc_vertudo_mais(void){}
int cat_visto(const CatItem*c){(void)c;return 0;}
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
static void fallbackUnexpected(const SDL_Event*e){fprintf(stderr,"Unexpected touch fallback event %u\n",e->type);abort();}
static void fingerTap(float x,float y){SDL_Event e;SDL_zero(e);e.type=SDL_FINGERDOWN;e.tfinger.touchId=1;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y/NV_LAYOUT_REAL_H;CHECK(ponteiro_evento(&e,fallbackUnexpected));clockNow+=60;e.type=SDL_FINGERUP;CHECK(ponteiro_evento(&e,fallbackUnexpected));clockNow+=400;}
static void enterView(void){tabFocus=1;tabCursor=nAbas();SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RETURN;vertudo_evento(&e);}
int main(int argc,char**argv){
 char tmp[]="/tmp/nuvio362-views-XXXXXX";CHECK(mkdtemp(tmp));CHECK(!setenv("NUVIO_DADOS",tmp,1));dados_iniciar(tmp);setup();
 layout_modo_definir(1);layout_tela_definir(1080,2340);
 if(argc>1&&!strcmp(argv[1],"header-focus")){globalView=VT_LAY_MURAL;folder.nSources=COL_SOURCE_MAX;readonlyOpen();tabFocus=1;tabCursor=nAbas();ponteiro_iniciar();ponteiro_quadro(clockNow);ponteiro_camada();themeHeader(1,vtInicio());ponteiro_desenhar();puts("PASS actual header focus boundary");return 0;}
 if(argc>1&&!strcmp(argv[1],"header-overlap")){globalView=VT_LAY_MURAL;folder.nSources=COL_SOURCE_MAX;readonlyOpen();tabFocus=0;source=0;ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_quadro(clockNow);ponteiro_camada();themeHeader(1,vtInicio());botaoExibicao(1,vtInicio());ponteiro_desenhar();const PonteiroAlvo*t;int n=ponteiro_teste_lista(&t),view=-1;for(int i=0;i<n;i++)if(t[i].ativar==toqueExibicao)view=i;CHECK(view>=0);for(int i=0;i<n;i++)if(i!=view){float w=fminf(t[i].x+t[i].w,t[view].x+t[view].w)-fmaxf(t[i].x,t[view].x),h=fminf(t[i].y+t[i].h,t[view].y+t[view].h)-fmaxf(t[i].y,t[view].y);if(w>0&&h>0)fprintf(stderr,"OVERLAP view vs tab%d: %g x %g\n",i,w,h);CHECK(w<=0||h<=0);}puts("PASS actual header target separation");return 0;}
 for(int director=0;director<2;director++)for(int ns=1;ns<=COL_SOURCE_MAX;ns=ns==1?2:ns==2?COL_SOURCE_MAX:COL_SOURCE_MAX+1){
  folder.nSources=ns;snprintf(folder.group,sizeof folder.group,"%s",director?"Directors":"Awards");
  for(int global=0;global<VT_LAY_N;global++)for(int own=0;own<VT_LAY_N;own++){
   globalView=global;gravarEscolha(&folder,own);unsigned before=writes;readonlyOpen();CHECK(layEscolha==own);CHECK(lay==expected(global,own,director));CHECK(writes==before);CHECK(escolhaGravada(&folder)==own);
   if(lay==VT_LAY_VITRINE){CHECK(vitMulti==(ns>1));if(ns>1)CHECK(vitN==(ns<VIT_LIN_MAX?ns:VIT_LIN_MAX));for(int i=0;i<ns*2+2;i++)vitCarregar(clockNow+=100);CHECK(vitN>0&&vitN<=VIT_LIN_MAX);for(int r=0;r<vitN;r++)CHECK(vitLin[r].n>=0&&vitLin[r].n<=vitCap);}
   else {montar(itemCount);CHECK(celN==itemCount);for(int i=0;i<itemCount;i++)CHECK(isfinite(celX[i])&&isfinite(celY[i])&&celW[i]>0&&celH[i]>0);}
  }
 }
 /* Default removes only the current valid override, including duplicate current entries. */
 folder.nSources=2;snprintf(folder.group,sizeof folder.group,"Awards");globalView=VT_LAY_MURAL;profile=1;
 CHECK(dados_gravar(arquivoEscolha(),"family/one\t7\nfamily/two\t4\nfamily/one\t2\n"));readonlyOpen();CHECK(lay==VT_LAY_VITRINE);enterView();CHECK(layEscolha==0);CHECK(lay==VT_LAY_MURAL);CHECK(escolhaGravada(&folder)==0);CHECK(escolhaGravada(&other)==VT_LAY_SAGA);char*t=dados_ler(arquivoEscolha());CHECK(t&&!strstr(t,"family/one")&&strstr(t,"family/two\t4"));free(t);
 /* All eight choices are reachable through the actual production event handler and Default stays inherited. */
 for(int i=1;i<=VT_LAY_N;i++){enterView();CHECK(layEscolha==i%VT_LAY_N);CHECK(lay==expected(globalView,i%VT_LAY_N,0));}
 /* Old explicit0 and absent entries inherit, and opening does not migrate either file. */
 CHECK(dados_gravar(arquivoEscolha(),"family/one\t0\nfamily/two\t4\n"));readonlyOpen();CHECK(layEscolha==0&&lay==VT_LAY_MURAL);t=dados_ler(arquivoEscolha());CHECK(t&&strstr(t,"family/one\t0"));free(t);
 /* Existing per-folder bank remains profile-isolated. */
 gravarEscolha(&folder,VT_LAY_RANKING);profile=2;readonlyOpen();CHECK(layEscolha==0&&lay==VT_LAY_MURAL);gravarEscolha(&folder,VT_LAY_DECADAS);profile=1;readonlyOpen();CHECK(layEscolha==VT_LAY_RANKING&&lay==VT_LAY_RANKING);profile=2;readonlyOpen();CHECK(layEscolha==VT_LAY_DECADAS&&lay==VT_LAY_DECADAS);profile=1;
 /* Seeall always retains its pre-feature policy for every global/default/explicit combination. */
 lookup=&folder;for(int director=0;director<2;director++){snprintf(folder.group,sizeof folder.group,"%s",director?"Directors":"Awards");for(int g=0;g<VT_LAY_N;g++)for(int own=0;own<VT_LAY_N;own++){
 globalView=g;gravarEscolha(&folder,own);unsigned before=writes,oldloads=allLoads;vertudo_abrir(sources[0].base,sources[0].type,sources[0].catId,"one");CHECK(semVitrineAuto);CHECK(lay==((own>0&&own!=VT_LAY_VITRINE)?own:director?VT_LAY_LINHA:VT_LAY_GRADE));CHECK(writes==before);CHECK(allLoads==oldloads+1);CHECK(escolhaGravada(&folder)==own);}}
 lookup=NULL;globalView=VT_LAY_VITRINE;vertudo_abrir(sources[0].base,sources[0].type,sources[0].catId,"unrelated");CHECK(lay==VT_LAY_GRADE);
 /* Invalid config getters are ignored by the consumer too. */
 snprintf(folder.group,sizeof folder.group,"Awards");gravarEscolha(&folder,0);int bad[]={-1,VT_LAY_N,999};for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){globalView=bad[i];readonlyOpen();CHECK(lay==VT_LAY_GRADE);}
 /* Empty folders do not issue a request or persist anything. */
 unsigned before=writes,oldloads=loads;folder.nSources=0;vertudo_colecao(&folder);CHECK(writes==before&&loads==oldloads);folder.nSources=2;
 /* Actual View pill, real retained target + finger events + persisted/reopened values. */
 for(int orientation=0;orientation<2;orientation++)for(int zoom=0;zoom<3;zoom++)for(int count=0;count<2;count++){
  folder.nSources=count?COL_SOURCE_MAX:1;
  uiScale=(float[]){1,1.3f,1.5f}[zoom];drawScale=uiScale;layout_modo_definir(1);layout_tela_definir(orientation?2340:1080,orientation?1080:2340);
  globalView=VT_LAY_MURAL;gravarEscolha(&folder,VT_LAY_AUTO);readonlyOpen();ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);
  for(int v=1;v<=VT_LAY_N;v++){
   ponteiro_quadro(clockNow);ponteiro_camada();if(lay!=VT_LAY_VITRINE)themeHeader(1,vtInicio());botaoExibicao(1,vtInicio());ponteiro_desenhar();const PonteiroAlvo*targets;int nt=ponteiro_teste_lista(&targets),view=-1;for(int i=0;i<nt;i++)if(targets[i].ativar==toqueExibicao){CHECK(view==-1);view=i;}CHECK(view>=0);PonteiroAlvo target=targets[view];
   for(int i=0;i<nt;i++)if(i!=view){float w=fminf(targets[i].x+targets[i].w,target.x+target.w)-fmaxf(targets[i].x,target.x),h=fminf(targets[i].y+targets[i].h,target.y+target.h)-fmaxf(targets[i].y,target.y);CHECK(w<=0||h<=0);}
   CHECK(target.w>=143.99f&&target.h>=143.99f);CHECK(target.x>=0&&target.y>=0&&target.x+target.w<=NV_LAYOUT_REAL_W+.1f&&target.y+target.h<=NV_LAYOUT_REAL_H+.1f);
   if(lay!=VT_LAY_VITRINE)CHECK(target.y+target.h<=(VT_TOPO-12)*uiScale+.1f);
   float x=target.x+target.w/2,y=target.y+target.h/2;CHECK(ponteiro_achar(targets,nt,x,y)==view);unsigned old=writes;fingerTap(x,y);CHECK(writes==old+1);CHECK(layEscolha==v%VT_LAY_N);CHECK(lay==expected(globalView,v%VT_LAY_N,0));
   /* Render next frame with the actual retained Display focus before reopening. */
   ponteiro_quadro(clockNow);ponteiro_camada();if(lay!=VT_LAY_VITRINE)themeHeader(1,vtInicio());botaoExibicao(1,vtInicio());ponteiro_desenhar();
   readonlyOpen();CHECK(escolhaGravada(&folder)==v%VT_LAY_N);
  }
  /* A retained Mobile target cannot mutate TV or closed page state. */
  ponteiro_quadro(clockNow);ponteiro_camada();botaoExibicao(1,vtInicio());ponteiro_desenhar();const PonteiroAlvo*targets;CHECK(ponteiro_teste_lista(&targets)==1);PonteiroAlvo stale=targets[0];unsigned old=writes;int oldchoice=layEscolha;layout_modo_definir(0);stale.ativar(stale.a,stale.b);CHECK(writes==old&&layEscolha==oldchoice);
  ponteiro_quadro(clockNow);ponteiro_camada();botaoExibicao(1,vtInicio());ponteiro_desenhar();CHECK(ponteiro_teste_lista(&targets)==0);layout_modo_definir(1);aberta=0;stale.ativar(stale.a,stale.b);CHECK(writes==old&&layEscolha==oldchoice);aberta=1;
 }
 printf("PASS actual collection views: %u assertions\n",checks);return 0;
}

/* Offline catalog boundary: hide-watched is Off; raw and visible snapshots coincide. */
int ajustes_ocultar_filmes_vistos(void){return 0;}
unsigned cat_revisao_itens(void){return 1;}
unsigned desc_vertudo_revisao(void){return 1;}
int cat_copiar_por_id(const char*i,const char*t,CatItem*d){(void)i;(void)t;(void)d;return 0;}
int desc_vertudo_n_bruto(void){return desc_vertudo_n();}
int desc_vertudo_item_bruto(int i,CatItem*d){return desc_vertudo_item(i,d);}

/* New copied lookup/status boundary; retain the original fixture assertions. */
int addons_base_por_id_copiar(const char *id, char *dst, unsigned tam) {
  const char *base = addons_base_por_id(id);
  if (!tam) return 0;
  if (strlen(base) >= tam) { dst[0] = 0; return 0; }
  snprintf(dst, tam, "%s", base); return base[0] != 0;
}
AddonIdentidadeEstado addons_identidade_estado(const char *id) {
  (void)id; return ADD_IDENTIDADE_AUSENTE;
}
