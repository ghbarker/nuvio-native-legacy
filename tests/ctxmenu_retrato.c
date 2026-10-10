/* Actual row-style dialog drawing, targets and selection, without GL/network. */
#ifndef NV_CTX_TESTE_OFICIAL
#define NV_TOUCH_UI 1
#endif
#define SDL_MAIN_HANDLED 1
#include "../src/ctxmenu.c"
#include <assert.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
float nv_cor_fundo_viva[3] = {.04f, .045f, .06f};
static float ui = 1, escala = 1, fonte = 1;
static GfxRect corte, previaTeste;
static int cortando, validar, nAlvos, salvou, tipoSalvo, nPrevias;
static PonteiroAlvo alvos[100];
static GfxRect painel;

float gfx_escala_ui(void) { return ui; }
float gfx_escala(void) { return escala; }
int ajustes_vidro(void) { return 0; }
int ajustes_idioma(void) { return 0; }
void ajustes_acento(float *r, float *g, float *b) { *r = *g = *b = .8f; }
float botao_cor_foco(float *r, float *g, float *b) { *r = *g = *b = .8f; return 0; }
const char *i18n(const char *s) { return s; }
int fil_tipo(const char *chave) { (void)chave; return FIL_TIPO_COLECAO; }
int fil_definir_tipo(const char *chave, int tipo) { (void)chave; salvou++; tipoSalvo = tipo; return 1; }
const char *fil_estilo_tam_palavra(int k) { return k == 0 ? "Pequeno" : k == 1 ? "Médio" : "Grande"; }
const char *fil_estilo_ajuda(const char *chave, int tipo) {
  (void)chave; (void)tipo;
  return "Uma descrição bastante longa da forma da fileira que precisa respeitar a largura da coluna da prévia.";
}

static void perto(float a, float b) { assert(fabsf(a - b) < .03f); }
static void dentro(GfxRect r, GfxRect b) {
  if (r.x < b.x - .03f || r.y < b.y - .03f || r.x + r.w > b.x + b.w + .03f ||
      r.y + r.h > b.y + b.h + .03f) {
    fprintf(stderr, "outside %.2f %.2f %.2f %.2f in %.2f %.2f %.2f %.2f; viewport %.0fx%.0f scale %.2f font %.2f\n",
            r.x, r.y, r.w, r.h, b.x, b.y, b.w, b.h, nv_layout_w, nv_layout_h, ui, fonte);
  }
  assert(r.w > 0 && r.h > 0);
  assert(r.x >= b.x - .03f && r.y >= b.y - .03f);
  assert(r.x + r.w <= b.x + b.w + .03f && r.y + r.h <= b.y + b.h + .03f);
}
static void desenho(GfxRect r) {
  if (!validar) return;
  if (cortando) {
    float x = fmaxf(r.x, corte.x), y = fmaxf(r.y, corte.y);
    float w = fminf(r.x + r.w, corte.x + corte.w) - x;
    float h = fminf(r.y + r.h, corte.y + corte.h) - y;
    if (w <= 0 || h <= 0) return;
    r = (GfxRect){x, y, w, h};
  }
  dentro(r, (GfxRect){0, 0, NV_TELA_W, NV_TELA_H});
}
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)ca; desenho(r);
  if (r.w < NV_TELA_W && r.h > 500 && !cortando) painel = r;
}
void gfx_anel(GfxRect r, float raio, float esp, float cr, float cg, float cb, float ca) {
  (void)raio; (void)esp; (void)cr; (void)cg; (void)cb; (void)ca; desenho(r);
}
void gfx_icone(GfxRect r, const char *nome, float cr, float cg, float cb, float ca) {
  (void)nome; (void)cr; (void)cg; (void)cb; (void)ca; desenho(r);
}
void gfx_sombra_sob(GfxRect s, float foco, float parx, float raio, float cr, float cg, float cb,
                    float ca, GfxRect p, float rp, float ap) {
  (void)s; (void)foco; (void)parx; (void)raio; (void)cr; (void)cg; (void)cb;
  (void)ca; (void)p; (void)rp; (void)ap;
}
void gfx_vidro_folha(GfxRect r, float raio, float a) { (void)raio; (void)a; desenho(r); }
void gfx_luz_canto(GfxRect r, float raio, float cx, float cy, float alcance,
                   float cr, float cg, float cb, float ca) {
  (void)raio; (void)cx; (void)cy; (void)alcance; (void)cr; (void)cg; (void)cb; (void)ca; desenho(r);
}
void gfx_recorte(float x, float y, float w, float h) { corte = (GfxRect){x, y, w, h}; cortando = 1; }
void gfx_sem_recorte(void) { cortando = 0; }

