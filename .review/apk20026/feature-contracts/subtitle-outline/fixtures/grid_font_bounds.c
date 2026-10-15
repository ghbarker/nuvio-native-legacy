/* Real complete palette function; typed renderer records its primitive bounds.
 * Its outer focus-ring growth follows the unchanged gfx_anel_fora contract. */
#include FAIXAS_SOURCE
#include <assert.h>
static VideoLegendaEstilo style={120,0,0,3,1,0,0,TXT_FAMILIA_INTER,0,0};
static GfxRect panel;static int failures,rings,targets,applied,touched,checks;static float scale=1;static Uint32 tick=1000;
VideoLegendaEstilo *player_leg_estilo(void){return &style;}
void player_leg_estilo_mudou(void){applied++;}void player_leg_estilo_tocou(int mask){(void)mask;touched++;}
int assrender_ativo(void){return 0;}int assrender_texto_simples(void){return 1;}
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
static void deliver(const SDL_Event*e){if(e->type==SDL_KEYDOWN)estiloTecla(e->key.keysym.sym,0);}
static void tap(float x,float y){SDL_Event e;SDL_zero(e);e.type=SDL_FINGERDOWN;e.tfinger.touchId=3;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y/NV_LAYOUT_REAL_H;ponteiro_evento(&e,deliver);tick+=60;e.type=SDL_FINGERUP;ponteiro_evento(&e,deliver);tick+=400;}

const char *const VIDEO_LEG_CORES[VIDEO_LEG_NCORES]={"white","yellow","green","blue","red","black"};
const char *const VIDEO_LEG_CORES_PT[VIDEO_LEG_NCORES]={"Branco","Amarelo","Verde","Azul","Vermelho","Preto"};
const char *const TXT_FAMILIAS_PT[TXT_FAMILIA_N]={"Inter","LG","Droid","Montserrat","Roboto","Atkinson","JetBrains"};
const char *i18n(const char*s){return s;}void plrui_decimal(char*s){(void)s;}
const char*player_id_canal(void){return "";}int video_n_audio(void){return 0;}
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
static void bounds(int expected){const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);check(n==expected,"expected actual grid/font pointer count");targets+=n;for(int i=0;i<n;i++){
/* The first body target is the unchanged Faixas tab. New grid/font choices follow. */
if(i==0)continue;
check(v[i].x>=panel.x*scale-.01f&&v[i].x+v[i].w<=(panel.x+panel.w)*scale+.01f,"new grid/font target horizontally inside island body");check(v[i].y>=panel.y*scale-.01f&&v[i].y+v[i].h<=(panel.y+panel.h)*scale+.01f,"new grid/font target vertically inside island body");check(v[i].x>=0&&v[i].x+v[i].w<=NV_LAYOUT_REAL_W+.01f&&v[i].y>=0&&v[i].y+v[i].h<=NV_LAYOUT_REAL_H+.01f,"new grid/font target inside canvas");if(layout_modo_mobile())check(v[i].w>=143.99f&&v[i].h>=143.99f,"new grid/font actual touch target at least48dp at360dp short edge");for(int j=i+1;j<n;j++){float ow=fminf(v[i].x+v[i].w,v[j].x+v[j].w)-fmaxf(v[i].x,v[j].x),oh=fminf(v[i].y+v[i].h,v[j].y+v[j].h)-fmaxf(v[i].y,v[j].y);check(ow<=.01f||oh<=.01f,"new grid/font actual pointer targets never overlap");}}
}
int main(int argc,char**argv){assert(argc==5);layout_tela_definir(atoi(argv[1]),atoi(argv[2]));layout_modo_definir(atoi(argv[4]));scale=atof(argv[3]);ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);aberta=1;modo=1;coluna=FX_COL_ESTILO;
for(int font=0;font<2;font++){
ctl=ctlVis=font?EST_FONTE:-1;ctlAnim=font?1:0;float w=NV_TELA_W-192;panel=(GfxRect){NV_TELA_W-NV_ILHA_MARGEM_D-w,NV_ILHA_Y+64,w,alturaEstilo()};check(panel.y+panel.h<=NV_TELA_H+.01f,"whole island body and font/grid footer stay inside screen");ponteiro_quadro(tick);ponteiro_camada();corpoEstilo(panel,1);ponteiro_desenhar();bounds(font?TXT_FAMILIA_N+1:FX_N_ESTILO+1);
const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);if(font&&layout_modo_mobile()&&n==TXT_FAMILIA_N+1){for(int chosen=0;chosen<TXT_FAMILIA_N;chosen++)for(int point=0;point<9;point++){
ctl=ctlVis=EST_FONTE;ctlAnim=1;ponteiro_quadro(tick);ponteiro_camada();corpoEstilo(panel,1);ponteiro_desenhar();n=ponteiro_teste_lista(&v);float f[]={.01f,.5f,.99f};float x=v[chosen+1].x+v[chosen+1].w*f[point%3],y=v[chosen+1].y+v[chosen+1].h*f[point/3];check(ponteiro_achar(v,n,x,y)==chosen+1,"font center/edge/corner resolves exact visible target");tap(x,y);check(ctl==-1&&style.familia==chosen,"actual finger selects each of seven visible fonts");}}
}
printf("RESULT actual whole Style grid/font checks=%d failures=%d targets=%d\n",checks,failures,targets);return failures?1:0;}
