/* Extend the real See all drawing fixture to ordinary and non-director grids. */
#define VT_TESTE_PONTEIRO 1
#define main vertudo_director_existing_main
#include "vertudo_retrato.c"
#undef main
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W nv_layout_w
#define NV_TELA_H nv_layout_h
#define ponteiro_ativo real_ponteiro_ativo
#define ponteiro_alvo real_ponteiro_alvo
#define ponteiro_alvo_faixa real_ponteiro_alvo_faixa
#define ponteiro_rolagem real_ponteiro_rolagem
#include "../src/ponteiro.c"
#undef ponteiro_ativo
#undef ponteiro_alvo
#undef ponteiro_alvo_faixa
#undef ponteiro_rolagem
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W (nv_layout_w / vtEscala())
#define NV_TELA_H (nv_layout_h / vtEscala())

static Uint32 relogioTeste=500;
float gfx_opacidade_grupo=1;
Uint32 SDL_GetTicks(void) { return relogioTeste; }
void *SDL_memset(void *dst,int c,size_t n) { return memset(dst,c,n); }
SDL_Window *SDL_GetWindowFromID(Uint32 id) { (void)id;return NULL; }
SDL_Window *SDL_GetMouseFocus(void) { return NULL; }
void SDL_GetWindowSize(SDL_Window *w,int *x,int *y) { (void)w;*x=(int)nv_layout_w;*y=(int)nv_layout_h; }
Uint32 SDL_GetMouseState(int *x,int *y) { if(x)*x=0;if(y)*y=0;return 0; }
static void quadro(void) {
  ponteiro_quadro(relogioTeste);nAlvos=nCards=nPainel=0;cortando=0;
  vertudo_desenhar(500);ponteiro_desenhar();
}
static int retornos;
static void entregar(const SDL_Event *e) { if(e->type==SDL_KEYDOWN&&e->key.keysym.sym==SDLK_RETURN)retornos++;vertudo_evento(e); }
static void dedo(Uint32 tipo,float x,float y) {
  SDL_Event e={0};e.type=tipo;e.tfinger.touchId=1;e.tfinger.fingerId=1;
  e.tfinger.x=x/nv_layout_w;e.tfinger.y=y/nv_layout_h;
  relogioTeste+=30;assert(ponteiro_evento(&e,entregar));
}
static PonteiroAlvo alvoCartaz(int item) {
  const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
  for(int i=0;i<n;i++)if(v[i].focar==ponteiroCartaz&&v[i].a==item)return v[i];
  assert(0);return (PonteiroAlvo){0};
}
static void gestoEToque(void) {
  int c=VT_COLS-1;PonteiroAlvo alvo=alvoCartaz(c);
  const char *arte;GfxRect cell=celulaRect(c,&item,&arte);
  assert(fabsf(alvo.x-cell.x*ui)<.01f && fabsf(alvo.w-cell.w*ui)<.01f);
  foco=0;pedAbrir=-1;retornos=adicionados=0;
  float x=alvo.x+alvo.w*.5f,y=alvo.y+fminf(50,alvo.h*.5f);
  dedo(SDL_FINGERDOWN,x,y);quadro();
  dedo(SDL_FINGERMOTION,x,y-37.5f*ui);quadro();
  dedo(SDL_FINGERUP,x,y-37.5f*ui);
  assert(fabsf(scrollY-37.5f)<.01f && foco==0 && pedAbrir==-1 && !retornos && !adicionados);
  ponteiro_cancelar_toque();quadro();alvo=alvoCartaz(c);
  x=alvo.x+alvo.w*.5f;y=alvo.y+fminf(50,alvo.h*.5f);
  dedo(SDL_FINGERDOWN,x,y);quadro();dedo(SDL_FINGERUP,x,y);
  assert(retornos==1 && foco==c && adicionados==1 && vertudo_pediu_abrir()==1100+c);
}

