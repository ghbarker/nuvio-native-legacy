/* Actual Explore landing drawing, pointer capture, taps and TV recovery.
   Offline SDL/data doubles; native GL captures remain a separate check. */
#define EXPLORAR_REVIEW_POINTER_REAL 1
#define main explorar_original_draw_main
#include "explorar_desenho_review.c"
#undef main
#include "ponteiro_sdl.h"
#include "../src/ponteiro.c"

float gfx_opacidade_grupo=1;
static Uint32 relogioTeste=1000;
static int abriuClima=-1, teclasTeste, menusTeste;
static Uint32 relogioFixture(void){return relogioTeste;}
void mapa_clima_abrir(int id){abriuClima=id;}
void mapa_vizinhos_pedir(const MapaObra *o,long p,const char *t){(void)o;(void)p;(void)t;}
int mapa_vizinhos_copiar(MapaVizinhos *v,unsigned *r){(void)v;(void)r;return 0;}
void mapa_climas_pedir(void){}
int mapa_climas_copiar(MapaClimas *c,unsigned *r){(void)c;(void)r;return 0;}
int desc_titulo_buscando(void){return 0;}
void desc_pedir_titulo_tmdb(long id,const char*t){(void)id;(void)t;}
void desc_pedir_titulo(const char *id){(void)id;}
static void evento(const SDL_Event *e){if(e->type==SDL_KEYDOWN)teclasTeste++;explorar_evento(e);}
static void menuTeste(void){menusTeste++;}
static void quadro(void){
  relogioTeste+=16;ponteiro_quadro(relogioTeste);clipped=0;phase=0;
  desenharClimas();ponteiro_borda_esquerda(30,menuTeste);ponteiro_desenhar();
}
static void dedo(Uint32 tipo,float x,float y,int id){
  SDL_Event e={0};e.type=tipo;e.tfinger.touchId=7;e.tfinger.fingerId=id;
  e.tfinger.x=x/NV_TELA_W;e.tfinger.y=y/NV_TELA_H;
  assert(ponteiro_evento(&e,evento));
}
static void perto(float a,float b){assert(fabsf(a-b)<.1f);}
static void preparar(float w,float h,float fonte){
  nv_layout_w=w;nv_layout_h=h;fontWide=fonte;modo=MODO_CLIMAS;clFoco=0;
  entrada=focoT=1;clRolar=0;toqueCl=(ToqueRolagem){0};sair=pediuAbrir=0;
  toqueCaAtiva=toqueVzAtiva=-1;abriuClima=-1;teclasTeste=menusTeste=0;
  ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)w,(int)h);
  ponteiro_teste_relogio(relogioFixture);quadro();
}
static void arrastar(float x,float y,float dx,float dy){
  dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=100;dedo(SDL_FINGERMOTION,x+dx,y+dy,1);
  relogioTeste+=160;dedo(SDL_FINGERUP,x+dx,y+dy,1);quadro();
}
int main(void){
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  climas.n=MAPA_CLIMA_N;
  for(int i=0;i<climas.n;i++){
    MapaClima*c=&climas.c[i];c->id=i;c->n=6;c->total=123;c->vistos=42;c->afinidade=65;
    for(int j=0;j<c->n;j++)obra(&c->itens[j],j);
  }
  int casos=0;
  for(int t=0;t<4;t++)for(int f=0;f<3;f++){
    float w=telas[t][0],h=telas[t][1];preparar(w,h,1+.25f*f);
    assert(climaColunas()<EX_CL_COLS);
    GfxRect cards[MAPA_CLIMA_N];climaGradeTelefone(cards);
    assert(cards[0].w>(EX_DIR-ajustes_conteudo_x()-EX_CL_GAP*3)/4*1.3f);
    assert(cards[0].h>=384);casos++;
    float x=w*.6f,y=h-160,antes=clRolar;
    dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=100;
    dedo(SDL_FINGERMOTION,x,y-150,1);perto(clRolar,fminf(antes+150,toqueCl.maximo));
    quadro();relogioTeste+=100;dedo(SDL_FINGERMOTION,x,y-300,1);
    perto(clRolar,fminf(antes+300,toqueCl.maximo));assert(toqueCl.livre&&modo==MODO_CLIMAS);
    relogioTeste+=160;dedo(SDL_FINGERUP,x,y-300,1);quadro();
    assert(!teclasTeste&&abriuClima==-1);casos++;
    float mantido=clRolar;for(int i=0;i<40;i++)quadro();perto(clRolar,mantido);casos++;
    for(int i=0;i<10;i++)arrastar(x,h*.75f,0,-h*.5f);
    perto(clRolar,toqueCl.maximo);assert(!teclasTeste&&abriuClima==-1);casos++;
    const PonteiroAlvo*alvos;int n=ponteiro_teste_lista(&alvos),achou=0;
    for(int i=0;i<n;i++)if(alvos[i].focar==ponteiroClimas&&alvos[i].a==MAPA_CLIMA_N-1){
      float tx=alvos[i].x+alvos[i].w*.5f,ty=alvos[i].y+alvos[i].h*.5f;
      dedo(SDL_FINGERDOWN,tx,ty,1);relogioTeste+=20;dedo(SDL_FINGERUP,tx,ty,1);achou=1;break;
    }
    assert(achou&&modo==MODO_CLIMA&&abriuClima==MAPA_CLIMA_N-1);casos++;
    SDL_Event volta;memset(&volta,0,sizeof volta);volta.type=SDL_KEYDOWN;volta.key.keysym.sym=SDLK_ESCAPE;explorar_evento(&volta);
    assert(modo==MODO_CLIMAS&&clFoco==MAPA_CLIMA_N-1);quadro();perto(clRolar,mantido=toqueCl.maximo);casos++;
    SDL_Event cima;memset(&cima,0,sizeof cima);cima.type=SDL_KEYDOWN;cima.key.keysym.sym=SDLK_UP;explorar_evento(&cima);quadro();
    assert(!toqueCl.livre);climaGradeTelefone(cards);
    assert(cards[clFoco].y-clRolar>=EX_CL_TOPO-.1f);
    assert(cards[clFoco].y+cards[clFoco].h-clRolar<=NV_TELA_H-80+.1f);casos++;
    preparar(w,h,1+.25f*f);arrastar(w*.5f,h*.5f,150,0);
    assert(!teclasTeste&&abriuClima==-1&&clFoco==0);casos++;
    arrastar(15,h*.5f,150,0);assert(menusTeste==1&&!teclasTeste&&abriuClima==-1);casos++;
    preparar(w,h,1+.25f*f);dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=100;
    dedo(SDL_FINGERMOTION,x,y-150,1);mantido=clRolar;ponteiro_cancelar_toque();
    dedo(SDL_FINGERUP,x,y-150,1);quadro();perto(clRolar,mantido);
    assert(!teclasTeste&&abriuClima==-1);casos++;
  }
  preparar(1920,1080,1);assert(!telefoneui_ativo()&&climaColunas()==4);
  clFoco=0;eventoClimas(SDLK_DOWN);assert(clFoco==4);casos++;
  longWordFixture=1;preparar(1080,2340,1);
  assert(climaColunas()==1);GfxRect traduzidos[MAPA_CLIMA_N];climaGradeTelefone(traduzidos);
  assert(traduzidos[4].w-48>=txt_largura(TXT_TITULO3,"AdventureMysteryTimeline"));casos++;
  longWordFixture=0;
  const char *palavra="ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
  float altura=climaBlocoTelefone(TXT_TITULO3,palavra,255,255,255,104,212,824,58,1);
  assert(altura>58 && fullBlockGlyphs==glyphs(palavra));casos++;
  const char *utf8="ÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvoreÁrvore";
  altura=climaBlocoTelefone(TXT_TITULO3,utf8,255,255,255,104,212,200,58,1);
  assert(altura>58 && fullBlockGlyphs==glyphs(utf8));casos++;
  printf("explorar_landing_touch: %d actual pointer/geometry/tap/cancel/TV-recovery cases, %d violations\n",casos,failures);
  return failures?1:0;
}
