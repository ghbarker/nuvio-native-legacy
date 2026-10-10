/* Real pointer events and Home's capture/snap; no GL, network or playback. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#define main home_identity_fixture_main
#include "heroidentidade_home.c"
#undef main
#include "ponteiro_sdl.h"
#include "../src/ponteiro.c"

float nv_layout_w = 1080, nv_layout_h = 2340;
static CatItem itensTeste[6];
int cat_n(void) { return 6; }
int cat_n_fileiras(void) { return 0; }
const CatFileira *cat_fileira(int i) { (void)i; return NULL; }
const CatItem *cat_item(int i) { return i>=0 && i<6 ? &itensTeste[i] : NULL; }
void cat_definir_tudo(const CatItem *i, int n, const CatFileira *f, int nf) {
  (void)f; (void)nf; assert(n==6); memcpy(itensTeste,i,sizeof itensTeste);
}
static int layoutTeste;
int ajustes_home_layout(void) { return layoutTeste; }
int ajustes_hero_ligado(void) { return 1; }
int ajustes_largura_poster_dp(void) { return 126; }
float ajustes_conteudo_x(void) { return 104; }
float ajustes_rail_largura_fixa(void) { return 0; }
int ajustes_posteres_deitados(void) { return 0; }
int ajustes_rotulos_poster(void) { return 1; }
int ajustes_borda_foco(void) { return 1; }
float ajustes_espaco_fileiras(void) { return 1; }
float ajustes_espaco_titulos(void) { return 1; }
int ajustes_hero_fonte(void) { return 0; }
int ajustes_hero_arte_diferente(void) { return 0; }
int ajustes_cw_thumb_episodio(void) { return 0; }
unsigned fil_revisao(void) { return 1; }
const char *fil_hero_fonte(void) { return ""; }
int amigosfil_indice_cat(int i) { (void)i; return -1; }
int amigosfil_rolagem(const PonteiroRolagem *e) { (void)e; return 0; }
float fil_tipo_fator(int t) { (void)t; return 1; }
const char *artehero_url_escolhida(const CatItem *c) { (void)c; return NULL; }
const char *artehero_url_destaque(const CatItem *c, int f, int d) { (void)f; (void)d; return c->backdrop; }
const char *artehero_url_episodio(const CatItem *c) { (void)c; return NULL; }
const char *artehero_url_metahub_fundo(const CatItem *c) { return c->backdrop; }
const char *artehero_url_card_fonte(const CatItem *c, int f, int d) { (void)f; (void)d; return c->backdrop; }
const char *posterprov_card_addon(const char *o,const char *id,long t,const char *tipo,const char *u) {
  (void)o; (void)id; (void)t; (void)tipo; return u;
}

float gfx_opacidade_grupo = 1;
void gfx_sem_recorte(void) {}
float gfx_escala(void) { return 1; }
float gfx_escala_ui(void) { return 1; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)r; (void)raio; (void)cr; (void)cg; (void)cb; (void)ca;
}
static Uint32 relogioTeste = 1000;
static int teclas, focos, menus;
static Uint32 agora(void) { return relogioTeste; }
static void receber(const SDL_Event *e) { if (e->type == SDL_KEYDOWN) teclas++; }
static void focoHeroTeste(int a, int b) { (void)a; (void)b; focos++; }
static void menuTeste(void) { menus++; }
static void publicar(void) {
  ponteiro_quadro(relogioTeste);
  ponteiro_alvo(0, 0, NV_TELA_W, 600, toqueHero.estado ? NULL : focoHeroTeste, NULL, 0, 0);
  ponteiro_rolagem(toqueHomeRolar);
  ponteiro_borda_esquerda(30, menuTeste);
  ponteiro_desenhar();
}
static void dedo(Uint32 tipo, Sint64 id, float x, float y) {
  SDL_Event e = {0}; e.type = tipo; e.tfinger.touchId = 7; e.tfinger.fingerId = id;
  e.tfinger.x = x / NV_TELA_W; e.tfinger.y = y / NV_TELA_H;
  assert(ponteiro_evento(&e, receber));
}
static void passo(float ms) {
  relogioTeste += (Uint32)ms;
  toqueHeroAtualizar(ms / 1000, relogioTeste, 0, 0);
  publicar();
}
static void terminar(void) { for (int i = 0; i < 30; i++) passo(16); assert(!toqueHero.estado); }
static void preparar(float w, float h, int pos) {
  nv_layout_w = w; nv_layout_h = h;
  ponteiro_iniciar(); ponteiro_teste_toque(1); ponteiro_teste_janela((int)w, (int)h);
  ponteiro_teste_relogio(agora);
  toqueHeroCancelar(); toqueHomeLimpar(); nFileiras = 1;
  memset(scrollX,0,sizeof scrollX);
  fileiras[0] = (Fileira){.tipo=FILEIRA_NORMAL,.n=4,.escala=1};
  foco.fileira = 0; focoHero = 1; scrollY = -empurraHero();
  heroAtual = heroAnterior = heroPendente = heroSet[pos];
  heroDesejado = -1; heroDesliza = 1; heroSai = 0; heroEntra = 1;
  heroArteRect = (GfxRect){0,0,w,600};
  heroSetRev = 1; heroSetFilRev = fil_revisao(); heroSetCat = cat_n(); heroSetNFil = nFileiras;
  teclas = focos = menus = 0; publicar();
}
static void moverDedo(float x0, float x1, int ms) {
  dedo(SDL_FINGERDOWN,1,x0,300); relogioTeste += ms;
  dedo(SDL_FINGERMOTION,1,x1,300);
}
static void perto(float a, float b) { if(fabsf(a-b)>=.1f)fprintf(stderr,"layout%d %.1fx%.1f: actual %.3f expected %.3f\n",layoutTeste,nv_layout_w,nv_layout_h,a,b); assert(fabsf(a-b) < .1f); }

int main(void) {
  static CatItem itens[6];
  for (int i = 0; i < 6; i++) {
    snprintf(itens[i].imdb,sizeof itens[i].imdb,"tt90000%d",i);
    snprintf(itens[i].tipo,sizeof itens[i].tipo,"movie");
    snprintf(itens[i].titulo,sizeof itens[i].titulo,"Title %d",i);
    snprintf(itens[i].backdrop,sizeof itens[i].backdrop,"https://fixture.invalid/%d.jpg",i);
  }
  cat_definir_tudo(itens,6,NULL,0);
  heroSetN = 4; heroSet[0]=4; heroSet[1]=1; heroSet[2]=5; heroSet[3]=2;
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  int casos = 0;
  for (int t = 0; t < 4; t++) for (layoutTeste=0;layoutTeste<3;layoutTeste++) {
    float w=telas[t][0], h=telas[t][1];
    for(int cheio=0;cheio<2;cheio++) {
      preparar(w,h,1); heroArteRect=heroArtworkRect(layoutTeste,cheio);
      moverDedo(w*.75f,w*.75f-324,160);
      float anterior, atual;
      toqueHeroDeslocamentos(heroArteRect.w,&anterior,&atual);
      perto(anterior*heroArteRect.w,-324);
      perto((atual-anterior)*heroArteRect.w,heroArteRect.w);
      perto(toqueHero.largura,heroArteRect.w);
      dedo(SDL_FINGERUP,1,w*.75f-324,300);
      assert(toqueHero.confirmar); perto(toqueHero.destino,-heroArteRect.w);
      terminar(); assert(heroAtual==5 && !teclas && !focos); casos++;
    }
    /* Row focus can publish narrow art before capture expands the main hero.
       That width change must preserve the finger's pixels and snap endpoint. */
    preparar(w,h,1); heroArteRect=heroArtworkRect(layoutTeste,0);
    moverDedo(w*.75f,w*.75f-324,160);
    dedo(SDL_FINGERUP,1,w*.75f-324,300);
    float anterior, atual;
    GfxRect expandido=heroArtworkRect(layoutTeste,1);
    toqueHeroDeslocamentos(expandido.w,&anterior,&atual);
    perto(anterior*expandido.w,-324);
    perto((atual-anterior)*expandido.w,expandido.w);
    perto(toqueHero.destino,-expandido.w);
    terminar(); assert(heroAtual==5 && !teclas); casos++;
    preparar(w,h,1);
    moverDedo(w*.75f,w*.45f,160);
    assert(toqueHero.estado == 1 && toqueHero.alvo == 5); perto(toqueHero.x,-w*.30f);
    relogioTeste += 16; dedo(SDL_FINGERMOTION,1,w*.40f,300); perto(toqueHero.x,-w*.35f);
    assert(heroAtual == 1 && !teclas && !focos);
    dedo(SDL_FINGERUP,1,w*.40f,300); assert(toqueHero.confirmar);
    passo(16); assert(toqueHero.x < -w*.35f && heroAtual == 1);
    terminar(); assert(heroAtual == 5 && heroPendente == 5 && heroDesejado == -1 && !teclas && !focos); casos++;
    /* The next right swipe returns through the non-contiguous set. */
    moverDedo(w*.3f,w*.65f,160); perto(toqueHero.x,w*.35f);
    dedo(SDL_FINGERUP,1,w*.65f,300); terminar(); assert(heroAtual == 1 && !teclas); casos++;
    /* Pause before release: a short move returns without choosing a title. */
    moverDedo(w*.6f,w*.51f,200); relogioTeste += 120;
    dedo(SDL_FINGERUP,1,w*.51f,300); assert(!toqueHero.confirmar);
    terminar(); assert(heroAtual == 1 && !teclas); casos++;
    /* A short deliberate fling does choose the adjacent title. */
    moverDedo(w*.6f,w*.54f,20); dedo(SDL_FINGERUP,1,w*.54f,300);
    assert(toqueHero.confirmar); terminar(); assert(heroAtual == 5 && !teclas); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    ponteiro_cancelar_toque(); assert(!toqueHero.estado);
    dedo(SDL_FINGERUP,1,w*.35f,300); assert(heroAtual == 1 && !teclas); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    dedo(SDL_FINGERDOWN,2,w*.5f,300); assert(!toqueHero.estado);
    dedo(SDL_FINGERUP,2,w*.5f,300); dedo(SDL_FINGERUP,1,w*.35f,300);
    assert(heroAtual == 1 && !teclas); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    nv_layout_w=h; nv_layout_h=w; toqueHeroAtualizar(.016f,relogioTeste,0,0);
    assert(!toqueHero.estado && heroAtual == 1 && !teclas); casos++;
    preparar(w,h,0); moverDedo(w*.3f,w*.65f,150);
    assert(toqueHero.alvo < 0); dedo(SDL_FINGERUP,1,w*.65f,300);
    terminar(); assert(heroAtual == 4 && !teclas); casos++;
    preparar(w,h,3); moverDedo(w*.7f,w*.35f,150);
    assert(toqueHero.alvo < 0); dedo(SDL_FINGERUP,1,w*.35f,300);
    terminar(); assert(heroAtual == 2 && !teclas); casos++;
    preparar(w,h,1); dedo(SDL_FINGERDOWN,1,w*.6f,300);
    relogioTeste+=20; dedo(SDL_FINGERUP,1,w*.6f,300);
    assert(!toqueHero.estado && teclas==1 && focos==1 && heroAtual==1); casos++;
    preparar(w,h,1); moverDedo(15,150,150); dedo(SDL_FINGERUP,1,150,300);
    assert(menus==1 && !toqueHero.estado && heroAtual==1 && !teclas); casos++;
    preparar(w,h,1); nFileiras=6;
    for(int r=1;r<nFileiras;r++)fileiras[r]=fileiras[0];
    heroSetNFil=nFileiras;
    dedo(SDL_FINGERDOWN,1,w*.6f,300);
    relogioTeste+=80; dedo(SDL_FINGERMOTION,1,w*.6f,180);
    assert(!toqueHero.estado && toqueLivreY); perto(scrollY,-empurraHero()+120);
    dedo(SDL_FINGERUP,1,w*.6f,180); assert(!teclas); casos++;
    preparar(w,h,1); fileiras[0].n=24;
    float linhaY=topoFileiras()-scrollY+NV_LEGACY_ROW_HEAD_H+40;
    dedo(SDL_FINGERDOWN,1,w*.75f,linhaY); relogioTeste+=80;
    dedo(SDL_FINGERMOTION,1,w*.75f-120,linhaY);
    assert(!toqueHero.estado && toqueLivreX[0]); perto(scrollX[0],120);
    dedo(SDL_FINGERUP,1,w*.75f-120,linhaY); assert(!teclas); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    dedo(SDL_FINGERUP,1,w*.35f,300); toqueHeroAtualizar(.001f,relogioTeste,0,1);
    assert(!toqueHero.estado && heroAtual==5 && !teclas); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    dedo(SDL_FINGERUP,1,w*.35f,300); passo(16);
    dedo(SDL_FINGERDOWN,1,w*.55f,300); dedo(SDL_FINGERUP,1,w*.55f,300);
    assert(toqueHero.estado==2 && !teclas && !focos);
    terminar(); assert(heroAtual==5 && !teclas && !focos); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    toqueHeroAtualizar(.016f,relogioTeste,1,0); assert(!toqueHero.estado && heroAtual==1); casos++;
    preparar(w,h,1); moverDedo(w*.7f,w*.35f,150);
    char id[64]; snprintf(id,sizeof id,"%s",itensTeste[5].imdb);
    snprintf(itensTeste[5].imdb,sizeof itensTeste[5].imdb,"changed");
    toqueHeroAtualizar(.016f,relogioTeste,0,0);
    assert(!toqueHero.estado && heroAtual==1);
    snprintf(itensTeste[5].imdb,sizeof itensTeste[5].imdb,"%s",id); casos++;
    preparar(w,h,1); heroPasso(1);
    assert(heroDesejado==5 && heroAtual==1 && heroDirDesejado==1 && !toqueHero.estado); casos++;
  }
  printf("home_hero_swipe: %d actual pointer/cancel/snap/direction/edge/page/arrow cases passed\n",casos);
  return 0;
}
