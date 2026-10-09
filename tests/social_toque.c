#define NV_TOUCH_PREVIEW 1
#if defined(TESTE_PESSOAS)
#include "../src/pessoas.c"
#elif defined(TESTE_OPINIOES)
#include "../src/amigostitulo_ui.c"
#elif defined(TESTE_PERFIL)
#include "../src/amigoperfil.c"
#elif defined(TESTE_AMIGOS)
#include "../src/amigosfil.c"
#elif defined(TESTE_ENVIAR)
#include "../src/recenviar.c"
#elif defined(TESTE_ARTE)
#include "../src/trocaarte.c"
#elif defined(TESTE_GUIA)
#include "../src/guia.c"
#else
#error escolha uma tela TESTE_*
#endif
#include <assert.h>
#include <math.h>
#include "rolagemtoque.h"

float nv_layout_w = 2400.0f;
float nv_layout_h = 1080.0f;
float gfx_escala(void) { return 1.0f; }
float gfx_escala_ui(void) { return 1.0f; }
int teclado_aberto(void) { return 0; }
const char *recomenda_meu_codigo(void) { return "abcdef"; }
Uint32 SDL_GetTicks(void) { return 100; }
int tex_falhou(const char *s) { (void)s; return 0; }
#if defined(TESTE_AMIGOS)
int socialvis_n_amigos(void) { return 5; }
const SvAmigo *socialvis_amigo(int i) { (void)i; return NULL; }
#endif
static void perto(float a, float b) { assert(fabsf(a - b) < 0.01f); }

