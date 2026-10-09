// #216: dedo real, sem SDL host, video ou dispositivo. Eventos SDL normalizados.
#define main ponteiro_roteiro_mouse
#include "ponteiro.c"
#undef main
#include <math.h>
#include "ctxlista.h"
#include "layout.h"
#include "catalogo.h"
#include "ctxmenu.h"
#include "descoberta.h"
#include "idioma.h"
#include "ilha.h"

// Dependencias inertes da lista: a decisao do gesto vem de src/ctxlista.c.
int ctx_aberto(void) { return 0; }
const char *i18n(const char *s) { return s; }
void ilha_atividade(const char *s, float p) { (void)s; (void)p; }
int cat_n(void) { return 0; }
int cat_indice_por_imdb(const char *s) { (void)s; return -1; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
void ctx_fileira(const char *c, const char *t) { (void)c; (void)t; }
void ctx_dispensar_retomar(int on) { (void)on; }
void ctx_abrir_cartaz(int i, GfxRect r, const char *a) { (void)i; (void)r; (void)a; }
void ctx_evento(const SDL_Event *e) { (void)e; }
int desc_titulo_buscando(void) { return 0; }
void desc_pedir_titulo_semente(const char *imdb, long tmdb, const char *tipo,
                               const char *titulo, const char *ano, const char *poster) {
  (void)imdb; (void)tmdb; (void)tipo; (void)titulo; (void)ano; (void)poster;
}

static void (*entregaDedo)(const SDL_Event *) = entregar;

static void dedo(Uint32 tipo, Sint64 id, float x, float y) {
  SDL_Event e; SDL_zero(e);
  e.type = tipo; e.tfinger.touchId = 3; e.tfinger.fingerId = id;
  e.tfinger.x = x; e.tfinger.y = y;
  CONFERE(ponteiro_evento(&e, entregaDedo) == 1, "evento de dedo consumido");
}
static void tocar(float x, float y) {
  dedo(SDL_FINGERDOWN, 1, x / 1920.0f, y / 1080.0f);
  dedo(SDL_FINGERUP, 1, x / 1920.0f, y / 1080.0f);
}
static void preparar(void (*alvos)(void)) {
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  entregaDedo = entregar;
  quadro(alvos); zerar(); nFocar = nAtivar = 0;
}
static int conta(SDL_Keycode k) {
  int n = 0;
  for (int i = 0; i < nEntregues; i++) if (entregues[i].type == SDL_KEYDOWN && entregues[i].key.keysym.sym == k) n++;
  return n;
}
static int nArrasto, arrastoToque;
static float arrastoX;
static void arrastoBarra(int a, int b) { (void)a; (void)b; nArrasto++; arrastoX = ponteiro_x(); arrastoToque = ponteiro_toque(); }
static void barra(void) {
  ponteiro_alvo(0, 0, 1920, 1080, focar, NULL, 5, 5);
  ponteiro_alvo(200, 900, 1500, 60, NULL, arrastoBarra, 0, 0);
  ponteiro_alvo_arrastavel();
}
static void canto(void) { ponteiro_alvo(1900, 1060, 20, 20, focar, NULL, 1, 1); }

static CtxHold holdDedo;
static int menusDedo, toquesDedo, flagsLongos, focosLongos;
static void entregarHold(const SDL_Event *e) {
  int r;
  entregar(e);
  if (ponteiro_ok_longo()) flagsLongos++;
  r = ctxhold_evento(&holdDedo, e, 1);
  if (r == CTXH_LONGO) menusDedo++;
  if (r == CTXH_TOQUE) toquesDedo++;
}
static void prepararHold(void (*alvos)(void)) {
  preparar(alvos);
  memset(&holdDedo, 0, sizeof holdDedo);
  menusDedo = toquesDedo = flagsLongos = focosLongos = 0;
  entregaDedo = entregarHold;
}
static void focoTemporada(int a, int b) {
  focar(a, b);
  if (ponteiro_toque()) focosLongos++;
}
static void temporada(void) {
  ponteiro_alvo(100, 100, 200, 100, NULL, ativar, 2, 3);
  ponteiro_alvo_segurar(focoTemporada);
}
static void pressaoLonga(void) {
  Uint32 inicio;
  SDL_Event e;
  prepararHold(home);
  inicio = relogio;
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio = inicio + NV_HOLD_MS - 1;
  quadro(home);
  CONFERE(!nEntregues && !nFocar, "antes do limiar nao existe OK nem foco");
  relogio = inicio + NV_HOLD_MS;
  quadro(home);
  CONFERE(nEntregues == 2 && nFocar == 1 && menusDedo == 1 && !toquesDedo &&
          flagsLongos == 2 && !holdDedo.armado,
          "limiar confirma um par OK longo pelo ctxhold real, sem abrir curto");
  CONFERE(!ponteiro_ok_longo(), "marca longa so existe durante a entrega");
  for (int i = 0; i < 60; i++) quadro(home);
  dedo(SDL_FINGERMOTION, 1, .5f, .5f);
  dedo(SDL_FINGERUP, 1, .5f, .5f);
  CONFERE(nEntregues == 2 && menusDedo == 1 && nFocar == 1,
          "segurar mais, arrastar e soltar depois do limiar nao repetem nem rolam");
  tocar(150, 150);
  CONFERE(toquesDedo == 1 && menusDedo == 1 && flagsLongos == 2,
          "proximo tap e curto e nao herda a marca longa");

  prepararHold(home);
  inicio = relogio;
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += NV_HOLD_MS;
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(menusDedo == 1 && nEntregues == 2, "soltura apos limiar sem quadro tambem confirma longo");

  prepararHold(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += NV_HOLD_MS - 1;
  dedo(SDL_FINGERMOTION, 1, .08f + 40.0f / 1920.0f, .14f);
  relogio += 1000; quadro(home);
  dedo(SDL_FINGERMOTION, 1, .08f, .14f);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!conta(SDLK_RETURN) && !nFocar && !menusDedo,
          "arrasto antes do limiar cancela longo mesmo retornando ao inicio");

  prepararHold(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += 200;
  dedo(SDL_FINGERMOTION, 1, .08f + 10.0f / 1920.0f, .14f);
  relogio += 600; quadro(home);
  dedo(SDL_FINGERUP, 1, .08f + 10.0f / 1920.0f, .14f);
  CONFERE(menusDedo == 1 && nEntregues == 2, "tremor pequeno mantem pressao longa");

  prepararHold(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += NV_HOLD_MS - 1;
  dedo(SDL_FINGERDOWN, 2, .09f, .14f);
  dedo(SDL_FINGERUP, 2, .09f, .14f);
  relogio += 1000; quadro(home);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nFocar, "segundo dedo cancela longo antes de confirmar");

  prepararHold(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += NV_HOLD_MS;
  quadro(homeComFolha);
  quadro(home);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nFocar, "camada nova no limiar cancela, mesmo voltando ao alvo antigo");

  for (int i = 0; i < 3; i++) {
    prepararHold(home);
    dedo(SDL_FINGERDOWN, 1, .08f, .14f);
    if (!i) dedo(SDL_FINGERMOTION, 1, NAN, .14f);
    else { SDL_zero(e); e.type = i == 1 ? SDL_WINDOWEVENT : SDL_APP_WILLENTERBACKGROUND;
      e.window.event = SDL_WINDOWEVENT_FOCUS_LOST; ponteiro_evento(&e, entregarHold); }
    relogio += NV_HOLD_MS + 1000; quadro(home);
    dedo(SDL_FINGERUP, 1, .08f, .14f);
    CONFERE(!nEntregues && !nFocar, "coordenada invalida, foco perdido ou background cancelam longo");
  }

  prepararHold(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  CONFERE(tecla(SDL_KEYDOWN, SDLK_RIGHT, 0) == 0, "seta real continua passando");
  relogio += NV_HOLD_MS; quadro(home);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nFocar, "navegar no controle cancela o dedo pendente");

  prepararHold(homeComFolha);
  dedo(SDL_FINGERDOWN, 1, 1450.0f / 1920.0f, 500.0f / 1080.0f);
  relogio += NV_HOLD_MS; quadro(homeComFolha);
  dedo(SDL_FINGERUP, 1, 1450.0f / 1920.0f, 500.0f / 1080.0f);
  CONFERE(!nEntregues && !nAtivar, "anteparo absorve pressao longa");
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 500.0f / 1080.0f);
  relogio += NV_HOLD_MS; quadro(homeComFolha);
  CONFERE(!nEntregues && !nAtivar, "acao propria nao ganha OK longo implicitamente");
  dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 500.0f / 1080.0f);
  CONFERE(!nEntregues && nAtivar == 1, "acao propria continua confirmando somente na soltura");

  prepararHold(temporada);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += NV_HOLD_MS; quadro(temporada);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(menusDedo == 1 && focosLongos == 1 && focoA == 2 && !nAtivar,
          "alvo proprio opta por foco longo sem disparar ativar nem repetir na soltura");
  tocar(150, 150);
  CONFERE(nAtivar == 1, "tap no alvo optante preserva seu ativar proprio");

  prepararHold(NULL);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  relogio += NV_HOLD_MS; quadro(NULL);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(menusDedo == 1 && nEntregues == 2, "camada sem alvos mantem OK longo puro");
}
int main(void) {
  ponteiro_roteiro_mouse();
  ponteiro_teste_relogio(agora);
  // Janela 960x540 e drawable diferente nao mudam coords normalizadas SDL.
  ponteiro_teste_janela(960, 540);
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 150.0f / 1080.0f);
  CONFERE(nFocar == 0 && nEntregues == 0, "DOWN nao foca nem abre");
  dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 150.0f / 1080.0f);
  CONFERE(nFocar == 1 && focoA == 0 && focoB == 0 && nEntregues == 2,
          "UP confirma toque, foca e envia um OK");
  CONFERE(entregues[0].type == SDL_KEYDOWN && entregues[1].type == SDL_KEYUP &&
          entregues[0].key.keysym.sym == SDLK_RETURN,
          "um par RETURN completo");
  CONFERE(fabsf(ponteiro_x() - 150.0f) < 0.01f && fabsf(ponteiro_y() - 150.0f) < 0.01f,
          "coords normalizadas viram viewport logico, independente DPI");
  // SDL tambem envia mouse derivado do mesmo tap: todos sao consumidos.
  for (int i = 0; i < 4; i++) {
    SDL_Event e; SDL_zero(e);
    e.type = i == 0 ? SDL_MOUSEMOTION : i == 1 ? SDL_MOUSEBUTTONDOWN :
             i == 2 ? SDL_MOUSEBUTTONUP : SDL_MOUSEWHEEL;
    if (i == 0) e.motion.which = SDL_TOUCH_MOUSEID;
    else if (i == 3) { e.wheel.which = SDL_TOUCH_MOUSEID; e.wheel.y = 1; }
    else { e.button.which = SDL_TOUCH_MOUSEID; e.button.button = SDL_BUTTON_LEFT; }
    CONFERE(ponteiro_evento(&e, entregar) == 1, "mouse emulado de toque consumido");
  }
  CONFERE(nEntregues == 2 && nFocar == 1, "mouse emulado nao duplica acao");

  preparar(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  dedo(SDL_FINGERMOTION, 1, .30f, .14f);
  dedo(SDL_FINGERMOTION, 1, .08f, .14f); // retornar nao transforma swipe emtap
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  { int ok = !nFocar;
    for (int i = 0; i < nEntregues; i++) if (entregues[i].key.keysym.sym == SDLK_RETURN) ok = 0;
    CONFERE(ok, "arrasto nao foca nem abre, mesmo retornando ao inicio"); }
  zerar();
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  dedo(SDL_FINGERDOWN, 2, .09f, .14f);
  dedo(SDL_FINGERUP, 2, .09f, .14f);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nFocar, "multifinger cancela o gesto inteiro");
  tocar(150, 150);
  CONFERE(nEntregues == 2, "proximo gesto simples funciona apos multifinger");
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, NAN, .14f);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  dedo(SDL_FINGERUP, 1, .08f, INFINITY);
  dedo(SDL_FINGERUP, 99, .08f, .14f);
  CONFERE(!nEntregues, "NaN infinito e UP sem DOWN nao ativam");
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  SDL_Event e; SDL_zero(e); e.type = SDL_WINDOWEVENT; e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
  ponteiro_evento(&e, entregar);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues, "perda de foco cancela tap");
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  e.type = SDL_APP_WILLENTERBACKGROUND; ponteiro_evento(&e, entregar);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues, "background cancela tap");
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, .08f, .14f);
  quadro(homeComFolha);
  dedo(SDL_FINGERUP, 1, .08f, .14f);
  CONFERE(!nEntregues && !nAtivar, "modal aberta entre DOWN e UP cancela alvo antigo");
  preparar(homeComFolha);
  tocar(1450, 500);
  CONFERE(!nEntregues && !nAtivar, "anteparo da folha absorve tap no vazio");
  tocar(1550, 150);
  CONFERE(nEntregues == 2 && focoA == 7 && focoB == 0, "alvo da camada superior recebe tap");
  zerar(); tocar(150, 500);
  CONFERE(nAtivar == 1 && ativA == 99 && !nEntregues, "acao propria so na soltura");
  preparar(canto);
  dedo(SDL_FINGERDOWN, 1, 1.01f, 1.01f); dedo(SDL_FINGERUP, 1, 1.01f, 1.01f);
  CONFERE(nEntregues == 2 && ponteiro_x() == 1919.0f && ponteiro_y() == 1079.0f,
          "bordas finitas limitadas ao viewport");
  // ROLAGEM: dedo sobe 400 px logicos devagar = duas setas para BAIXO, sem OK.
  preparar(home);
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 700.0f / 1080.0f);
  for (int y = 680; y >= 300; y -= 20) { relogio += 40; dedo(SDL_FINGERMOTION, 1, 150.0f / 1920.0f, y / 1080.0f); }
  relogio += 200;
  dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 300.0f / 1080.0f);
  CONFERE(conta(SDLK_DOWN) == 2 && conta(SDLK_UP) == 0 && conta(SDLK_RETURN) == 0 && !nFocar,
          "arrasto vertical lento: 2 setas para baixo, sem foco nem OK");
  for (int i = 0; i < 30; i++) quadro(home);
  CONFERE(conta(SDLK_DOWN) == 2, "soltar parado nao tem inercia");
  // Dedo desce: setas para CIMA.
  zerar();
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 200.0f / 1080.0f);
  for (int y = 220; y <= 560; y += 20) { relogio += 40; dedo(SDL_FINGERMOTION, 1, 150.0f / 1920.0f, y / 1080.0f); }
  relogio += 200; dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 560.0f / 1080.0f);
  CONFERE(conta(SDLK_UP) == 2 && conta(SDLK_DOWN) == 0, "arrasto para baixo rola para cima");
  // Horizontal: dedo para a esquerda = DIREITA (proximo card).
  zerar();
  dedo(SDL_FINGERDOWN, 1, 900.0f / 1920.0f, 150.0f / 1080.0f);
  for (int x = 880; x >= 400; x -= 20) { relogio += 40; dedo(SDL_FINGERMOTION, 1, x / 1920.0f, 160.0f / 1080.0f); }
  relogio += 200; dedo(SDL_FINGERUP, 1, 400.0f / 1920.0f, 160.0f / 1080.0f);
  CONFERE(conta(SDLK_RIGHT) == 2 && conta(SDLK_LEFT) == 0 && conta(SDLK_DOWN) == 0 && conta(SDLK_UP) == 0,
          "arrasto horizontal trava no eixo x e vai para a direita");
  // INERCIA: um peteleco rapido segue andando e para sozinho.
  zerar();
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 800.0f / 1080.0f);
  for (int y = 760; y >= 560; y -= 40) { relogio += 16; dedo(SDL_FINGERMOTION, 1, 150.0f / 1920.0f, y / 1080.0f); }
  relogio += 8; dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 560.0f / 1080.0f);
  { int antes = conta(SDLK_DOWN), depois, fim;
    for (int i = 0; i < 120; i++) quadro(home);
    depois = conta(SDLK_DOWN);
    for (int i = 0; i < 60; i++) quadro(home);
    fim = conta(SDLK_DOWN);
    CONFERE(antes == 1 && depois > antes && depois <= antes + 12 && fim == depois && !conta(SDLK_UP),
            "peteleco tem inercia limitada e para");
    printf("inercia: %d seta(s) no gesto, %d depois de soltar\n", antes, depois - antes); }
  // Dedo novo segura a inercia.
  zerar();
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 800.0f / 1080.0f);
  for (int y = 760; y >= 560; y -= 40) { relogio += 16; dedo(SDL_FINGERMOTION, 1, 150.0f / 1920.0f, y / 1080.0f); }
  relogio += 8; dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 560.0f / 1080.0f);
  dedo(SDL_FINGERDOWN, 1, 150.0f / 1920.0f, 560.0f / 1080.0f);
  { int antes = conta(SDLK_DOWN);
    for (int i = 0; i < 60; i++) quadro(home);
    CONFERE(conta(SDLK_DOWN) == antes, "tocar de novo para a inercia"); }
  dedo(SDL_FINGERUP, 1, 150.0f / 1920.0f, 560.0f / 1080.0f);
  // ARRASTO DE ALVO: a barra recebe o ativar a cada movimento, nada rola.
  preparar(barra);
  nArrasto = 0;
  dedo(SDL_FINGERDOWN, 1, 500.0f / 1920.0f, 930.0f / 1080.0f);
  for (int x = 520; x <= 1200; x += 40) { relogio += 16; dedo(SDL_FINGERMOTION, 1, x / 1920.0f, 930.0f / 1080.0f); }
  dedo(SDL_FINGERUP, 1, 1200.0f / 1920.0f, 930.0f / 1080.0f);
  CONFERE(nArrasto >= 10 && arrastoToque == 1 && arrastoX > 1190.0f && !nEntregues && !nFocar,
          "arrastar na barra chama ativar com a posicao, sem setas");
  CONFERE(!ponteiro_toque(), "ponteiro_toque so vale dentro do ativar");
  // Tocar (sem arrastar) na barra tambem e ativar por toque.
  nArrasto = 0; arrastoToque = 0; tocar(800, 930);
  CONFERE(nArrasto == 1 && arrastoToque == 1, "tap na barra ativa por toque");
  // Fora da barra o fundo do player: toque = focar (e OK, sem ativar proprio).
  zerar(); nFocar = 0; tocar(800, 300);
  CONFERE(nFocar == 1 && focoA == 5 && conta(SDLK_RETURN) == 1, "tap no fundo foca e da OK");
  pressaoLonga();
  printf("ponteiro toque: %s\n", falhas ? "FALHOU" : "PASS");
  return falhas ? 1 : 0;
}
