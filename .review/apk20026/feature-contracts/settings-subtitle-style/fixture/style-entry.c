/* Real complete palette function; typed renderer records its primitive bounds.
 * Its outer focus-ring growth follows the unchanged gfx_anel_fora contract. */
#include FAIXAS_SOURCE
#include <assert.h>
#define style (*player_leg_estilo())
static GfxRect panel;static int failures,rings,targets,applied,touched,checks;static float scale=1;static Uint32 tick=1000;


static int ass;int assrender_ativo(void){return ass;}int assrender_texto_simples(void){return !ass;}
int ajustes_vidro(void){return 0;}
void ajustes_acento_marca(float*r,float*g,float*b){*r=.5f;*g=.5f;*b=1;}
void gfx_cor(GfxRect r,float radius,float red,float green,float blue,float alpha){(void)r;(void)radius;(void)red;(void)green;(void)blue;(void)alpha;}
void gfx_anel(GfxRect r,float radius,float esp,float red,float green,float blue,float alpha){(void)r;(void)radius;(void)esp;(void)red;(void)green;(void)blue;(void)alpha;}
void gfx_anel_fora(GfxRect r,float radius,float gap,float esp,float red,float green,float blue,float alpha){(void)radius;(void)red;(void)green;(void)blue;(void)alpha;float grow=gap+esp;GfxRect outer={r.x-grow,r.y-grow,r.w+2*grow,r.h+2*grow};rings++;if(outer.x<panel.x-.01f||outer.x+outer.w>panel.x+panel.w+.01f||outer.y<panel.y-.01f||outer.y+outer.h>panel.y+panel.h+.01f){failures++;printf("CLIPPED palette ring x=%.3f..%.3f panel=%.3f..%.3f scale=%.2f\n",outer.x,outer.x+outer.w,panel.x,panel.x+panel.w,scale);}}

float gfx_escala(void){return scale;}float gfx_escala_ui(void){return scale;}
Uint32 SDL_GetTicks(void){return tick;}int SDL_GetNumTouchDevices(void){return 1;}
SDL_Window*SDL_GL_GetCurrentWindow(void){return NULL;}SDL_Window*SDL_GetWindowFromID(Uint32 id){(void)id;return NULL;}SDL_Window*SDL_GetMouseFocus(void){return NULL;}
Uint32 SDL_GetMouseState(int*x,int*y){if(x)*x=0;if(y)*y=0;return 0;}void SDL_GetWindowSize(SDL_Window*w,int*x,int*y){(void)w;*x=1080;*y=2340;}void SDL_GL_GetDrawableSize(SDL_Window*w,int*x,int*y){SDL_GetWindowSize(w,x,y);}int SDL_ShowCursor(int q){(void)q;return 0;}
float gfx_opacidade_grupo=1;void gfx_sem_recorte(void){}
static void check(int ok,const char*why){checks++;if(!ok){failures++;fprintf(stderr,"FAIL %s scale=%.2f mobile=%d\n",why,scale,layout_modo_mobile());}}
static void deliver(const SDL_Event*e){faixas_evento(e);}
static void tap(float x,float y){SDL_Event e;SDL_zero(e);e.type=SDL_FINGERDOWN;e.tfinger.touchId=3;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y/NV_LAYOUT_REAL_H;ponteiro_evento(&e,deliver);tick+=60;e.type=SDL_FINGERUP;ponteiro_evento(&e,deliver);tick+=400;}

const char *const VIDEO_LEG_CORES[VIDEO_LEG_NCORES]={"white","yellow","green","blue","red","black"};
const char *const VIDEO_LEG_CORES_PT[VIDEO_LEG_NCORES]={"Branco","Amarelo","Verde","Azul","Vermelho","Preto"};
const char *const TXT_FAMILIAS_PT[TXT_FAMILIA_N]={"Inter","LG","Droid","Montserrat","Roboto","Atkinson","JetBrains"};
const char *i18n(const char*s){return s;}void plrui_decimal(char*s){(void)s;}
int video_n_audio(void){return 0;}
int video_n_legenda(void){return 0;}int addons_n_legendas(void){return 0;}
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)e;(void)s;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){.w=40,.h=20};}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float w){TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=w;return t;}
TxtLinha txt_linha_corta_familia(TxtEstilo e,const char*s,int r,int g,int b,int a,float w,TxtFamilia f){(void)f;return txt_linha_corta(e,s,r,g,b,a,w);}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a){(void)l;(void)x;(void)y;(void)a;}
float txt_bloco_corta(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float leading,float a,int max){(void)e;(void)s;(void)r;(void)g;(void)b;(void)x;(void)y;(void)w;(void)a;return leading*(max>0?max:1);}
void gfx_icone(GfxRect r,const char*s,float red,float green,float blue,float a){(void)r;(void)s;(void)red;(void)green;(void)blue;(void)a;}
void plrui_linha_foco(GfxRect r,float radius,float a){(void)r;(void)radius;(void)a;}
float plrui_seg(const char*const*r,const int*c,int n,int sel,int ed,float x,float y,float a){(void)r;(void)c;(void)sel;(void)ed;(void)x;(void)y;(void)a;return n*80;}
float plrui_dicas(const char*const*k,const char*const*r,int n,float x,float y,int right,float a){(void)k;(void)r;(void)n;(void)x;(void)y;(void)right;(void)a;return 100;}

void test_player_seed(const char*dir);
void test_player_preserved(void);
void test_player_unwritten(void);
void test_island_present(void);
void test_island_fade(void);
void test_island_cancelled(void);
void test_player_reload(void);
static int backendReads;
int video_velocidade_bloqueada(void){backendReads++;return 0;}
int video_velocidade_atual(void){return 100;}
int video_velocidade_suportada(void){return 0;}
unsigned legsync_manual_seq(void){return 0;}

