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
int main(int argc,char**argv){assert(argc==5);layout_tela_definir(atoi(argv[1]),atoi(argv[2]));layout_modo_definir(atoi(argv[4]));scale=atof(argv[3]);ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);float width=NV_TELA_W-192;panel=(GfxRect){NV_TELA_W-NV_ILHA_MARGEM_D-width,NV_ILHA_Y+64,width,alturaEstilo()};
/* Same whole Style-body rectangle and current chosen-cell/remaining rows as
 * production corpoEstilo/controleEstilo. The owner supplies exact helper
 * functions for responsive padding/cell height before final binding. */
float pad=estiloPadX(),cell=IL_EST_CEL_H;
GfxRect palette={panel.x+pad,panel.y+IL_PAD_Y+IL_EST_TOPO+14+cell+6,panel.w-2*pad,paletaColunas(panel.w-2*pad)<VIDEO_LEG_NCORES ? IL_EST_GRADE_H-cell-6 : cell};
for(int control=0;control<2;control++)for(int chosen=0;chosen<VIDEO_LEG_NCORES;chosen++)for(int point=0;point<9;point++){
ctl=ctlVis=control?EST_COR_BORDA:EST_COR;ctlAnim=1;style.corBorda=chosen;style.cor=chosen;ctlSub=chosen;memset(ctlAmostra,0,sizeof ctlAmostra);ctlAmostra[chosen]=1;ponteiro_quadro(tick);ponteiro_camada();paletaCtl(palette,1,1);ponteiro_desenhar();
const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);check(n==6,"all six palette choices register actual pointer targets");targets+=n;
for(int i=0;i<n;i++){
check(v[i].x>=panel.x*scale-.01f&&v[i].x+v[i].w<=(panel.x+panel.w)*scale+.01f,"target fully inside real clipped panel");check(v[i].y>=panel.y*scale-.01f&&v[i].y+v[i].h<=(panel.y+panel.h)*scale+.01f,"target vertically inside real clipped panel");
if(layout_modo_mobile())check(v[i].w>=143.99f&&v[i].h>=143.99f,"actual Mobile target at least144base units or48dp at360dp short edge");
for(int j=i+1;j<n;j++){float ow=fminf(v[i].x+v[i].w,v[j].x+v[j].w)-fmaxf(v[i].x,v[j].x),oh=fminf(v[i].y+v[i].h,v[j].y+v[j].h)-fmaxf(v[i].y,v[j].y);check(ow<=.01f||oh<=.01f,"actual palette targets do not overlap");}
}
if(layout_modo_mobile()&&n==6){float fx[]={.01f,.5f,.99f},fy[]={.01f,.5f,.99f};float x=v[chosen].x+v[chosen].w*fx[point%3],y=v[chosen].y+v[chosen].h*fy[point/3];check(ponteiro_achar(v,n,x,y)==chosen,"color center/edge/corner has correct actual pointer target");tap(x,y);check(ctl==-1&&(control?style.corBorda:style.cor)==chosen,"actual finger tap chooses exact visible color and closes palette");}
}
printf("RESULT actual palette checks=%d failures=%d rings=%d targets=%d\n",checks,failures,rings,targets);return failures?1:0;}
