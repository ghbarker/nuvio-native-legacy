/* Actual full production ASS adapter and actual host libass software frames. */
#include <SDL2/SDL.h>
char *SDL_GetBasePath(void);
void SDL_free(void*);
#include "gl_compat.h"
void glDeleteTextures(GLsizei,const GLuint*);
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#include ASS_SOURCE
#include "video.h"
#include <assert.h>
static int checks, frames;
static void check(int ok,const char*why){checks++;if(!ok){fprintf(stderr,"FAIL %s\n",why);abort();}}
static void quiet(int level,const char*fmt,va_list args,void*u){(void)level;(void)fmt;(void)args;(void)u;}
static int image_color(int type,unsigned color){int changed=0,found=0;ASS_Image*i=ass_render_frame(assRenderer,assTrack,1000,&changed);frames++;for(;i;i=i->next)if((type<0||i->type==type)&&i->color==color&&i->w>0&&i->h>0)found++;return found;}
int main(int argc,char**argv){check(argc==2,"font path argument");
const char*script="[Script Info]\nScriptType: v4.00+\nPlayResX: 1920\nPlayResY: 1080\nYCbCr Matrix: None\n[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\nStyle: Default,Inter Display,56,&H00FFFFFF,&H00FFFFFF,&H00112233,&H00223344,0,0,0,0,100,100,0,0,1,2,4,2,130,130,100,1\n[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\nDialogue: 0,0:00:00.00,0:00:10.00,Default,,0,0,0,,Subtitle outline color\n";
assLib=ass_library_init();check(assLib!=NULL,"actual libass library initializes");ass_set_message_cb(assLib,quiet,NULL);assRenderer=ass_renderer_init(assLib);check(assRenderer!=NULL,"actual software renderer initializes");ass_set_frame_size(assRenderer,1920,1080);ass_set_storage_size(assRenderer,1920,1080);ass_set_fonts(assRenderer,argv[1],"Inter Display",ASS_FONTPROVIDER_NONE,NULL,1);
assTrack=ass_read_memory(assLib,(char*)script,strlen(script),"UTF-8");check(assTrack&&assTrack->n_styles,"actual document parser creates default style");PlainAssStyle p={0};strcpy(p.font,"Inter Display");p.size=56;p.rgb=0xffffff;p.marginV=100;p.outlineRgb=0xffffff;assTextoSimples=0;ass_plain_style_locked(&p);check(assTrack->styles[assTrack->default_style].OutlineColour==0x33221100u,"authored ASS AABBGGRR file value decodes to public libass RRGGBBAA and remains unchanged");check(image_color(IMAGE_TYPE_OUTLINE,0x33221100u)>0&&image_color(IMAGE_TYPE_SHADOW,0x44332200u)>0,"actual authored outline and shadow images preserve original file colors");assTextoSimples=1;
unsigned expected[6]={0x000000,0xffffff,0xffde30,0x40e070,0x4e9cff,0xff5050};
for(int index=0;index<6;index++)for(int border=1;border<=2;border++)for(int bg=0;bg<=4;bg++){
check(video_legenda_borda_rgb(index)==expected[index],"palette stable RGB definition including pure-black default");p.outlineRgb=(int)video_legenda_borda_rgb(index);p.border=border;p.background=bg;ass_plain_style_locked(&p);ASS_Style*s=&assTrack->styles[assTrack->default_style];
check(s->OutlineColour==(expected[index]<<8),"libass style receives RRGGBBAA with opaque outline alpha");check(s->PrimaryColour==0xffffff00u&&s->SecondaryColour==0xffffff00u,"outline preference leaves primary and secondary text colors unchanged");
unsigned back=bg?(unsigned)(255-bg*255*16/100):(border==2?expected[index]<<8:0)|46u;check(s->BackColour==back,"actual shadow RGB or original black background with unchanged alpha");check(s->BorderStyle==(bg?4:1)&&s->Outline==(border==1?2:0)&&s->Shadow==(border==2?4:0),"existing outline/shadow/background geometry preserved");
if(bg)check(image_color(-1,back)>0,"actual rasterized background box remains black with its existing alpha");
if(!bg){int type=border==1?IMAGE_TYPE_OUTLINE:IMAGE_TYPE_SHADOW;unsigned color=(expected[index]<<8)|(border==2?46u:0u);check(image_color(type,color)>0,"actual rasterized contour/shadow uses selected RGB without red-blue swapping");check(image_color(IMAGE_TYPE_CHARACTER,0xffffff00u)>0,"actual text image remains white");}
PlainAssStyle cached=plainApl;ass_plain_style_locked(&p);check(!memcmp(&cached,&plainApl,sizeof cached),"unchanged style retains cache key");}
check(video_legenda_borda_rgb(-1)==0&&video_legenda_borda_rgb(6)==0&&video_legenda_borda_rgb(100000)==0,"invalid palette indices safely fall back to pure black");
p.background=0;p.border=1;p.outlineRgb=0xff5050;ass_plain_style_locked(&p);check(image_color(IMAGE_TYPE_OUTLINE,0xff505000u)>0,"live red style rendered");p.outlineRgb=0x4e9cff;ass_plain_style_locked(&p);check(image_color(IMAGE_TYPE_OUTLINE,0x4e9cff00u)>0,"changing only outline color invalidates cache and renders blue");
ASS_Style*s=&assTrack->styles[assTrack->default_style];unsigned original=s->OutlineColour,originalBack=s->BackColour;assTextoSimples=0;p.outlineRgb=0xffffff;ass_plain_style_locked(&p);check(s->OutlineColour==original&&s->BackColour==originalBack,"authored ASS never receives generated-plain outline override");check(image_color(IMAGE_TYPE_OUTLINE,original)>0,"authored renderer keeps existing authored style color");
ass_free_track(assTrack);assTrack=NULL;ass_renderer_done(assRenderer);assRenderer=NULL;ass_library_done(assLib);assLib=NULL;printf("PASS actual-libass outline checks=%d software_frames=%d colors=6 border_modes=2 backgrounds=5\n",checks,frames);return 0;}
