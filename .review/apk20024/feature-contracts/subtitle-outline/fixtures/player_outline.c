/* Real complete player preferences and primary caption draw path. */
#include <SDL2/SDL.h>
char *SDL_GetBasePath(void);void SDL_free(void*);
#include PLAYER_SOURCE
#include <assert.h>
#include <limits.h>
static int checks,applied,dirty;
static VideoLegendaEstilo backend;
typedef struct{int r,g,b,a;float x,y,alpha;} Sample;
static Sample sample[32];static int samples,draws,bg_calls;static float bg_alpha;static Sample drawing[32];
static void check(int ok,const char*why){checks++;if(!ok){fprintf(stderr,"FAIL %s\n",why);abort();}}
char *SDL_GetBasePath(void){return NULL;}void SDL_free(void*p){free(p);}
void dados_marcar_sujo(int imediato){(void)imediato;dirty++;}
void video_legenda_estilo(const VideoLegendaEstilo*e){backend=*e;applied++;}
int txt_bidi_legenda(TxtFamilia f,TxtEstilo e,const char*s,char*out,size_t cap){(void)f;(void)e;(void)s;(void)out;(void)cap;return 0;}
TxtLinha txt_linha_corta_enfase(TxtEstilo e,const char*s,int r,int g,int b,int a,float max,TxtFamilia f,int enf){(void)e;(void)f;(void)enf;check(samples<32,"bounded primary draw capture");sample[samples]=(Sample){r,g,b,a};int id=++samples;return (TxtLinha){.tex=id,.w=fminf(max,strlen(s)*24),.h=54};}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a){check(t.tex>0&&t.tex<=samples&&draws<32,"valid primary text texture");Sample d=sample[t.tex-1];d.x=x;d.y=y;d.alpha=a;drawing[draws++]=d;}
void gfx_cor(GfxRect r,float rad,float red,float green,float blue,float alpha){(void)r;(void)rad;check(red==0&&green==0&&blue==0,"colored edge never colors the black subtitle background");bg_alpha=alpha;bg_calls++;}
static void write_pref(const char*s){FILE*f=fopen(prefsArquivo(),"w");check(f!=NULL,"isolated player preference file opens");check(fputs(s,f)>=0,"fixture preferences written");fclose(f);}
static void gl_check(void){
unsigned rgb[6]={0x000000,0xffffff,0xffde30,0x40e070,0x4e9cff,0xff5050};
for(int c=0;c<6;c++)for(int border=0;border<3;border++)for(int opacity=0;opacity<4;opacity++){
samples=draws=bg_calls=0;legEstilo.corBorda=c;legEstilo.borda=border;legEstilo.fundo=2;LegBloco bl={0};blocoLinha(&bl,TXT_LEG_120,"Color contrast",200,210,220,TXT_FAMILIA_INTER,0);check(bl.n==1&&samples==(border?2:1),"primary caption produces expected foreground and optional edge textures");check(sample[0].r==200&&sample[0].g==210&&sample[0].b==220&&sample[0].a==255,"outline preference preserves exact foreground RGBA");
if(border){check((((unsigned)sample[1].r<<16)|((unsigned)sample[1].g<<8)|(unsigned)sample[1].b)==rgb[c]&&sample[1].a==255,"primary GL texture receives selected RGB and unchanged raster alpha");}
float alpha=1-.25f*opacity;desenharBloco(&bl,960,900,alpha,1);check(draws==(border==1?4:border==2?2:1),"outline/shadow draw pass count unchanged");check(bg_calls==1&&fabsf(bg_alpha-.32f*alpha)<.00001f,"black background opacity remains independent from edge color");for(int i=0;i<draws-1;i++)check(fabsf(drawing[i].alpha-.82f*alpha)<.00001f,"primary outline/shadow retains existing82percent global-alpha multiplier");check(fabsf(drawing[draws-1].alpha-alpha)<.00001f,"foreground retains global subtitle opacity");
}
}
int main(int argc,char**argv){check(argc==2,"temporary preference directory argument");player_dir(argv[1]);VideoLegendaEstilo defaults=legEstilo;check(defaults.corBorda==0&&video_legenda_borda_rgb(defaults.corBorda)==0,"appending field gives old initializers pure-black default");
write_pref("aspecto 2\nleg_tamanho 2\nleg_cor 3\nleg_borda 2\nleg_fundo 1\nleg_pos 4\nleg_opacidade 2\nleg_familia 0\nleg_negrito 1\n");prefsLer();check(legEstilo.tamanho==120&&legEstilo.cor==3&&legEstilo.borda==2&&legEstilo.fundo==1&&legEstilo.posicao==4&&legEstilo.opacidade==2&&legEstilo.negrito==1,"old preference keys keep exact migration and styling");check(legEstilo.corBorda==0,"old preference file missing new key stays pure black");
for(int c=0;c<6;c++){legEstilo.corBorda=c;VideoLegendaEstilo saved=legEstilo;int oldapplied=applied;player_leg_estilo_mudou();check(applied==oldapplied+1&&backend.corBorda==c,"live style apply copies appended field into backend boundary");check(dirty>0,"real persistence marks app data dirty");legEstilo=defaults;prefsLer();check(!memcmp(&saved,&legEstilo,sizeof saved),"every palette color and all existing style fields survive actual file save/reload");FILE*f=fopen(prefsArquivo(),"r");check(f!=NULL,"saved preferences reopen");char data[1024];size_t n=fread(data,1,sizeof data-1,f);data[n]=0;fclose(f);char key[40];snprintf(key,sizeof key,"leg_cor_borda %d\n",c);check(strstr(data,key)!=NULL,"new color persisted under its named key");}
int invalid[3]={-1,6,INT_MAX};for(int i=0;i<3;i++){char data[100];snprintf(data,sizeof data,"leg_tamanho 160\nleg_cor_borda %d\n",invalid[i]);write_pref(data);legEstilo=defaults;prefsLer();check(legEstilo.corBorda==0&&legEstilo.tamanho==160,"invalid new key safely uses black without discarding existing preferences");}
gl_check();remove(prefsArquivo());printf("PASS primaryGL/preferences checks=%d colors=6 border_modes=3 opacity_modes=4\n",checks);return 0;}