int main(void) {
  anim_politica_reduzida=1;
  ponteiro_teste_toque(1);
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  strcpy(item.titulo,"A very long ordinary title and extended catalog metadata");
  strcpy(item.meta,"2024");strcpy(item.genero,"Drama science fiction thriller");
  strcpy(item.sinopse,"Long synopsis with enough words to exercise the actual bounded detail panel.");
  strcpy(item.poster,"poster");strcpy(item.backdrop,"art");
  aberta=1;timeline=0;anim=1;orderN=-1;ondaArmada=ondaEm=0;tabFocus=0;
  int pages=0;
  const float escalas[]={1,1.2f,1.3f,1.5f};
  for(int d=0;d<4;d++)for(int r=0;r<2;r++)for(int f=0;f<2;f++)for(int family=0;family<4;family++)for(int u=0;u<4;u++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];rail=r?140:0;fonte=f?30:12;
    ui=escalas[u];escalaAtiva=1.17f;
    strcpy(titulo,"A long collection or ordinary addon catalog heading");
    collection=family?&pasta:NULL;
    strcpy(pasta.group,family==1?"Genres":family==2?"Streaming":"Awards");
    strcpy(pasta.title,"A collection with a longer title");pasta.nSources=0;
    ranked=family==3;source=tabCursor=0;tabRol=0;
    for(int end=0;end<2;end++) {
      foco=end?15:0;scrollY=end?toqueVertudoMax():0;nAlvos=nCards=nPainel=0;cortando=0;
      ponteiro_cancelar_toque();ponteiro_teste_janela((int)nv_layout_w,(int)nv_layout_h);
      quadro();assert(nAlvos>0 && !cortando);pages++;
      assert(nPainel==0 && escalaAtiva==1.17f);
      assert(fabsf(NV_TELA_W-dims[d][0]/ui)<.01f);
      assert(fabsf(vtInicio()*ui-(48*ui+rail))<.01f && fabsf(vtFim()-(NV_TELA_W-48))<.01f);
      if(d<2)assert(VT_COLS==3);
      assert(fabsf(vtInicio()+VT_COLS*VT_CARD_W+(VT_COLS-1)*VT_GAP_X-vtFim())<.01f);
      for(int i=0;i<nAlvos;i++)if(alvos[i].focar==ponteiroCartaz) {
        const char *arte;GfxRect cell=celulaRect(alvos[i].a,&item,&arte);
        assert(fabsf(cell.x-alvos[i].x)<.01f&&fabsf(cell.w-alvos[i].w)<.01f);
        float top=fmaxf(cell.y,VT_TOPO-12),bottom=fminf(cell.y+cell.h+40,NV_TELA_H);
        assert(fabsf(top-alvos[i].y)<.01f&&fabsf(bottom-top-alvos[i].h)<.01f);
      }
      /* Targets are visible, and focusing a visible card preserves the route. */
      int found=0;
      for(int i=0;i<nAlvos;i++)if(alvos[i].focar==ponteiroCartaz){
        alvos[i].focar(alvos[i].a,alvos[i].b);assert(foco==alvos[i].a);found=1;break;
      }
      assert(found);
    }
  }
  int gestos=0;
  for(int d=0;d<4;d++)for(int u=0;u<2;u++)for(int r=0;r<2;r++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];ui=u?1.5f:1;rail=r?140:0;
    collection=NULL;timeline=ranked=tabFocus=0;foco=0;scrollY=0;toqueLivre=0;
    ponteiro_cancelar_toque();ponteiro_teste_janela((int)nv_layout_w,(int)nv_layout_h);
    quadro();gestoEToque();gestos++;
    /* Event/focus and update run outside the drawing scale, even when nested. */
    escalaAtiva=1.17f;SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_DOWN;
    int antes=foco;vertudo_evento(&e);assert(foco==fminf(antes+VT_COLS,total-1));
    vertudo_atualizar(1.0f/60,relogioTeste);assert(scrollY>=0&&scrollY<=toqueVertudoMax()+.01f);
  }
  testMobile=0;
  for(int h=0;h<2;h++)for(int u=0;u<2;u++)for(int r=0;r<2;r++) {
    nv_layout_w=1920;nv_layout_h=h?1200:1080;ui=u?1.5f:1;rail=r?140:0;
    collection=NULL;timeline=ranked=tabFocus=0;foco=0;scrollY=0;toqueLivre=0;
    quadro();assert(!vtTelefone()&&nPainel==1&&VT_CARD_W==248&&VT_CARD_H==372);
    assert(VT_COLS==(r?4:5)&&vtInicio()==104+rail&&vtFim()==1456);
  }
  testMobile=1;
  printf("vertudo_grid_review: %d phone grid pages, %d real pointer drag/tap/event/update cases, original TV/tablet panel preserved OK\n",pages,gestos);
}
