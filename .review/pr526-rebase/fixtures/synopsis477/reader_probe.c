/* External review: real synopsis module, layout, pointer and finger events.
 * Renderer uses font hmtx advances exported from the shipped Inter font;
 * it captures production draw calls, not an Android/GL screenshot. */
#include SINOPSE_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "font_metrics.inc"
static int font_size, translation, checks, total_frames, clip_on;
static uint64_t session=1;
static float active_scale=1, configured_scale=1;
static Uint32 ticks=1000;
static GfxRect clip, close_box, sheet;
static int num_lines;
typedef struct { char text[2048]; TxtEstilo style; int h,w; } Line;
static Line lines[4096];
static int drawn_tail, drawn_count;
static const char *tail="FINAL_SENTENCE_477";
static void check(int ok,const char *why){checks++;if(!ok){fprintf(stderr,"FAIL %s size=%d scale=%.2f mobile=%d\n",why,font_size,configured_scale,layout_modo_mobile());abort();}}
int ajustes_sinopse_tamanho(void){return font_size;}
TxtFamilia txt_fonte_interface(void){return TXT_FAMILIA_INTER;}
uint64_t perfis_sessao_geracao(void){return session;}
int perfis_sessao_atual(uint64_t s){return session==s;}
float gfx_escala(void){return active_scale;}
float gfx_escala_ui(void){return configured_scale;}
float gfx_escala_entrar(void){float old=active_scale;active_scale=configured_scale;return old;}
void gfx_escala_sair(float s){active_scale=s;}
float gfx_opacidade_grupo=1;
const char *i18n(const char *s){if(!translation)return s;if(!strcmp(s,"Ler sinopse"))return "Vollständige Zusammenfassung des ausgewählten Titels lesen";if(!strcmp(s,"Fechar"))return "Zusammenfassungsfenster schließen";return s;}
static unsigned codepoint(const char **s){const unsigned char*p=(const unsigned char*)*s;unsigned c=*p++;int n=0;if(c>=0xf0){c&=7;n=3;}else if(c>=0xe0){c&=15;n=2;}else if(c>=0xc0){c&=31;n=1;}else check(c<128,"rendered UTF-8 does not start mid-codepoint");for(int i=0;i<n;i++){check((*p&0xc0)==0x80,"rendered UTF-8 complete codepoint");c=(c<<6)|(*p++&63);}*s=(const char*)p;return c;}
static int px(TxtEstilo e){switch(e){case TXT_DET_SIN:return 26;case TXT_AJ_INSP:return 33;case TXT_ILHA_SUB:return 19;case TXT_PG_ROTULO:return 20;case TXT_SINOPSE_19:return 19;case TXT_SINOPSE_20:return 20;case TXT_SINOPSE_21:return 21;case TXT_SINOPSE_22:return 22;case TXT_SINOPSE_23:return 23;case TXT_SINOPSE_24:return 24;case TXT_SINOPSE_25:return 25;case TXT_SINOPSE_26:return 26;case TXT_SINOPSE_27:return 27;case TXT_SINOPSE_28:return 28;case TXT_SINOPSE_29:return 29;case TXT_SINOPSE_30:return 30;case TXT_SINOPSE_31:return 31;case TXT_SINOPSE_33:return 33;case TXT_SINOPSE_34:return 34;case TXT_SINOPSE_35:return 35;case TXT_SINOPSE_39:return 39;default:return 22;}}
int txt_largura(TxtEstilo e,const char*s){double w=0;while(*s){unsigned c=codepoint(&s);double a=.6;for(size_t i=0;i<sizeof advances/sizeof advances[0];i++)if(advances[i].cp==c){a=advances[i].advance;break;}w+=a*px(e);}return (int)ceil(w);}
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)r;(void)g;(void)b;(void)a;check(num_lines<4096,"bounded draw capture");int i=++num_lines;snprintf(lines[i-1].text,sizeof lines[i-1].text,"%s",s);lines[i-1].style=e;lines[i-1].h=(int)ceil(px(e)*1.22);lines[i-1].w=txt_largura(e,s);return (TxtLinha){.tex=i,.w=lines[i-1].w,.h=lines[i-1].h};}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float max){TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>max)t.w=(int)max;return t;}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a){(void)a;check(t.tex>0&&t.tex<=num_lines,"valid captured text handle");Line*l=&lines[t.tex-1];if(clip_on){check(x>=clip.x-.01f&&x+t.w<=clip.x+clip.w+1,"every synopsis line fits horizontal text viewport");check(y<clip.y+clip.h+1&&y+t.h>=clip.y-100,"draw only adjacent visible lines");drawn_count++;if(strstr(l->text,"477"))drawn_tail=1;}else{check(x>=0&&y>=0,"fixed labels begin on canvas");check((x+t.w)*active_scale<=NV_LAYOUT_REAL_W+1,"fixed labels end on canvas");check((y+t.h)*active_scale<=NV_LAYOUT_REAL_H+1,"fixed labels fit canvas height");}}
float txt_bloco(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float lead,float a,int n){(void)e;(void)s;(void)r;(void)g;(void)b;(void)x;(void)y;(void)w;(void)a;return lead*n;}
float txt_bloco_corta(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float lead,float a,int n){return txt_bloco(e,s,r,g,b,x,y,w,lead,a,n);}
void gfx_cor(GfxRect r,float radius,float red,float g,float b,float a){(void)r;(void)radius;(void)red;(void)g;(void)b;(void)a;}
void gfx_recorte(float x,float y,float w,float h){clip=(GfxRect){x,y,w,h};clip_on=1;check(w>0&&h>0,"positive reader viewport");}
void gfx_sem_recorte(void){clip_on=0;}
void plrui_material(GfxRect r,float radius,int modal,float a){(void)radius;(void)modal;(void)a;sheet=r;}
void plrui_linha_foco(GfxRect r,float radius,float a){(void)radius;(void)a;close_box=r;}
Uint32 SDL_GetTicks(void){return ticks;}
int SDL_GetNumTouchDevices(void){return 1;}
SDL_Window*SDL_GL_GetCurrentWindow(void){return NULL;}
SDL_Window*SDL_GetWindowFromID(Uint32 id){(void)id;return NULL;}
SDL_Window*SDL_GetMouseFocus(void){return NULL;}
Uint32 SDL_GetMouseState(int*x,int*y){if(x)*x=0;if(y)*y=0;return 0;}
void SDL_GetWindowSize(SDL_Window*w,int*x,int*y){(void)w;*x=(int)NV_LAYOUT_REAL_W;*y=(int)NV_LAYOUT_REAL_H;}
void SDL_GL_GetDrawableSize(SDL_Window*w,int*x,int*y){SDL_GetWindowSize(w,x,y);}
int SDL_ShowCursor(int q){(void)q;return 0;}
static void frame(void){num_lines=drawn_count=drawn_tail=0;clip_on=0;ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);ponteiro_quadro(ticks);sinopse_desenhar();ponteiro_desenhar();ticks+=100;total_frames++;check(active_scale==1,"renderer restores incoming scale");if(sinopse_aberta()){check(close_box.y>=clip.y+clip.h+23,"Close fixed below scroll viewport");check(close_box.x>=sheet.x&&close_box.x+close_box.w<=sheet.x+sheet.w,"Close fits sheet");}}
static void key(SDL_Keycode k){SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=k;sinopse_evento(&e);}
static void deliver(const SDL_Event*e){sinopse_evento(e);}
static void complete_cache(void){unsigned char seen[2048]={0};size_t len=strlen(leitor.texto);for(int i=0;i<leitor.nLinhas;i++){size_t start=leitor.linhas[i].inicio,n=leitor.linhas[i].n;check(start+n<=len,"cached line byte range belongs to copied metadata");for(size_t j=start;j<start+n;j++){check(!seen[j],"cached wrapped lines do not repeat text bytes");seen[j]=1;}}for(size_t j=0;j<len;j++)check(seen[j]||leitor.texto[j]==' '||leitor.texto[j]=='\r'||leitor.texto[j]=='\n',"every non-whitespace metadata byte survives wrapping");}
static void finger(float from,float to,float x){SDL_Event e;SDL_zero(e);e.type=SDL_FINGERDOWN;e.tfinger.touchId=7;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=from/NV_LAYOUT_REAL_H;ponteiro_evento(&e,deliver);for(int i=1;i<=8;i++){ticks+=50;e.type=SDL_FINGERMOTION;e.tfinger.y=(from+(to-from)*i/8)/NV_LAYOUT_REAL_H;ponteiro_evento(&e,deliver);}e.type=SDL_FINGERUP;ponteiro_evento(&e,deliver);ticks+=200;}
static void tap_close(void){float x=(close_box.x+close_box.w*.5f)*configured_scale,y=(close_box.y+close_box.h*.5f)*configured_scale;const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v),hit=ponteiro_achar(v,n,x,y);check(hit>=0&&v[hit].ativar==fecharPonteiro,"visible Close is top real pointer target");finger(y,y,x);check(!sinopse_aberta(),"real finger tap activates Close");}
static void fill(char*out,size_t n,const char*token){out[0]=0;while(strlen(out)+strlen(token)+strlen(tail)+2<n)strcat(out,token);strcat(out," ");strcat(out,tail);}
int main(int argc,char**argv){check(argc==5,"width height scale mobile args");int width=atoi(argv[1]),height=atoi(argv[2]),mobile=atoi(argv[4]);configured_scale=(float)atof(argv[3]);layout_tela_definir(width,height);layout_modo_definir(mobile);ponteiro_iniciar();ponteiro_teste_toque(1);char source[2000];const char*tokens[]={"A deliberately lengthy synopsis explains a distant destination and the people who discover it. ","éàç汉字日本語العربيةภาษาไทย 🧑‍🚀 ","WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"};for(font_size=0;font_size<3;font_size++)for(int kind=0;kind<3;kind++)for(translation=0;translation<2;translation++){fill(source,sizeof source,tokens[kind]);sinopse_abrir("A very long title that remains on one bounded line in every layout and translation",source);check(sinopse_aberta(),"nonempty metadata opens reader");frame();complete_cache();check(!strcmp(leitor.texto,source),"reader snapshots all available metadata bytes");float oldmax=leitor.max;key(SDLK_END);frame();check(leitor.scroll==leitor.max,"remote End reaches bottom");check(drawn_tail,"final synopsis sentence appears at bottom");GfxRect fixed=close_box;key(SDLK_HOME);frame();check(leitor.scroll==0,"remote Home returns to top");check(!memcmp(&fixed,&close_box,sizeof fixed),"scroll never moves Close");key(SDLK_DOWN);check(leitor.scroll==fminf(leitor.passo,leitor.max),"remote line scroll uses selected typography");key(SDLK_HOME);key(SDLK_PAGEDOWN);check(leitor.scroll==fminf(leitor.pagina,leitor.max),"remote page scroll reaches next viewport");
#ifdef NV_TOUCH_UI
if(mobile&&oldmax>0){key(SDLK_HOME);frame();for(int j=0;j<40&&leitor.scroll<leitor.max;j++){float x=(clip.x+clip.w*.5f)*configured_scale,from=(clip.y+clip.h*.8f)*configured_scale,to=(clip.y+clip.h*.2f)*configured_scale;finger(from,to,x);frame();}check(leitor.scroll==leitor.max,"actual finger swipes can reach full synopsis tail");frame();check(drawn_tail,"actual finger bottom shows final sentence");}
#endif
if(mobile)tap_close();else{key(SDLK_RETURN);check(!sinopse_aberta(),"remote OK closes reader");}sinopse_abrir("Copied title",source);source[0]='X';check(leitor.texto[0]!='X',"external metadata mutation does not replace open snapshot");session++;check(!sinopse_aberta(),"profile epoch change closes old reader");sinopse_abrir("Title",NULL);check(!sinopse_aberta(),"missing metadata does not open empty reader");sinopse_abrir("Title","");check(!sinopse_aberta(),"empty metadata does not open empty reader");}
fill(source,sizeof source,tokens[2]);font_size=2;sinopse_abrir("Rotation and active size change",source);frame();key(SDLK_END);frame();float old_scale=configured_scale;configured_scale=1.5f;layout_tela_definir(height,width);frame();complete_cache();check(leitor.scroll>=0&&leitor.scroll<=leitor.max,"rotation and UI rescale clamp current offset");key(SDLK_END);frame();check(drawn_tail,"rotated/rescaled reader still exposes final sentence");font_size=0;frame();complete_cache();key(SDLK_END);frame();check(drawn_tail,"changing typography rewraps without losing tail");configured_scale=old_scale;layout_tela_definir(width,height);frame();if(mobile)tap_close();else key(SDLK_ESCAPE);check(!sinopse_aberta(),"return from rotation preserves exit control");
char huge[5000];huge[0]=0;while(strlen(huge)+4<sizeof huge)strcat(huge,"🧑");sinopse_abrir(huge,huge);check(strlen(leitor.texto)<sizeof leitor.texto&&strlen(leitor.titulo)<sizeof leitor.titulo,"oversized metadata and title snapshots stay bounded");const char*cp=leitor.texto;while(*cp)codepoint(&cp);cp=leitor.titulo;while(*cp)codepoint(&cp);sinopse_fechar();
printf("PASS checks=%d frames=%d size_modes=3 text_shapes=3 translations=2 mobile=%d\n",checks,total_frames,mobile);return 0;}