static int altura(TxtEstilo e) {
  return (int)((e == TXT_TITULO3 ? 48 : e == TXT_ROW_TITULO ? 34 :
                e == TXT_MINI ? 16 : e == TXT_ILHA_APOIO ? 18 : 26) * fonte);
}
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) {
  (void)r; (void)g; (void)b; (void)a;
  int h = altura(e);
  return (TxtLinha){.w = (int)(strlen(s) * h * .55f), .h = h};
}
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float max) {
  assert(max > 0);
  TxtLinha t = txt_linha(e, s, r, g, b, a);
  if (t.w > max) t.w = (int)max;
  return t;
}
void txt_desenhar_alpha(TxtLinha t, float x, float y, float a) {
  (void)a; if (t.w > 0) desenho((GfxRect){x, y, t.w, t.h});
}
float txt_tracking(TxtEstilo e, const char *s, int r, int g, int b,
                   float x, float y, float a, float tracking) {
  TxtLinha t = txt_linha(e, s, r, g, b, 255);
  t.w += (int)(strlen(s) * tracking);
  if (x >= 0) txt_desenhar_alpha(t, x, y, a);
  return t.w;
}
float txt_bloco_corta(TxtEstilo e, const char *s, int r, int g, int b,
                      float x, float y, float w, float lead, float a, int max) {
  TxtLinha t = txt_linha(e, s, r, g, b, 255);
  int n = (int)ceilf(t.w / w); if (n > max) n = max;
  t.w = (int)w; t.h = (int)((n - 1) * lead + t.h);
  txt_desenhar_alpha(t, x, y, a); return t.h;
}
float badge_largura(const char *texto) { (void)texto; return 86; }
float badge_desenhar(float x, float y, const char *texto, BadgeEstilo estilo, float a) {
  (void)texto; (void)estilo; (void)a; desenho((GfxRect){x, y, 86, BADGE_H}); return 86;
}
int home_previa_fileira(const char *chave, int tipo, int ref, GfxRect area, float a) {
  (void)chave; (void)tipo; (void)ref; (void)a;
  desenho(area); previaTeste = area; nPrevias++; return 1;
}

