#include RENDER_SOURCE
#include <assert.h>
#include <math.h>

// Actual Settings resolver + actual wrap/Bidi/draw functions. The only display
// boundaries replaced here are deterministic text metrics/textures and GPU calls.
void fixture_secondary(int,int,int,int,int,int,int,int,int);
static VideoLegendaEstilo primary = {120,0,0,3,1,0,0,TXT_FAMILIA_INTER,0,0};
static LegendaCue cue;
static int hasCue=1, island;
typedef struct { TxtEstilo style; int r,g,b,family,emphasis; char text[768]; } Texture;
static Texture textures[2048];
static int nt, nd, nbg, nbidi;
static int bidiFamily;
static struct { unsigned tex; float x,y,alpha; } drawn[2048];
static GfxRect backgrounds[64];
static float backgroundAlpha[64];

VideoLegendaEstilo *player_leg_estilo(void) { return &primary; }
int legenda2_cues(double pos, LegendaCue *out, int max) {
  (void)pos;
  if (!hasCue || max < 1) return 0;
  out[0]=cue;
  return 1;
}
int plrilha_rect(GfxRect *r) { *r=(GfxRect){1200,0,650,120}; return island; }
float gfx_escala_ui(void) { return 1; }
int txt_bidi_legenda(TxtFamilia family, TxtEstilo style, const char *in, char *out, size_t n) {
  (void)style; nbidi++; bidiFamily=family;
  return bidi_visual_utf8(in,out,n);
}
TxtLinha txt_linha_corta_enfase(TxtEstilo style,const char *text,int r,int g,int b,int a,float maxW,TxtFamilia family,int emphasis) {
  (void)a;
  assert(nt+1 < 2048);
  Texture *t=&textures[++nt];
  *t=(Texture){.style=style,.r=r,.g=g,.b=b,.family=family,.emphasis=emphasis};
  snprintf(t->text,sizeof t->text,"%s",text);
  int h=20+(style-TXT_LEG_50)*2;
  int w=(int)strlen(text)*(h/2);
  if(w>maxW)w=(int)maxW;
  return (TxtLinha){.tex=(unsigned)nt,.w=w,.h=h,.pw=w,.ph=h};
}
void txt_desenhar_alpha(TxtLinha line,float x,float y,float alpha) {
  assert(nd<2048); drawn[nd].tex=line.tex; drawn[nd].x=x; drawn[nd].y=y; drawn[nd++].alpha=alpha;
}
void gfx_cor(GfxRect r,float radius,float cr,float cg,float cb,float ca) {
  (void)radius; assert(cr==0 && cg==0 && cb==0); assert(nbg<64);
  backgrounds[nbg]=r; backgroundAlpha[nbg++]=ca;
}
static void reset_capture(void) { nt=nd=nbg=nbidi=0; }
static int foreground_index(void) { assert(nd>0); return nd-1; }
static void report_legacy(int stacked,int osd,int letterbox,int useIsland) {
  LegendasGeo g={0,letterbox?130:0,1920,letterbox?820:1080,(float)osd,.7,10,stacked,980};
  fixture_secondary(0,0,0,0,0,0,0,0,0);
  primary=(VideoLegendaEstilo){120,1,2,3,1,0,2,TXT_FAMILIA_DROID,1,4};
  cue=(LegendaCue){0}; snprintf(cue.texto,sizeof cue.texto,"one line\ntwo lines"); cue.italico=1;
  island=useIsland; reset_capture();
  legendasui_desenhar_secundaria(&g);
  float h=legendasui_altura_secundaria(&g);
  printf("LEGACY %d %d %d %d h=%.3f band=%.3f draws=%d bg=%d",stacked,osd,letterbox,useIsland,h,legendasui_topo_livre(0),nd,nbg);
  for(int i=0;i<nd;i++) {
    Texture *t=&textures[drawn[i].tex];
    printf(" | %d,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%s",t->style,t->family,t->emphasis,t->r,t->g,t->b,drawn[i].x,drawn[i].y,drawn[i].alpha,t->text);
  }
  puts("");
}
static float position_offset(int p) { return p>=3?-(p-3)*48.f:(3-p)*20.f; }

