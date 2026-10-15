/* Only affected footer geometry/input checks after the144-base-unit change. */
#define main original_reader_matrix_main
#include "reader_probe.c"
#undef main
static GfxRect actual_target(void){const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);for(int i=n-1;i>=0;i--)if(v[i].ativar==fecharPonteiro)return (GfxRect){v[i].x,v[i].y,v[i].w,v[i].h};check(0,"reader registers real Close target");return (GfxRect){0};}
int main(int argc,char**argv){check(argc==5,"width height scale mobile args");int width=atoi(argv[1]),height=atoi(argv[2]),mobile=atoi(argv[4]);configured_scale=(float)atof(argv[3]);layout_tela_definir(width,height);layout_modo_definir(mobile);ponteiro_iniciar();ponteiro_teste_toque(1);char source[2000];fill(source,sizeof source,"A long synopsis fills the viewport without entering the footer reserved for accessible touch. ");
const float points[9][2]={{0,0},{.5f,0},{1,0},{0,.5f},{.5f,.5f},{1,.5f},{0,1},{.5f,1},{1,1}};
for(font_size=0;font_size<3;font_size++)for(int p=0;p<9;p++){
translation=p&1;sinopse_abrir("A sufficiently long title remains separate from its scrollable synopsis",source);frame();GfxRect t=actual_target();
if(mobile){check(t.h>=143.99f,"actual Mobile Close target has at least144base units");check(t.h*360.0f/1080.0f>=47.99f,"Close target projects to at least48dp on a360dp-short-edge phone");}
else check(fabsf(t.h-56*configured_scale)<.01f,"TV Close target retains original56virtual-unit height");
check(t.x>=0&&t.y>=0&&t.x+t.w<=NV_LAYOUT_REAL_W+.01f&&t.y+t.h<=NV_LAYOUT_REAL_H+.01f,"full actual Close target fits real canvas");
check(t.y>=(clip.y+clip.h+23.99f)*configured_scale,"entire touch target stays24virtual units below body viewport");
check(close_box.y*configured_scale>=t.y-.01f&&(close_box.y+close_box.h)*configured_scale<=t.y+t.h+.01f,"visual Close stays centered within actual larger target");
check(fabsf((close_box.y+close_box.h*.5f)*configured_scale-(t.y+t.h*.5f))<.01f,"visual Close center matches actual target center");
key(SDLK_END);frame();check(drawn_tail,"footer reserve preserves reachable final synopsis sentence");
if(mobile){float x=t.x+1+(t.w-2)*points[p][0],y=t.y+1+(t.h-2)*points[p][1];const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v),hit=ponteiro_achar(v,n,x,y);check(hit>=0&&v[hit].ativar==fecharPonteiro,"all four corners, four edges and center hit Close");finger(y,y,x);check(!sinopse_aberta(),"actual finger tap at target edge or corner closes reader");}
else {key(SDLK_RETURN);check(!sinopse_aberta(),"TV OK closes unchanged footer");}
}
font_size=2;sinopse_abrir("Touch body separation",source);frame();
#ifdef NV_TOUCH_UI
if(mobile&&leitor.max>0){float x=(clip.x+clip.w*.5f)*configured_scale,y=(clip.y+clip.h*.8f)*configured_scale;finger(y,y-clip.h*.6f*configured_scale,x);frame();check(sinopse_aberta()&&leitor.scroll>0,"body swipe scrolls instead of accidentally triggering larger Close");}
#endif
sinopse_fechar();printf("PASS Close refinement checks=%d frames=%d edge_points=9 size_modes=3 mobile=%d\n",checks,total_frames,mobile);return 0;}