int main(void) {
  PonteiroRolagem e = { PONT_ROL_INICIO, 1, 0, 0, 400, 400 };
  GfxRect vista = { 100, 100, 500, 350 };
#if defined(TESTE_GUIA)
  nCats = 3;
  for (int l = 0; l < nCats; l++) catN[l] = 12;
  toquerol_vincular(&toqueGradeY, vista, 2, 0, 1200, 1, &rolY);
  assert(toqueGuiaRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueGuiaRolar(&e)); perto(rolY, 37); assert(focoLin == 0 && focoCol == 0);
  e.fase = PONT_ROL_FIM; toqueGuiaRolar(&e); assert(toqueGradeY.livre);
  e.fase = PONT_ROL_INERCIA; e.delta = -1e6f; toqueGuiaRolar(&e); perto(rolY, 1200);
  assert(!toqueGuiaRolar(&e));
  toqueLimpar();
  toquerol_vincular(&toqueGradeX[1], vista, 2, 0, 1000, 0, &rolX[1]);
  e.fase = PONT_ROL_INICIO; e.eixoY = 0;
  assert(toqueGuiaRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueGuiaRolar(&e)); perto(rolX[1], 37); perto(rolX[0], 0);
  ponteiroCanal(1, 5); assert(focoLin == 1 && focoCol == 5 && !toqueGradeX[1].livre);
  toqueLimpar(); modoLista = 1; janelaDesl = 0; toquePpm = 10;
  toquerol_vincular(&toqueTempo, vista, 2, 0, G_L_DESL_MAX, 0, &toqueTempoMin);
  e.fase = PONT_ROL_INICIO;
  assert(toqueGuiaRolar(&e)); e.fase = PONT_ROL_MOVER;
  assert(toqueGuiaRolar(&e)); perto(toqueTempoMin, 3.7f);
  assert(janelaIni(7200) == 7422); // 37 pixels, no 30-minute navigation step.
  e.fase = PONT_ROL_FIM; toqueGuiaRolar(&e); assert(toqueTempo.livre);
  e.fase = PONT_ROL_INERCIA; e.delta = -1e6f; toqueGuiaRolar(&e);
  perto(toqueTempoMin, G_L_DESL_MAX); assert(!toqueGuiaRolar(&e));
  toqueGuiaRetomar(); assert(!toqueTempo.livre && janelaDesl == G_L_DESL_MAX);
  ToqueRolagem *listas[] = { &toqueCat, &toqueAddon, &toqueBusca };
  float *offsets[] = { &catRol, &paRol, &buscaRol };
  int (*callbacks[])(const PonteiroRolagem *) = { toqueCategoriaRolar, toqueAddonRolar, toqueBuscaRolar };
  for (int k = 0; k < 3; k++) {
    toquerol_vincular(listas[k], vista, 2, 0, 1000, 1, offsets[k]);
    e.fase = PONT_ROL_INICIO; e.eixoY = 1;
    assert(callbacks[k](&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
    assert(callbacks[k](&e)); perto(*offsets[k], 37);
    e.fase = PONT_ROL_CANCELAR; callbacks[k](&e); assert(listas[k]->livre);
    e.fase = PONT_ROL_INICIO; e.x = 0; assert(!callbacks[k](&e)); e.x = 400;
  }
#endif
#if defined(TESTE_PESSOAS)
  aberto = 1; nL = 20; conteudoH = 2000;
  for (int i = 0; i < nL; i++) { linhas[i].tipo = T_NAV; linhaY[i] = i * 100.0f; }
  toquerol_vincular(&toque, vista, 2, 0, 1650, 1, &rolagem);
  assert(toqueRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueRolar(&e)); perto(rolagem, 37); perto(rolagemAlvo, 37);
  e.fase = PONT_ROL_FIM; toqueRolar(&e); assert(toque.livre);
  toqueRetomar(); assert(!toque.livre && foco >= 0 && foco < nL);
  toque.livre = 1; ponteiroLinha(7, 0); assert(!toque.livre && foco == 7);
#elif defined(TESTE_OPINIOES)
  aberta = 1; lista.n = 20;
  toquerol_vincular(&toque, vista, 2, 0, 2200, 1, &rolagem);
  assert(toqueRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueRolar(&e)); perto(rolagem, 37); assert(foco == 0);
  e.fase = PONT_ROL_CANCELAR; toqueRolar(&e); assert(toque.livre);
  ponteiroLinha(7, 0); assert(!toque.livre && foco == 7);
#elif defined(TESTE_PERFIL)
  temPerfil = 1; perf.nAssistindo = 8; fila = col = 0;
  toquerol_vincular(&toque[0], vista, 2, 0, 1000, 0, &toqueX[0]);
  e.eixoY = 0;
  assert(toqueRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueRolar(&e)); perto(toqueX[0], 37); assert(fila == 0 && col == 0);
  e.fase = PONT_ROL_FIM; toqueRolar(&e); assert(toque[0].livre);
  ponteiroCartaz(0, 7); assert(!toque[0].livre && col == 7);
#elif defined(TESTE_AMIGOS)
  ultX0 = 100; ultAlt = 240;
  toquerol_vincular(&toque, vista, 2, 0, 1000, 0, &scroll);
  e.eixoY = 0;
  assert(amigosfil_rolagem(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(amigosfil_rolagem(&e)); perto(scroll, 37);
  e.fase = PONT_ROL_FIM; amigosfil_rolagem(&e); assert(toque.livre);
  int c = 0; amigosfil_retomar_foco(&c); assert(!toque.livre && c >= 0 && c < 6);
  toque.livre = 1; amigosfil_focar(3, 1); assert(!toque.livre && colAnt == 3 && dentro == 1);
#elif defined(TESTE_ENVIAR)
  aberto = 1; pagina = RE_PAG_AMIGOS; nCtts = 12; toquePasso = RE_L_PESSOA + RE_L_GAP;
  toquerol_vincular(&toque, vista, 2, 0, 10, 1, &rolar);
  assert(toqueRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueRolar(&e)); perto(rolar * toquePasso, 37); assert(foco == 0);
  e.fase = PONT_ROL_FIM; toqueRolar(&e); assert(toque.livre);
  ponteiroLinha(7, 0); assert(!toque.livre && foco == 7);
#elif defined(TESTE_ARTE)
  aberto = 1; nCand[0] = 20;
  toquerol_vincular(&toque[0], vista, 2, 0, 500, 1, &toqueY[0]);
  assert(toqueRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -74;
  assert(toqueRolar(&e)); perto(toqueY[0], 37); assert(foco[0] == 0);
  e.fase = PONT_ROL_FIM; toqueRolar(&e); assert(toque[0].livre);
  toqueRetomar(); assert(!toque[0].livre && foco[0] >= 0 && foco[0] < nCand[0]);
  toque[0].livre = 1; ponteiroFoco(0, 17); assert(!toque[0].livre && foco[0] == 17);
#endif
#if !defined(TESTE_GUIA)
#if defined(TESTE_PERFIL) || defined(TESTE_ARTE)
  ToqueRolagem *r = &toque[0];
#else
  ToqueRolagem *r = &toque;
#endif
#if defined(TESTE_AMIGOS)
  int (*callback)(const PonteiroRolagem *) = amigosfil_rolagem;
#else
  int (*callback)(const PonteiroRolagem *) = toqueRolar;
#endif
  e.fase = PONT_ROL_INICIO; e.eixoY = r->eixoY; e.delta = 0;
  assert(callback(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -1e7f; callback(&e); perto(*r->offset, r->maximo);
  e.fase = PONT_ROL_INERCIA; assert(!callback(&e));
  e.delta = 1e7f; callback(&e); perto(*r->offset, r->minimo);
  e.fase = PONT_ROL_INICIO; e.eixoY = !r->eixoY; assert(!callback(&e));
  e.eixoY = r->eixoY; e.x = 0; assert(!callback(&e));
#endif
  puts("social_toque: OK");
  return 0;
}
