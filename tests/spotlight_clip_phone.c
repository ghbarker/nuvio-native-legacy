/* Actual Spotlight list drawing after a Settings scene replaces its scissor.
 * The GL boundary models ajc_comecar/ajc_fim; no Settings actions or network. */
#define NV_SPOTLIGHT_DOUBLES_ONLY 1
#define NV_SPOTLIGHT_DRAW_HOOK clipDesenho
#define NV_SPOTLIGHT_PREVIEW_HOOK clipPrevia
#include "spotlight_retrato.c"

static int validarClip, emPrevia, desenhosLista, desenhosCortados, nPrevias;
static int desenhoSemClip, desenhosRodape;
static int desenhosMini;
static GfxRect janelaLista;

static void pertoClip(float a, float b) { assert(fabsf(a-b) < .03f); }
static void clipDesenho(GfxRect r) {
  if (emPrevia) {
    /* Even an inner scene's disabled scissor stays in the offscreen target. */
    if(validarClip) { assert(miniAtivaTeste);desenhosMini++; }
    return;
  }
  if (!validarClip) {
    if (!recortando) desenhoSemClip++;
    return;
  }
  assert(recortando);
  pertoClip(clip.x, janelaLista.x); pertoClip(clip.y, janelaLista.y);
  pertoClip(clip.w, janelaLista.w); pertoClip(clip.h, janelaLista.h);
  desenhosLista++;
  if (r.y < clip.y || r.y+r.h > clip.y+clip.h) desenhosCortados++;
  float x=fmaxf(r.x,clip.x), y=fmaxf(r.y,clip.y);
  float right=fminf(r.x+r.w,clip.x+clip.w), bottom=fminf(r.y+r.h,clip.y+clip.h);
  if (right>x && bottom>y) {
    assert(x>=janelaLista.x-.03f && right<=janelaLista.x+janelaLista.w+.03f);
    assert(y>=janelaLista.y-.03f && bottom<=janelaLista.y+janelaLista.h+.03f);
  }
}
static void clipPrevia(int op,float x,float y,float w,float h,float a) {
  (void)op; nPrevias++; emPrevia=1;
  gfx_cor((GfxRect){x,y,w,h},0,0,0,0,a);
  /* Production Settings scene uses a private clip, then ajc_fim disables it. */
  gfx_recorte(x,y,w,h);
  gfx_cor((GfxRect){x,y,w,h},0,0,0,0,a);
  gfx_sem_recorte(); emPrevia=0;
}
static void linhasClip(void) {
  nLin=12; float y=0;
  for(int i=0;i<nLin;i++) {
    int tipo=i==0?L_CAB:L_AJUSTE;
    lin[i]=(Linha){.tipo=tipo,.y=y,.h=i==1?194:ALTURA[tipo],.ref2=i==1};
    snprintf(lin[i].t1,sizeof lin[i].t1,"Second subtitle size and a long translated setting %d",i);
    snprintf(lin[i].t2,sizeof lin[i].t2,"Appearance / Secondary subtitles / Large text");
    if(i==1) strcpy(lin[i].base,"A setting preview followed by rows which must stay above the footer.");
    y+=lin[i].h; entraLin[i]=1; animLin[i]=i==3?1:0;
    snprintf(lin[i].icone,sizeof lin[i].icone,"aj_type");
  }
}
int main(void) {
  const float dims[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float zoom[]={1,1.2f,1.3f,1.5f}; int cases=0;
  for(int d=0;d<4;d++)for(int z=0;z<4;z++)for(int font=0;font<2;font++)for(int small=0;small<2;small++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];escalaUi=zoom[z];
    escalaAtiva=escala_min(SP_ESCALA_MIN);letrasLargas=font;
    spBW=SP_BW_BASE;kbAnim=0;kbAberto=0;entrada=1;aberto=1;painel=P_LISTA;focoL=3;
    listaX=SP_LISTA_X0;listaW=SP_LISTA_XF-listaX;corpoH=fminf(small?200:350,SP_CORPO_MAX);
    linhasClip(); toquerol_limpar(&toqueSpot); scrollY=0;
    for(int pos=0;pos<4;pos++)for(int move=0;move<2;move++)for(int fail=0;fail<2;fail++) {
      float dy=move?-12.0f:0.0f;
      float topo=SP_CORPO_Y+SP_CPAD_T+dy+spListaDesloc();
      float vis=corpoH-SP_CPAD_T-SP_CPAD_B-SP_RODAPE_H-spListaDesloc();
      scrollY=pos==0?0:pos==1?32:pos==2?140:fmaxf(0,alturaLista()-vis);
      miniFalhaTeste=fail;
      janelaLista=(GfxRect){listaX,topo-8,listaW,vis+4};
      validarClip=1;nPrevias=desenhosMini=desenhosLista=desenhosCortados=nAlvos=miniComposicoesTeste=0;
      desenhaLista(dy,1);validarClip=0;
      assert(!recortando && desenhosLista>0);
      if(pos<3) {
        assert(nPrevias==!fail && desenhosCortados>0);
        if(!fail) {
          assert(desenhosMini==2 && miniComposicoesTeste==1);
          if(pos==2) assert(miniCompostaTeste.y<janelaLista.y);
          if(small && pos==0) assert(miniCompostaTeste.y+miniCompostaTeste.h>janelaLista.y+janelaLista.h);
        } else assert(miniComposicoesTeste==0);
      }
      assert(!miniAtivaTeste && escalaAtiva==escala_min(SP_ESCALA_MIN));
      for(int i=0;i<nAlvos;i++) {
        assert(alvos[i].focar==focarLinha);
        assert(alvos[i].y>=topo-.03f && alvos[i].y+alvos[i].h<=topo+vis+.03f);
        assert(alvos[i].x>=listaX-.03f && alvos[i].x+alvos[i].w<=listaX+listaW+.03f);
      }
      int before=desenhoSemClip;desenhaRodape(dy,1);
      assert(!recortando && desenhoSemClip>before);desenhosRodape++;
      cases++;
    }
  }
  miniFalhaTeste=0;nv_layout_w=1920;nv_layout_h=1080;escalaUi=escalaAtiva=1;
  assert(!spTelefone());spBW=SP_BW_BASE;listaX=SP_LISTA_X0;listaW=SP_LISTA_XF-listaX;
  corpoH=350;scrollY=0;linhasClip();validarClip=0;desenhoSemClip=0;
  desenhaLista(0,1);assert(!recortando && desenhoSemClip>0);
  assert(desenhosRodape==cases);
  printf("spotlight_clip_phone: %d actual list/preview/label/icon/footer/target cases PASS; TV unchanged\n",cases);
  return 0;
}