int ponteiro_ativo(void) { return 1; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { (void)fn; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  assert(nAlvos < 100);
  GfxRect r = {x, y, w, h}; if (validar) dentro(r, (GfxRect){0, 0, NV_TELA_W, NV_TELA_H});
  alvos[nAlvos++] = (PonteiroAlvo){x, y, w, h, focar, ativar, a, b};
}
void ponteiro_alvo_faixa(float x, float y, float w, float h, float y0, float y1,
                        PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  float top = fmaxf(y, y0), bottom = fminf(y + h, y1);
  if (bottom > top) ponteiro_alvo(x, top, w, bottom - top, focar, ativar, a, b);
}

static void montarTeste(void) {
  nEstilos = 7; pagina = 1; aberto = 1; estFoco = 0; prevAtual = FIL_TIPO_AUTO; prevAnt = -1; prevT = 1;
  snprintf(filTitulo, sizeof filTitulo, "Uma fileira com um nome muito longo para caber inteiro e que precisa ser truncado");
  snprintf(filChave, sizeof filChave, "catalogo:teste");
  for (int i = 0; i < nEstilos; i++) {
    estLin[i] = (FilEstiloLinha){"Uma forma com uma descrição de tamanho longa", 1, {FIL_TIPO_CARTAZ},
                               {"Uma forma bastante longa da prévia que precisa caber na coluna"}};
    estTam[i] = 0; estAnim[i] = i == estFoco;
  }
  for (int i = 2; i <= 4; i += 2) {
    estLin[i].n = 3;
    for (int j = 0; j < 3; j++) {
      estLin[i].tipos[j] = FIL_TIPO_SERVICO + j;
      estLin[i].nomes[j] = "Uma forma bastante longa da prévia que precisa caber na coluna";
    }
  }
}
static void desenharTeste(void) { nAlvos = nPrevias = 0; cortando = 0; desenhaEstilos(1); assert(nPrevias); }

#ifdef NV_TOUCH_UI
static void telefone(float w, float h, float s, int linhas) {
  nv_layout_w = w; nv_layout_h = h; ui = escala = s; montarTeste();
  nEstilos = linhas;
  estRolY = 0; memset(&toqueEst, 0, sizeof toqueEst); validar = 1;
  EstTelefoneGeo g = estTelefoneGeo(1);
  dentro(g.modal, (GfxRect){32, 32, NV_TELA_W - 64, NV_TELA_H - 64});
  dentro(g.lista, g.modal); dentro(g.previa, g.modal); dentro(g.cancelar, g.modal);
  desenharTeste(); dentro(previaTeste, g.previa);
  for (int i = 0; i < nEstilos; i++) {
    estFoco = i; toquerol_limpar(&toqueEst); desenharTeste();
    for (int j = 0; j < estLin[i].n; j++) {
      int achou = 0;
      for (int k = 0; k < nAlvos; k++) if (alvos[k].focar == ponteiroEstFoco && alvos[k].a == i &&
          alvos[k].b == (estLin[i].n > 1 ? j + 1 : 0)) {
        PonteiroAlvo t = alvos[k];
        dentro((GfxRect){t.x, t.y, t.w, t.h}, g.lista);
        if (t.b) { perto(t.w, EST_TOQUE_SEG); perto(t.h, EST_TOQUE_SEG); }
        aberto = 1; salvou = 0; t.ativar(t.a, t.b);
        assert(salvou == 1 && tipoSalvo == estLin[i].tipos[j] && !aberto);
        aberto = 1; achou = 1; break;
      }
      assert(achou);
    }
  }
  estFoco = 0; estRolY = 0; toquerol_limpar(&toqueEst); desenharTeste();
  PonteiroRolagem e = {.fase=PONT_ROL_INICIO,.eixoY=1,
                       .x=(g.lista.x+20)*s,.y=(g.lista.y+20)*s};
  if (toqueEst.maximo > 0) {
    assert(toqueEstRolar(&e)); int f = estFoco;
    e.fase = PONT_ROL_MOVER; e.delta = -13.5f * s;
    assert(toqueEstRolar(&e)); perto(estRolY, 13.5f); assert(estFoco == f);
    desenharTeste(); perto(estRolY, 13.5f);
    e.fase = PONT_ROL_INERCIA; e.delta = -10000;
    toqueEstRolar(&e); perto(estRolY, toqueEst.maximo); desenharTeste();
    assert(!toqueEstRolar(&e));
    /* Captured scale follows the last drawing, even after the setting changes. */
    float antes = estRolY; ui = s == 1.5f ? 1.2f : 1.5f;
    e.fase = PONT_ROL_MOVER; e.delta = 7.25f * s;
    assert(toqueEstRolar(&e)); perto(estRolY, antes - 7.25f); ui = s;
    e.fase = PONT_ROL_INICIO; e.y = (g.lista.y - 10) * s; assert(!toqueEstRolar(&e));
    e.y = (g.lista.y + 20) * s; e.eixoY = 0; assert(!toqueEstRolar(&e));
    /* Rotation clamps the free offset without pulling it back to focus. */
    nv_layout_w = h > w ? 2340 : 1080; nv_layout_h = h > w ? 1080 : 1920;
    ui = escala = 1; desenharTeste();
    assert(estRolY >= 0 && estRolY <= toqueEst.maximo); assert(estFoco == f);
  }
}
#endif

int main(void) {
#ifdef NV_TOUCH_UI
  float tamanhos[][2] = {{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  float escalas[] = {1,1.2f,1.3f,1.5f};
  for (int f = 0; f < 2; f++) { fonte = f ? 1.3f : 1;
    for (int i = 0; i < 4; i++) for (int s = 0; s < 4; s++) for (int n = 4; n <= 7; n += 3)
      telefone(tamanhos[i][0], tamanhos[i][1], escalas[s], n);
  }
  testMobile = 0; nv_layout_w = 1920; nv_layout_h = 1080; assert(!telefoneui_ativo());
#endif
  /* The original TV/tablet geometry and size controls keep their dimensions. */
  fonte = 1; ui = escala = 1; validar = 0; montarTeste(); desenharTeste();
  perto(alvos[1].w, EST_W); perto(alvos[1].x, (1920 - EST_W) * .5f);
  for (int k = 0; k < nAlvos; k++) if (alvos[k].b > 0) { perto(alvos[k].w, EST_SEG_W); perto(alvos[k].h, EST_SEG_H); }
  puts("ctxmenu_retrato: phone dialog drawing, shapes/sizes, clipped targets, fractional scroll and TV geometry PASS");
  return 0;
}