static void key(SDL_Keycode k){SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;faixas_evento(&e);}
float plrui_botao_largura(const char*r,const char*i){(void)r;(void)i;return 160;}
float plrui_botao(float x,float y,const char*r,const char*i,float f,float a){(void)x;(void)y;(void)r;(void)i;(void)f;(void)a;return 160;}
static void frame(void){ponteiro_quadro(tick);ponteiro_camada();faixas_atualizar(.05f,tick);faixas_desenhar(tick);ponteiro_desenhar();tick+=50;}
void gfx_veu_css(GfxRect r,int d,float s,float p,float a){(void)r;(void)d;(void)s;(void)p;(void)a;}
float gfx_escala_entrar(void){return scale;}void gfx_escala_sair(float s){scale=s;}
int main(int argc,char**argv){
 assert(argc==6);layout_tela_definir(atoi(argv[1]),atoi(argv[2]));layout_modo_definir(atoi(argv[4]));scale=atof(argv[3]);
 ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);test_player_seed(argv[5]);
 /* Opening before playback must load existing preferences without writes/probes. */
 faixas_abrir_estilo_ajustes();check(faixas_aberta()&&faixas_estilo_ajustes()&&faixas_estilo_topo(),"Settings opens the shared Style editor directly");
 check(style.tamanho==150&&style.corBorda==4,"first open loads existing player style");test_player_unwritten();test_player_preserved();
 int reads=backendReads;ass=1;char value[100];valorEstilo(0,value,sizeof value);check(!strstr(value,"ASS"),"global editor ignores retained authored-ASS context");
 for(int row=0;row<FX_N_ESTILO;row++)check(!estiloPreservadoAss(row),"all global controls remain editable");
 for(int i=0;i<80;i++)frame();check(backendReads==reads,"standalone update never enters subtitle/media session work");
 anim=1;ctlAnim=0;float w=NV_TELA_W-192;panel=(GfxRect){NV_TELA_W-NV_ILHA_MARGEM_D-w,NV_ILHA_Y+64,w,alturaEstilo()};
 check(panel.y+panel.h<=NV_TELA_H+.01f,"whole Settings Style body fits actual viewport");ponteiro_quadro(tick);ponteiro_camada();corpoEstilo(panel,1);ponteiro_desenhar();
 const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v),close=-1;PonteiroFn stale=NULL;
 check(n==FX_N_ESTILO+1,"Settings has Close and every shared Style control, without Tracks");
 for(int i=0;i<n;i++){
 check(v[i].x>=panel.x*scale-.1f&&v[i].y>=panel.y*scale-.1f&&v[i].x+v[i].w<=(panel.x+panel.w)*scale+.1f&&v[i].y+v[i].h<=(panel.y+panel.h)*scale+.1f,"every emitted target remains inside actual body");
 if(layout_modo_mobile())check(v[i].w>=143.99f&&v[i].h>=143.99f,"every Mobile target at least48dp at360dp short edge");
 if(v[i].ativar==ponteiroFecharEstiloAjustes){close=i;stale=v[i].ativar;}
 }
 check(close>=0,"visible Close target exists");float closeX=v[close].x+v[close].w*.5f,closeY=v[close].y+v[close].h*.5f;
 /* Same shared control: adjust Outline color, immediate save, Back closes only the palette. */
 foco[FX_COL_ESTILO]=EST_COR_BORDA;key(SDLK_RETURN);check(ctl==EST_COR_BORDA,"same Outline color picker");key(SDLK_RIGHT);check(style.corBorda==5,"existing palette saves new color");key(SDLK_AC_BACK);check(aberta&&ctl<0,"Back returns from palette to same editor");
 /* Close owns input through both animations, then permits retained-player updates again. */
 test_island_present();tap(closeX,closeY);check(!aberta&&faixas_estilo_ajustes(),"Close retains fading editor ownership");
 key(SDLK_RETURN);check(!aberta,"closing keyboard input cannot reopen or edit");for(int i=0;i<80;i++)frame();check(faixas_estilo_ajustes(),"island fade also owns the screen");
 const PonteiroAlvo*shield;check(ponteiro_teste_lista(&shield)==1&&shield[0].ativar==NULL&&shield[0].focar==NULL,"closing frame has only inert full-screen shield");
 test_island_fade();frame();check(!faixas_estilo_ajustes(),"completed exit releases standalone mode");frame();check(backendReads>reads,"normal faixas updates resume without reiniciar on retained restore");
 test_player_reload();check(style.corBorda==5,"same player.txt receives style edit");test_player_preserved();
 /* Forced route/profile exit is immediate UI-only; no subtitle reset. */
 faixas_abrir_estilo_ajustes();frame();test_island_present();faixas_encerrar_estilo_ajustes();check(!faixas_aberta()&&!faixas_estilo_ajustes()&&anim==0&&ctlAnim==0,"route/session exit cancels only editor UI");test_island_cancelled();test_player_preserved();
 /* The old in-player Tracks/Style behavior and ASS restrictions are retained. */
 aberta=modo=1;coluna=FX_COL_ESTILO;foco[FX_COL_ESTILO]=0;check(estiloPreservadoAss(EST_COR_BORDA),"original in-player ASS restriction unchanged");stale(0,0);check(aberta,"stale Settings Close cannot close player sheet");key(SDLK_LEFT);check(coluna==1&&aberta,"in-player Left still returns to Tracks");
 printf("PASS actual Settings Style lifecycle/preferences/pointer checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
