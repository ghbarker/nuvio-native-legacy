/* Real complete secondary caption renderer and its existing override policy. */
#include SECONDARY_SOURCE
#include <assert.h>
static int checks,own,whichBorder,samples,draws,bg_calls;static float bg_alpha;
static VideoLegendaEstilo style={120,0,2,3,1,0,0,TXT_FAMILIA_INTER,0,0};
typedef struct{int r,g,b,a;float alpha,x,y;} Sample;
static Sample sample[128],drawing[32];
static void check(int ok,const char*why){checks++;if(!ok){fprintf(stderr,"FAIL %s\n",why);abort();}}
VideoLegendaEstilo *player_leg_estilo(void){return &style;}
int ajustes_leg2_tamanho(void){return own?160:0;}
int ajustes_leg2_cor(void){return own?3:-1;}
int ajustes_leg2_fundo(void){return own?0:-1;}
int ajustes_leg2_borda(void){return own?(whichBorder+1)%3:-1;}
float gfx_escala_ui(void){return 1;}
int plrilha_rect(GfxRect*r){(void)r;return 0;}
int legenda2_cues(double pos,LegendaCue*out,int max){(void)pos;check(max>0,"secondary cue capacity");memset(out,0,sizeof *out);strcpy(out->texto,"Secondary");return 1;}
int txt_bidi_legenda(TxtFamilia f,TxtEstilo e,const char*s,char*out,size_t cap){(void)f;(void)e;(void)s;(void)out;(void)cap;return 0;}
TxtLinha txt_linha_corta_enfase(TxtEstilo e,const char*s,int r,int g,int b,int a,float max,TxtFamilia f,int enf){(void)e;(void)f;(void)enf;check(samples<128,"bounded secondary texture capture");sample[samples]=(Sample){r,g,b,a};int id=++samples;return (TxtLinha){.tex=id,.w=fminf(max,strlen(s)*24),.h=54};}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a){check(t.tex>0&&t.tex<=samples&&draws<32,"valid secondary texture");Sample d=sample[t.tex-1];d.alpha=a;d.x=x;d.y=y;drawing[draws++]=d;}
void gfx_cor(GfxRect r,float rad,float red,float green,float blue,float alpha){(void)r;(void)rad;check(red==0&&green==0&&blue==0,"secondary background remains black independently from selected edge color");bg_alpha=alpha;bg_calls++;}
int main(void){layout_tela_definir(1080,2340);layout_modo_definir(1);unsigned palette[6]={0x000000,0xffffff,0xffde30,0x40e070,0x4e9cff,0xff5050};
for(int c=0;c<6;c++)for(whichBorder=0;whichBorder<3;whichBorder++)for(int opacity=0;opacity<4;opacity++)for(own=0;own<2;own++)for(int stacked=0;stacked<2;stacked++){
style.corBorda=c;style.borda=whichBorder;style.opacidade=opacity;style.cor=1;style.fundo=2;samples=draws=bg_calls=0;LegendasGeo geo={0};geo.videoW=NV_TELA_W;geo.videoH=NV_TELA_H;geo.alpha=.8f;geo.baseY=900;geo.junto=stacked;
legendasui_desenhar_secundaria(&geo);int border=own?(whichBorder+1)%3:whichBorder;int expected=border==1?4:border==2?2:1;check(draws==expected,"secondary uses own/inherited outline mode with unchanged draw count");float alpha=(1-.25f*opacity)*.8f;
for(int i=0;i<draws-1;i++){unsigned rgb=((unsigned)drawing[i].r<<16)|((unsigned)drawing[i].g<<8)|(unsigned)drawing[i].b;check(rgb==palette[c]&&drawing[i].a==255,"secondary edge follows shared selected RGB across independent text-style overrides");check(fabsf(drawing[i].alpha-.82f*alpha)<.00001f,"secondary edge opacity follows existing82percent multiplier");}
Sample fg=drawing[draws-1];unsigned foreground=((unsigned)fg.r<<16)|((unsigned)fg.g<<8)|(unsigned)fg.b;check(foreground==(own?0x4e9cffu:0xffde30u),"independent secondary text color override remains separate from edge color");check(fabsf(fg.alpha-alpha)<.00001f,"secondary foreground opacity unchanged");check(bg_calls==(own?0:1),"independent secondary background override preserved");if(!own)check(fabsf(bg_alpha-.32f*alpha)<.00001f,"secondary black background alpha unchanged");
check(fg.x>=0&&fg.x+216<=NV_TELA_W&&fg.y>=0&&fg.y+54<=NV_TELA_H,"colored edge does not move caption outside canvas");float oldHeight=legendasui_altura_secundaria(&geo);style.corBorda=(c+1)%6;float newHeight=legendasui_altura_secundaria(&geo);check(oldHeight==newHeight&&oldHeight>0,"changing only edge color preserves stacked subtitle height");}
printf("PASS secondaryGL checks=%d colors=6 border_modes=3 opacity_modes=4 override_modes=2 placements=2\n",checks);return 0;}

/* Approved API-boundary adapter: keep the fixture's exact preexisting controlled
 * Settings fields; new fields inherit. Full new Settings behavior is tested
 * separately with the actual production resolver. */
int ajustes_leg2_posicao(void) { return -1; }
void ajustes_leg2_estilo(const VideoLegendaEstilo *main, VideoLegendaEstilo *out) {
  *out = *main;
  int size = ajustes_leg2_tamanho(), color = ajustes_leg2_cor();
  int background = ajustes_leg2_fundo(), border = ajustes_leg2_borda();
  out->tamanho = size > 0 ? size : main->tamanho * 9 / 10;
  if (color >= 0) out->cor = color;
  if (background >= 0) out->fundo = background;
  if (border >= 0) out->borda = border;
}
