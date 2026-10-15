#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include FAIXAS_SOURCE
#include <assert.h>
#include <stdlib.h>

static VideoLegendaEstilo model={.tamanho=120,.posicao=3,.borda=1};
static int saved,touched,authored,plain=1,mobile;
float nv_layout_w=1920,nv_layout_h=1080;
VideoLegendaEstilo *player_leg_estilo(void){return &model;}
void player_leg_estilo_mudou(void){saved++;}
void player_leg_estilo_tocou(int fields){if(!fields)touched=0;else touched|=fields;}
int assrender_ativo(void){return authored;}
int assrender_texto_simples(void){return plain;}
int layout_modo_mobile(void){return mobile;}
const char *const VIDEO_LEG_CORES[VIDEO_LEG_NCORES]={"white","yellow","green","blue","red","black"};
const char *const VIDEO_LEG_CORES_PT[VIDEO_LEG_NCORES]={"Branco","Amarelo","Verde","Azul","Vermelho","Preto"};
const char *const TXT_FAMILIAS_PT[TXT_FAMILIA_N]={"Inter","LG","Droid","Montserrat","Roboto","Atkinson","JetBrains"};
const char *i18n(const char *s){return s;}
void plrui_decimal(char *s){(void)s;}
int ajustes_vidro(void){return 0;}
void ajustes_acento_marca(float*r,float*g,float*b){*r=.2f;*g=.4f;*b=.8f;}
float gfx_escala(void){return 1;}
float gfx_escala_ui(void){return 1;}
void gfx_anel(GfxRect r,float raio,float esp,float cr,float cg,float cb,float ca){(void)r;(void)raio;(void)esp;(void)cr;(void)cg;(void)cb;(void)ca;}
void gfx_anel_fora(GfxRect r,float raio,float gap,float esp,float cr,float cg,float cb,float ca){(void)r;(void)raio;(void)gap;(void)esp;(void)cr;(void)cg;(void)cb;(void)ca;}
TxtLinha txt_linha_corta_familia(TxtEstilo estilo,const char*s,int r,int g,int b,int a,float max,TxtFamilia family){(void)estilo;(void)r;(void)g;(void)b;(void)a;(void)family;int w=(int)strlen(s)*12;if(w>max)w=(int)max;return (TxtLinha){.tex=1,.w=w,.h=30};}
void txt_desenhar_alpha(TxtLinha line,float x,float y,float a){(void)line;(void)x;(void)y;(void)a;}
void plrui_linha_foco(GfxRect r,float radius,float a){(void)r;(void)radius;(void)a;}
void gfx_icone(GfxRect r,const char*name,float cr,float cg,float cb,float a){(void)r;(void)name;(void)cr;(void)cg;(void)cb;(void)a;}
static unsigned draws[16];static int nDraws;
void gfx_cor(GfxRect r,float raio,float cr,float cg,float cb,float ca){(void)raio;(void)ca;if(r.w<100&&r.h<100){assert(nDraws<16);draws[nDraws++]=((unsigned)lroundf(cr*255)<<16)|((unsigned)lroundf(cg*255)<<8)|(unsigned)lroundf(cb*255);}}
typedef struct{GfxRect r;PonteiroFn focus,click;int a,b;}Target;
static Target targets[16];static int nTargets;
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn focus,PonteiroFn click,int a,int b){assert(nTargets<16);targets[nTargets++]=(Target){{x,y,w,h},focus,click,a,b};}
static void key(SDL_Keycode k){SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;faixas_evento(&e);}
static void openStyle(void){aberta=1;modo=1;coluna=FX_COL_ESTILO;ctl=-1;foco[FX_COL_ESTILO]=EST_COR_BORDA;}
int main(void){
 unsigned expected[]={0,0xffffff,0xffde30,0x40e070,0x4e9cff,0xff5050};int checks=0;
 for(mobile=0;mobile<2;mobile++){
  openStyle();model.corBorda=0;model.cor=4;saved=touched=0;
  assert(FX_N_ESTILO==11&&estTipo(EST_COR_BORDA)==2);checks++;
  key(SDLK_RETURN);assert(ctl==EST_COR_BORDA&&ctlSub==0);checks++;
  for(int i=1;i<=6;i++){key(SDLK_RIGHT);assert(model.corBorda==i%6&&model.cor==4&&touched==0);checks++;}
  key(SDLK_AC_BACK);assert(ctl==-1&&aberta);checks++;
  for(int i=0;i<6;i++){
   key(SDLK_RETURN);ctlAnim=1;nTargets=nDraws=0;
   paletaCtl((GfxRect){50,100,1000,100},1,1);
   assert(nTargets==6&&nDraws==6);checks++;
   for(int j=0;j<6;j++){assert(draws[j]==expected[j]&&targets[j].click==ponteiroEscolher);checks++;}
   if(targets[i].focus)targets[i].focus(targets[i].a,targets[i].b);
   targets[i].click(targets[i].a,targets[i].b);
   assert(ctl==-1&&model.corBorda==i&&model.cor==4&&touched==0);checks++;
   char value[80];valorEstilo(EST_COR_BORDA,value,sizeof value);
   assert(!strcmp(value,VIDEO_LEG_CORES_PT[i?i-1:5]));checks++;
  }
  foco[FX_COL_ESTILO]=EST_COR;key(SDLK_RETURN);int old=model.cor;key(SDLK_RIGHT);
  assert(model.cor==(old+1)%6&&model.corBorda==5&&(touched&PLR_LEG_COR));checks++;
  key(SDLK_AC_BACK);foco[FX_COL_ESTILO]=EST_FONTE;key(SDLK_RETURN);old=model.familia;key(SDLK_RIGHT);
  assert(model.familia==old+1&&model.corBorda==5);checks++;
  key(SDLK_AC_BACK);
  for(int i=0;i<TXT_FAMILIA_N;i++){
   key(SDLK_RETURN);ctlAnim=1;nTargets=nDraws=0;
   GfxRect fonts={50,100,1000,IL_EST_GRADE_H};fontesCtl(fonts,1,1);
   assert(nTargets==TXT_FAMILIA_N);checks++;
   for(int j=0;j<TXT_FAMILIA_N;j++){
    assert(targets[j].r.y>=fonts.y&&targets[j].r.y+targets[j].r.h<=fonts.y+fonts.h+.01f);checks++;
   }
   if(targets[i].focus)targets[i].focus(targets[i].a,targets[i].b);
   targets[i].click(targets[i].a,targets[i].b);
   assert(ctl==-1&&model.familia==i&&model.corBorda==5);checks++;
  }
  model.familia=0;key(SDLK_RETURN);key(SDLK_DOWN);key(SDLK_DOWN);key(SDLK_DOWN);
  assert(ctlSub==6&&model.familia==6);checks++;
  key(SDLK_RIGHT);assert(ctlSub==6);checks++;
  key(SDLK_UP);assert(ctlSub==4&&model.familia==4);checks++;
  key(SDLK_AC_BACK);foco[FX_COL_ESTILO]=7;key(SDLK_RETURN);old=model.atrasoMs;key(SDLK_RIGHT);
  assert(model.atrasoMs==old+100&&model.corBorda==5);checks++;
  key(SDLK_AC_BACK);foco[FX_COL_ESTILO]=4;key(SDLK_DOWN);
  assert(foco[FX_COL_ESTILO]==EST_RESTAURAR&&EST_RESTAURAR==9&&EST_COR_BORDA==10);checks++;
  key(SDLK_RETURN);assert(model.corBorda==0&&model.cor==0&&model.borda==1&&touched==0);checks++;
  foco[FX_COL_ESTILO]=EST_COR_BORDA;authored=1;plain=0;int oldSaved=saved;
  key(SDLK_RETURN);assert(ctl==-1&&model.corBorda==0&&saved==oldSaved);checks++;
  char value[80];valorEstilo(EST_COR_BORDA,value,sizeof value);assert(!strcmp(value,"Preservado pelo ASS"));checks++;
  authored=0;plain=1;key(SDLK_AC_BACK);assert(!aberta);checks++;
 }
 printf("PASS actual style controls %d checks\n",checks);return 0;
}
