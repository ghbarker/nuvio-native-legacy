// Actual modal + pointer modules; no display, real account or LAN server.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "estatvivoui.h"
#include "celbotao.h"
#include "celular.h"
#include "ponteiro.h"
#include "gfx.h"
#include "layout.h"
#ifndef SDL_DISABLE
#define SDL_DISABLE 0
#endif
#include "ponteiro_sdl.h"
static Uint32 now = 1000;
static int activated, serverOpen, serverStops;
static Uint32 clockNow(void) { return now; }
int celular_disponivel(void) { return 1; }
int celular_abrir(const char *title) { (void)title; serverOpen = 1; return CEL_ESPERANDO; }
void celular_fechar(void) { serverOpen = 0; serverStops++; }
int celular_estado(void) { return serverOpen ? CEL_ESPERANDO : CEL_PARADO; }
float gfx_escala(void) { return 1; }
float gfx_opacidade_grupo = 1;
void gfx_sem_recorte(void) {}
void gfx_cor(GfxRect r, float radius, float red, float green, float blue, float alpha) {
  (void)r; (void)radius; (void)red; (void)green; (void)blue; (void)alpha;
}
static void activate(int a, int b) { (void)a; (void)b; activated++; }
static void deliver(const SDL_Event *e) { (void)e; }
static void finger(Uint32 type) {
  SDL_Event e; SDL_zero(e); e.type = type; e.tfinger.touchId = 1; e.tfinger.fingerId = 2;
  e.tfinger.x = 300.0f / NV_TELA_W; e.tfinger.y = 200.0f / NV_TELA_H;
  now += 16; assert(ponteiro_evento(&e, deliver));
}
static void tap(void) { finger(SDL_FINGERDOWN); finger(SDL_FINGERUP); }
static void frame(void) {
  ponteiro_quadro(now);
  ponteiro_alvo(100, 100, 500, 250, NULL, activate, 1, 0);
  // Exact Mobile CAMADA_SE(celb_aberto()) behavior from app.c. A hosted QR
  // does not paint its floating card, but this global null target existed.
  if (celb_aberto()) {
    ponteiro_camada();
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, NULL, 0, 0);
  }
  ponteiro_desenhar(); now += 16;
}
static void boundary(void) {
  ponteiro_abrir_sessao();
  estatvivo_fechar();
  celb_fechar();
}
int main(void) {
  SDL_Event key; SDL_zero(key); key.type = SDL_KEYDOWN; key.key.keysym.sym = SDLK_RIGHT;
  estatvivo_abrir(); assert(estatvivo_aberto());
  assert(estatvivo_evento(&key));  // still consumes after the player stops
  boundary(); assert(!estatvivo_aberto() && !estatvivo_fixado());
  assert(!estatvivo_evento(&key));
  puts("PASS: outgoing statistics modal no longer consumes the new profile's keys");
  estatvivo_abrir(); key.key.keysym.sym = SDLK_RETURN;
  assert(estatvivo_evento(&key) && estatvivo_fixado()); boundary();
  assert(!estatvivo_fixado());
  puts("PASS: a pinned statistics sample is also removed without a player action");

  ponteiro_teste_relogio(clockNow); ponteiro_teste_janela(1920,1080);
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  assert(celb_abrir_embutido(CELB_TECLADO,"Fixture draft"));
  assert(celb_aberto() && celb_embutido(CELB_TECLADO) && serverOpen);
  frame(); tap(); assert(activated == 0);
  int previousStops = serverStops; boundary();
  assert(!celb_aberto() && !celb_embutido(CELB_TECLADO) && !serverOpen);
  assert(serverStops == previousStops + 1);
  frame(); tap(); assert(activated == 1);
  puts("PASS: invisible hosted QR blocker clears and a fresh real pointer tap reaches Home");
  return 0;
}
