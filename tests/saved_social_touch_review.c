/* Production Social controls cross real pointer DOWN/redraw/UP events.
 * All selected account/request operations end at the builder's inert spies. */
#define NV_SAVED_SOCIAL_ACTIONS_DOUBLES_ONLY 1
#define ponteiro_ativo reviewCaptureActive
#define ponteiro_alvo reviewCaptureTarget
#define ponteiro_alvo_faixa reviewCaptureRange
#define ponteiro_camada reviewCaptureLayer
#define ponteiro_rolagem reviewCaptureScroll
#include "saved_social_phone.c"
#undef ponteiro_ativo
#undef ponteiro_alvo
#undef ponteiro_alvo_faixa
#undef ponteiro_camada
#undef ponteiro_rolagem
#define SDL_GetTicks reviewUnusedTicks
#include "ponteiro_sdl.h"
#undef SDL_GetTicks
float gfx_opacidade_grupo=1;
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W nv_layout_w
#define NV_TELA_H nv_layout_h
#define lista reviewPointerList
#define visivel reviewPointerVisible
#include "../src/ponteiro.c"
#undef lista
#undef visivel

static Uint32 reviewNow=5000;
static int reviewKeys;
static Uint32 reviewClock(void) { return reviewNow; }
static void reviewDeliver(const SDL_Event *e) { (void)e;reviewKeys++; }
static void reviewFrame(int tipo,int clipped) {
  escala=1;ponteiro_quadro(reviewNow);escala=zoom;
  socialDesenhar(tipo,clipped);
  for(int i=0;i<nBotoesTeste;i++) {
    GfxRect r=botoesTeste[i];
    ponteiro_alvo(r.x,r.y,r.w,r.h,alvoFocar[i],alvoAtivar[i],alvoA[i],alvoB[i]);
  }
  escala=1;ponteiro_desenhar();reviewNow+=16;
}
static PonteiroAlvo reviewTarget(int row,int column) {
  const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
  for(int i=0;i<n;i++)if(v[i].ativar==toqueSocialAtivar&&v[i].a==row&&v[i].b%3==column)return v[i];
  assert(!"missing actual Social control");return (PonteiroAlvo){0};
}
static void reviewEvent(Uint32 type,PonteiroAlvo t,int mouse,float dy) {
  SDL_Event e;SDL_zero(e);e.type=type;float x=t.x+t.w*.5f,y=t.y+t.h*.5f+dy;
  if(mouse){e.button.button=SDL_BUTTON_LEFT;e.button.x=(int)x;e.button.y=(int)y;}
  else{e.tfinger.touchId=3;e.tfinger.fingerId=1;e.tfinger.x=x/nv_layout_w;e.tfinger.y=y/nv_layout_h;}
  assert(ponteiro_evento(&e,reviewDeliver));
}
static void reviewInit(int tipo) {
  socialPreparar(tipo);escala=1;reviewKeys=0;
  ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_relogio(reviewClock);
  ponteiro_teste_janela((int)nv_layout_w,(int)nv_layout_h);
}
static void reviewTap(int tipo,int row,int column,int mouse,int clipped) {
  reviewFrame(tipo,clipped);PonteiroAlvo t=reviewTarget(row,column);
  reviewEvent(mouse?SDL_MOUSEBUTTONDOWN:SDL_FINGERDOWN,t,mouse,0);
  reviewFrame(tipo,clipped);reviewFrame(tipo,clipped);
  reviewEvent(mouse?SDL_MOUSEBUTTONUP:SDL_FINGERUP,t,mouse,0);
  assert(!reviewKeys);
}
int main(void) {
  const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float zooms[]={1,1.2f,1.3f,1.5f};int cases=0;
  for(int s=0;s<4;s++)for(int z=0;z<4;z++) {
    nv_layout_w=screens[s][0];nv_layout_h=screens[s][1];zoom=zooms[z];
    for(int mouse=0;mouse<2;mouse++) {
      for(int c=0;c<2;c++)for(int clip=0;clip<2;clip++) {
        reviewInit(SPS_PEDIDO);reviewTap(SPS_PEDIDO,0,c,mouse,clip);
        assert(aceitouTeste==(c==0)&&recusouTeste==(c==1)&&!strcmp(ultimoPedido,"fixture-person"));cases++;
      }
      for(int c=0;c<3;c++) {
        reviewInit(SPS_ALCANCE);reviewTap(SPS_ALCANCE,0,c,mouse,0);
        assert(alcCol==c&&alcanceChamadas==(c!=0));if(c)assert(ultimoAlcance==c);cases++;
      }
      for(int servico=0;servico<3;servico++)for(int linked=0;linked<2;linked++) {
        reviewInit(SPS_IDENT);nSocial=3;social[0]=(SPSocial){SPS_IDENT,0};
        social[1]=(SPSocial){SPS_SIMKL,0};social[2]=(SPSocial){SPS_LETTERBOXD,0};
        if(linked)identidadeTeste[servico]=servico?REC_IDENT_E_LIGADO:REC_IDENT_UNIDA;
        reviewTap(SPS_IDENT,servico,0,mouse,0);
        if(linked) {
          assert(!contaChamadas[servico]&&identConfirma==social[servico].tipo+1);
          reviewTap(SPS_IDENT,servico,0,mouse,0);assert(contaChamadas[servico]==1);
        }else if(servico<2)assert(contaChamadas[servico]==1);
        else assert(tecladoChamadas==1&&tecladoPara==TK_LETTERBOXD);
        cases++;
      }
    }
    /* A fresh draw after model/eligibility changes cannot inherit the press. */
    for(int change=0;change<6;change++) {
      reviewInit(SPS_PEDIDO);reviewFrame(SPS_PEDIDO,0);PonteiroAlvo t=reviewTarget(0,1);
      reviewEvent(SDL_FINGERDOWN,t,0,0);
      if(change==0)snprintf(peds[0].pub,sizeof peds[0].pub,"other-person");
      else if(change==1)perfilTeste=2;
      else if(change==2)aberto=0;
      else if(change==3)pop=POP_SOCIAL;
      else if(change==4)tecladoTeste=1;
      else {SDL_Event e;SDL_zero(e);e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_SIZE_CHANGED;ponteiro_evento(&e,reviewDeliver);}
      reviewFrame(SPS_PEDIDO,0);reviewEvent(SDL_FINGERUP,t,0,0);
      assert(!aceitouTeste&&!recusouTeste&&!reviewKeys);
    }
    reviewInit(SPS_ALCANCE);reviewFrame(SPS_ALCANCE,0);PonteiroAlvo t=reviewTarget(0,2);
    reviewEvent(SDL_FINGERDOWN,t,0,0);privacidadeTeste=1;reviewFrame(SPS_ALCANCE,0);
    reviewEvent(SDL_FINGERUP,t,0,0);assert(!alcanceChamadas&&!reviewKeys);
    reviewInit(SPS_LETTERBOXD);reviewFrame(SPS_LETTERBOXD,0);t=reviewTarget(0,0);
    reviewEvent(SDL_FINGERDOWN,t,0,0);operacaoTeste[2]=REC_IDENT_OP_INDO;reviewFrame(SPS_LETTERBOXD,0);
    reviewEvent(SDL_FINGERUP,t,0,0);assert(!tecladoChamadas&&!contaChamadas[2]&&!reviewKeys);
    reviewInit(SPS_PEDIDO);reviewFrame(SPS_PEDIDO,0);t=reviewTarget(0,1);
    reviewEvent(SDL_FINGERDOWN,t,0,0);reviewEvent(SDL_FINGERMOTION,t,0,-50);
    reviewFrame(SPS_PEDIDO,0);reviewEvent(SDL_FINGERUP,t,0,-50);
    assert(!aceitouTeste&&!recusouTeste&&!reviewKeys);
  }
  printf("Saved Social real pointer: %d redraw-spanning decision cases, confirmation, model/modal/rotation/drag cancellation PASS\n",cases);
  return 0;
}
