/* Real style-cell renderer, wide translated text and existing delay actions.
 * No media/backend changes; native-font snapshots are checked separately. */
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#include "../src/faixas.c"
#include <assert.h>
#include <math.h>
float nv_layout_w=2160,nv_layout_h=1080;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
static float zoom=1;
static GfxRect cell;
static int wide,ass,changes,nHits,blocks;
static VideoLegendaEstilo estilo={120,0,0,3,1,0,0,TXT_FAMILIA_INTER};
static struct {GfxRect r;PonteiroFn focus,action;int a,b;} hits[4];
const char *const TXT_FAMILIAS_PT[TXT_FAMILIA_N]={"Inter","LG Display","Droid Sans","Montserrat","Roboto","Atkinson Hyperlegible Next"};
const char *const VIDEO_LEG_CORES_PT[VIDEO_LEG_NCORES]={"Branco","Amarelo","Verde","Azul","Vermelho","Preto"};
const char *i18n(const char *s) {
  if(!wide)return s;
  if(!strcmp(s,"Restaurar padrão"))return "Restaurar valores predeterminados";
  if(!strcmp(s,"Fonte OpenSubtitles"))return "Subtitle font from OpenSubtitles";
  if(!strcmp(s,"Preservado pelo ASS"))return "Formatting preserved by the subtitle file";
  if(!strcmp(s,"Branco"))return "A particularly long translated colour";
  return s;
}
float gfx_escala_ui(void){return zoom;}
float gfx_escala(void){return zoom;}
int ponteiro_ativo(void){return 1;}
static void bounded(GfxRect r) {
  assert(isfinite(r.x)&&isfinite(r.y)&&r.w>=0&&r.h>=0);
  if(r.x<cell.x-.01f||r.x+r.w>cell.x+cell.w+.01f||r.y<cell.y-.01f||r.y+r.h>cell.y+cell.h+.01f) {
    fprintf(stderr,"cell %.1f,%.1f %.1fx%.1f, draw %.1f,%.1f %.1fx%.1f\n",cell.x,cell.y,cell.w,cell.h,r.x,r.y,r.w,r.h);assert(0);
  }
}
int txt_largura(TxtEstilo e,const char *s){(void)e;s=i18n(s);if(!strcmp(s,"Restaurar valores predeterminados"))return wide==1?328:373;return (int)strlen(s)*(wide?12:10);}
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a){(void)r;(void)g;(void)b;(void)a;return (TxtLinha){.w=txt_largura(e,s),.h=e==TXT_G16B?20:27};}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w){assert(w>0);TxtLinha l=txt_linha(e,s,r,g,b,a);if(l.w>w)l.w=(int)w;return l;}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a){if(a>0)bounded((GfxRect){x,y,l.w,l.h});}
float txt_bloco_corta(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float lead,float a,int max){
  assert(w>0);int n=(int)ceilf(txt_largura(e,s)/w);if(max>0&&n>max)n=max;(void)r;(void)g;(void)b;
  blocks++;if(a>0)bounded((GfxRect){x,y,w,n*lead});return n*lead;
}
void gfx_cor(GfxRect r,float radius,float cr,float cg,float cb,float a){(void)radius;(void)cr;(void)cg;(void)cb;if(a>0)bounded(r);}
void gfx_icone(GfxRect r,const char *s,float cr,float cg,float cb,float a){(void)s;gfx_cor(r,0,cr,cg,cb,a);}
void plrui_linha_foco(GfxRect r,float radius,float a){gfx_cor(r,radius,1,1,1,a);}
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int i,int b){
  GfxRect r={x,y,w,h};bounded(r);assert(nHits<4);hits[nHits].r=r;hits[nHits].focus=f;hits[nHits].action=a;hits[nHits].a=i;hits[nHits++].b=b;
}
int assrender_ativo(void){return ass;}
int assrender_texto_simples(void){return 0;}
VideoLegendaEstilo *player_leg_estilo(void){return &estilo;}
void player_leg_estilo_mudou(void){changes++;}
void plrui_decimal(char *s){(void)s;}
int ajustes_idioma(void){return 0;}
int main(void){
  const float screens[][2]={{1080,1920},{1080,2340},{2160,1080},{2340,1080},{2520,1080}};
  const float scales[]={1,1.2f,1.3f,1.5f};int cases=0;
  aberta=modo=1;coluna=FX_COL_ESTILO;
  for(int t=0;t<5;t++)for(int z=0;z<4;z++)for(wide=0;wide<3;wide++)for(ass=0;ass<2;ass++) {
    nv_layout_w=screens[t][0];nv_layout_h=screens[t][1];zoom=scales[z];assert(telefoneui_ativo());
    float cw=(NV_TELA_W-192-IL_PAD_X*2-24)/FX_BARRA_COLS;
    for(int variant=0;variant<TXT_FAMILIA_N;variant++)for(int i=0;i<FX_N_ESTILO;i++)for(int selected=0;selected<2;selected++) {
      cell=(GfxRect){114+(i%5)*(cw+6),150+(i/5)*(IL_EST_CEL_H+6),cw,IL_EST_CEL_H};
      foco[FX_COL_ESTILO]=selected?i:(i+1)%FX_N_ESTILO;nHits=0;
      estilo.atrasoMs=-5000;estilo.familia=variant;estilo.cor=variant%VIDEO_LEG_NCORES;
      estilo.opacidade=variant%4;estilo.fundo=variant%5;estilo.posicao=variant%8;estilo.borda=variant%3;estilo.negrito=variant%2;
      int old=changes;celulaEstilo(i,cell,1);assert(changes==old);
      int arrows=i==7&&selected;assert(nHits==1+2*arrows);
      if(arrows){hits[2].action(hits[2].a,hits[2].b);assert(estilo.atrasoMs==-4750&&changes==old+1);hits[1].action(hits[1].a,hits[1].b);assert(estilo.atrasoMs==-5000);}
      cases++;
    }
  }
  testMobile=0; nv_layout_w=1920;nv_layout_h=1080;assert(!telefoneui_ativo());
  zoom=1;wide=ass=0;cell=(GfxRect){100,100,334,92};nHits=0;int oldBlocks=blocks;
  celulaEstilo(FX_N_ESTILO-1,cell,1);assert(blocks==oldBlocks); /* Original TV draw path. */
  printf("faixas_estilo_phone: %d actual style-cell drawings, bounded wide labels/values/hits, existing delay actions PASS\n",cases);
  return 0;
}