int main(void) {
  for(int stacked=0;stacked<2;stacked++) for(int osd=0;osd<2;osd++)
    for(int letterbox=0;letterbox<2;letterbox++) for(int pill=0;pill<2;pill++)
      report_legacy(stacked,osd,letterbox,pill);
#ifndef BASELINE
  primary=(VideoLegendaEstilo){120,0,0,3,1,0,0,TXT_FAMILIA_INTER,1,0};
  LegendasGeo g={0,0,1920,1080,0,.5,10,0,1000};
  cue=(LegendaCue){0}; snprintf(cue.texto,sizeof cue.texto,"independent");
  // Explicit regular must remove the primary bold preference, while cue-level
  // bold/italic remains authoritative for semantic emphasis.
  fixture_secondary(26,3,5,3,7,4,6,2,0); reset_capture(); island=0;
  legendasui_desenhar_secundaria(&g);
  int f=foreground_index(); Texture *t=&textures[drawn[f].tex];
  assert(t->style==TXT_LEG_50+25 && t->family==TXT_FAMILIA_JETBRAINS);
  assert(t->r==64 && t->g==224 && t->b==112 && t->emphasis==0);
  assert(fabsf(drawn[f].alpha-.125f)<.0001f && nbg==1 && fabsf(backgroundAlpha[0]-.08f)<.0001f);
  assert(nd==2); // own shadow overrides the primary outline
  Texture *outline=&textures[drawn[0].tex];
  assert(outline->r==255 && outline->g==80 && outline->b==80);
  assert(fabsf(drawn[0].x-drawn[f].x-4.f)<.0001f);
  cue.negrito=1; cue.italico=1; reset_capture(); legendasui_desenhar_secundaria(&g);
  t=&textures[drawn[foreground_index()].tex]; assert(t->emphasis==(TXT_ENF_NEGRITO|TXT_ENF_ITALICO));
  cue.negrito=cue.italico=0;
  fixture_secondary(1,1,1,1,2,1,1,1,0); reset_capture(); legendasui_desenhar_secundaria(&g);
  t=&textures[drawn[foreground_index()].tex];
  assert(t->family==TXT_FAMILIA_LG && t->style==TXT_LEG_50 && t->emphasis==TXT_ENF_NEGRITO);
  assert(nd==1 && nbg==0 && fabsf(drawn[0].alpha-.5f)<.0001f);
  snprintf(cue.texto,sizeof cue.texto,"שלום"); reset_capture(); legendasui_desenhar_secundaria(&g);
  t=&textures[drawn[foreground_index()].tex];
  char visual[768]; assert(bidi_visual_utf8(cue.texto,visual,sizeof visual)>0);
  assert(nbidi>0 && bidiFamily==TXT_FAMILIA_LG && strcmp(visual,cue.texto) && !strcmp(t->text,visual));
  puts("PARITY independent family/size/color/opacity/background/border/outline/bold and cue emphasis/Bidi: PASS");

  // Every secondary position must depend on its own value, not the primary's.
  // The safe neutral anchors represent hidden OSD, visible OSD, next card and
  // mobile transport. The app supplies these through baseLegendaPrincipal().
  snprintf(cue.texto,sizeof cue.texto,"position");
  float anchors[]={1000,760,690,620};
  for(unsigned a=0;a<sizeof anchors/sizeof anchors[0];a++) for(int p=0;p<8;p++) for(int second=0;second<8;second++) {
    primary.posicao=p;
    g.baseY=anchors[a]+position_offset(p);
    fixture_secondary(0,0,0,0,0,0,0,0,second+1);
    reset_capture(); legendasui_desenhar_secundaria(&g);
    int last=foreground_index(); t=&textures[drawn[last].tex];
    float h=20+(t->style-TXT_LEG_50)*2;
    assert(fabsf(drawn[last].y+h-(anchors[a]+position_offset(second)))<.0001f);
    assert(legendasui_altura_secundaria(&g)==0 && legendasui_topo_livre(0)==0);
  }
  g.baseY=0; reset_capture(); legendasui_desenhar_secundaria(&g); assert(nd==0);
  g.baseY=1000; g.videoW=500; reset_capture(); legendasui_desenhar_secundaria(&g); assert(nd==0);
  g.videoW=1920; hasCue=0; reset_capture(); legendasui_desenhar_secundaria(&g); assert(nd==0);
  puts("PARITY 256 primary/secondary position pairs across 4 real safe-anchor classes, no stacking reservation, missing anchor/mini/empty guarded: PASS");
#endif
  return 0;
}
