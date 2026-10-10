/* Real GL/font card captures. The native editor state is injected through
 * real sistexto; this fixture does not draw or prove Android's EditText/IME.
 * The emulator smoke originals cover that platform overlay separately. */
#include "../src/teclado.c"
#include "dados.h"
#include "tex_cache.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static SDL_Window *janela;
static const char *saida;
static unsigned quadros, conferencias;
static Uint32 relogio = 1000;
static Uint32 agoraPonteiro(void) { return relogio; }

/* Independent Latin-word packing for these complete German/caller labels.
 * Every token and final line must have real glyphs inside its text budget;
 * the measured production block must contain every line, without a cap. */
static float linhaCompleta(TxtEstilo estilo, const char *s, float w) {
  TxtLinha t = txt_linha(estilo, s, 243, 242, 239, 255);
  assert(t.tex && t.w > 0 && t.h > 0 && t.w <= w + .15f);
  return t.h;
}
static float blocoCompleto(TxtEstilo estilo, const char *s, float w, float passo, float *glifos) {
  const char *p = i18n(s);
  char linha[512] = "", palavra[512], tentativa[512];
  float h = 0, fim = 0;
  while (*p) {
    while (*p == ' ') p++;
    if (!*p) break;
    const char *ini = p;
    while (*p && *p != ' ' && *p != '\n') p++;
    size_t n = (size_t)(p - ini);
    assert(n && n < sizeof palavra);
    memcpy(palavra, ini, n); palavra[n] = 0;
    assert(txt_largura(estilo, palavra) <= w + .15f);
    assert(strlen(linha) + n + 2 < sizeof tentativa);
    snprintf(tentativa, sizeof tentativa, "%s%s%s", linha, linha[0] ? " " : "", palavra);
    if (linha[0] && txt_largura(estilo, tentativa) > w) {
      fim = fmaxf(fim, h + linhaCompleta(estilo, linha, w)); h += passo;
      snprintf(linha, sizeof linha, "%s", !strcmp(palavra, "·") ? "" : palavra);
    } else snprintf(linha, sizeof linha, "%s", tentativa);
    if (*p == '\n') {
      if (linha[0]) fim = fmaxf(fim, h + linhaCompleta(estilo, linha, w));
      h += passo; linha[0] = 0; p++;
    }
  }
  if (linha[0]) { fim = fmaxf(fim, h + linhaCompleta(estilo, linha, w)); h += passo; }
  float medido = tePhoneTexto(estilo, s, w, passo);
  assert(fabsf(medido - h) < .15f);
  if (glifos) *glifos = fim;
  return h;
}
static void contextoCompleto(TePhoneLayout l) {
  float kg, tg, dg;
  float k = blocoCompleto(TXT_CAPTION, kickerAtual, l.w, 28, &kg);
  float titulo = blocoCompleto(TXT_ILHA_TITULO, tituloAtual, l.w, 48, &tg);
  float dica = blocoCompleto(TXT_ILHA_CORPO, dicaAtual, l.w, 30, &dg);
  assert(fabsf(l.tituloY - (kickerAtual[0] ? k + 8 : 0)) < .15f);
  assert(fabsf(l.dicaY - l.tituloY - titulo - 10) < .15f);
  assert(l.kickerY + kg <= l.tituloY + .15f);
  assert(l.tituloY + tg <= l.dicaY + .15f);
  float cabecalho = l.tituloY + titulo + (dicaAtual[0] ? dica + 10 : 0);
  assert(l.controles[0].id == TE_P_CAMPO && fabsf(l.controles[0].r.y - cabecalho - 24) < .15f);
  assert(l.dicaY + dg <= l.controles[0].r.y + .15f);
  for (int i = 0; i < l.n; i++) {
    TePhoneControle c = l.controles[i];
    if (c.id == TE_P_CAMPO) continue;
    float glifos, h = blocoCompleto(TXT_ILHA_SEG, c.rotulo, c.r.w - 32, 28, &glifos);
    assert(c.r.h >= h + 28 - .15f);
    assert((c.r.h - h) * .5f + glifos <= c.r.h + .15f);
    if (c.id == TE_P_PRONTO || c.id == TE_P_CANCELAR)
      assert(c.r.y >= l.corpo.y + l.corpo.h + 20 - .15f && c.r.y + c.r.h <= l.painel.y + l.painel.h - 24 + .15f);
  }
  if (celOk()) {
    float glifos, h = blocoCompleto(TXT_ILHA_CORPO, TE_FRASE_CEL, l.w, 30, &glifos);
    assert(glifos <= h + 12 + .15f);
  }
  float glifos, aviso = blocoCompleto(TXT_ILHA_CORPO, l.aviso, l.w, 30, &glifos);
  assert(fabsf(l.total - l.avisoY - aviso) < .15f);
  assert(l.avisoY + glifos <= l.total + .15f);
}

static void conferir(void) {
  float e = gfx_escala_ui();
  TePhoneLayout l = tePhoneMedir();
  assert(l.painel.x >= 0 && l.painel.y >= 0 && l.corpo.w > 0 && l.corpo.h > 0);
  assert((l.painel.x + l.painel.w) * e <= nv_layout_w + .15f);
  assert((l.painel.y + l.painel.h) * e <= nv_layout_h + .15f);
  assert(l.total >= 0 && tePhoneOffset >= 0 && tePhoneOffset <= l.maximo + .15f);
  const PonteiroAlvo *v;
  int n = ponteiro_teste_lista(&v), acoes = 0;
  for (int i = 0; i < n; i++) {
    assert(v[i].focar != focarTecla && v[i].focar != focarBarra);
    if (v[i].ativar != tePhoneAcao) continue;
    acoes++;
    assert(v[i].x >= 0 && v[i].y >= 0);
    assert(v[i].x + v[i].w <= nv_layout_w + .15f && v[i].y + v[i].h <= nv_layout_h + .15f);
    if (v[i].a == TE_P_PRONTO || v[i].a == TE_P_CANCELAR) {
      assert(fabsf(v[i].y - l.rodape.y * e) < .15f);
      assert(fabsf(v[i].h - l.rodape.h * e) < .15f);
    } else {
      assert(v[i].y >= l.corpo.y * e - .15f);
      assert(v[i].y + v[i].h <= (l.corpo.y + l.corpo.h) * e + .15f);
    }
  }
  if (l.nativo) {
    assert(acoes == 0 && n == 1 && !v[0].focar && !v[0].ativar);
    assert(v[0].x == 0 && v[0].y == 0 && fabsf(v[0].w - nv_layout_w) < .15f && fabsf(v[0].h - nv_layout_h) < .15f);
  }
  else assert(acoes >= 2);  // fixed footer survives every body scroll
  conferencias++;
}
static void quadro(void) {
  relogio += 17; txt_pendentes = 0;
  SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
  ponteiro_quadro(relogio);
  teclado_atualizar(1.0f / 60, SDL_GetTicks());
  celb_atualizar(1.0f / 60);
  glClearColor(.025f, .025f, .03f, 1); glClear(GL_COLOR_BUFFER_BIT);
  float anterior = gfx_escala();
  int retangulos = gfx_n_rect;
  teclado_desenhar(SDL_GetTicks());
  assert(gfx_escala() == anterior);
  if (tePhoneNativo() && anim >= .01f) assert(gfx_n_rect - retangulos == 1); // only full-canvas dim
  ponteiro_desenhar(); conferir(); quadros++;
}
static void foto(const char *nome) {
  char caminho[1024];
  for (int i = 0; i < 90; i++) { quadro(); SDL_GL_SwapWindow(janela); }
  quadro(); glFinish();
  if (!tePhoneNativo()) {
    float escalaAnt = gfx_escala_entrar();
    contextoCompleto(tePhoneMedir());
    gfx_escala_sair(escalaAnt);
  }
  assert(!txt_pendentes);
  snprintf(caminho, sizeof caminho, "%s-%s.bmp", saida, nome);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  assert(s && SDL_SaveBMP(s, caminho) == 0); SDL_FreeSurface(s);
}
static void abrir(const char *kicker, const char *titulo, const char *dica,
                  const char *alfabeto, const char *valor, int tipo) {
  teclado_contexto(kicker);
  teclado_abrir_com(titulo, dica, 120, alfabeto, valor);
  teclado_tipo(tipo);
  assert(teTelefoneIme() && teclado_aberto());
}
static void editor(void) { tePhoneAcao(TE_P_CAMPO, 0); assert(tePhoneNativo()); }
static void voltar(const char *valor) {
  char evento[TECLADO_LONGO + 2]; snprintf(evento, sizeof evento, "T%s", valor);
  st_teste_evento(evento); st_teste_evento("X"); quadro();
  assert(!tePhoneNativo() && teclado_aberto() && teclado_resultado() == TECLADO_NADA);
  assert(!strcmp(teclado_texto(), valor));
}
static void dedo(Uint32 tipo, float x, float y) {
  SDL_Event e = {0}; e.type = tipo; e.tfinger.touchId = 1; e.tfinger.fingerId = 1;
  e.tfinger.x = x / nv_layout_w; e.tfinger.y = y / nv_layout_h;
  assert(ponteiro_evento(&e, teclado_evento));
}
static void rodape(void) {
  quadro();
  char antes[TECLADO_LONGO + 1]; snprintf(antes, sizeof antes, "%s", teclado_texto());
  int estado = st_estado(), donoAntes = st_dono();
  for (int i = 0; i < 24 && tePhoneOffset < tePhoneMedir().maximo - .15f; i++) {
    TePhoneLayout l = tePhoneMedir(); float e = gfx_escala_ui();
    float x = (l.corpo.x + l.corpo.w * .5f) * e,
          baixo = (l.corpo.y + l.corpo.h) * e - 20, alto = l.corpo.y * e + 20;
    dedo(SDL_FINGERDOWN, x, baixo); quadro();
    dedo(SDL_FINGERMOTION, x, alto); quadro();
    dedo(SDL_FINGERUP, x, alto); quadro();
    assert(!strcmp(antes, teclado_texto()) && teclado_aberto() && teclado_resultado() == TECLADO_NADA);
    assert(st_estado() == estado && st_dono() == donoAntes);
  }
  assert(fabsf(tePhoneOffset - tePhoneMedir().maximo) < .15f);
  printf("actual pointer reached body end %.2f of %.2f without action\n", tePhoneOffset, tePhoneMedir().maximo);
}

int main(int argc, char **argv) {
  const char *dir = getenv("NUVIO_DADOS"), *escala = getenv("NUVIO_TAMANHO_UI");
  assert(dir && *dir); saida = argc > 1 ? argv[1] : "/tmp/teclado-phone";
  char caminho[1024]; snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  FILE *f = fopen(caminho, "w"); assert(f);
  /* German wraps several real optional action labels. */
  fputs("idioma 6\nidiomaAutoLocal 0\n", f); fclose(f);
  dados_iniciar(dir); ajustes_iniciar(); ajustes_dir(dir);
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0); IMG_Init(IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio phone input card", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(janela && SDL_GL_CreateContext(janela)); SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1)); tex_iniciar(16); gfx_icones_dir("deploy/app/art");
  gfx_escala_ui_definir(escala ? strtof(escala, NULL) : 1);
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  ponteiro_teste_relogio(agoraPonteiro);
  ponteiro_teste_janela((int)nv_layout_w, (int)nv_layout_h);
  st_teste_ligar(1); teclado_teste_modos(1, 1, 1);

  const char *email = "abcdefghijklmnopqrstuvwxyz0123456789@._-+";
  abrir("Conta Nuvio", "E-mail", "O e-mail da sua conta Nuvio.", email,
        "touch-preview@example.invalid", TECLADO_TIPO_EMAIL);
  editor(); foto("email-before-native-X");
  voltar("touch-preview@example.invalid"); foto("email-after-native-X");
  rodape(); foto("email-shortcuts");
  tePhoneAcao(TE_P_CANCELAR, 0); assert(teclado_resultado() == TECLADO_CANCELOU);
  assert(teclado_resultado() == TECLADO_NADA);

  abrir("Conta Nuvio", "Senha", "A senha da sua conta Nuvio. Ela não aparece na tela.",
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,_-@!#$%&*+=?/", "segredo123", TECLADO_TIPO_SENHA);
  editor(); foto("password-before-native-X"); voltar("segredo123"); foto("password-after-native-X");
  rodape(); foto("password-actions"); tePhoneAcao(TE_P_CANCELAR, 0); (void)teclado_resultado();

  txt_definir_fonte_interface(TXT_FAMILIA_MONTSERRAT);
  abrir("Fontes e addons · Debrid", "Chave do Real-Debrid", "Sua chave em real-debrid.com/apitoken. Vazio apaga.",
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.", "AbC123xyz", TECLADO_TIPO_TEXTO);
  foto("key-context"); rodape(); foto("key-optional-actions");
  tePhoneAcao(TE_P_CANCELAR, 0); (void)teclado_resultado();

  abrir("Contas e serviços · Ajustes de conta e fontes para a biblioteca", "Um título de entrada longo para conferir todo o contexto da modal",
        "Esta dica longa continua inteira e pode rolar antes de concluir a entrada. Os botões de concluir e cancelar permanecem disponíveis.",
        "abcdefghijklmnopqrstuvwxyz0123456789", "valor123", TECLADO_TIPO_TEXTO);
  foto("long-context"); rodape(); foto("long-actions");
  tePhoneAcao(TE_P_PRONTO, 0); assert(teclado_resultado() == TECLADO_PRONTO && teclado_resultado() == TECLADO_NADA);
  printf("PASS: real phone input card GL/fonts, %u frames, %u bounds checks; native overlay separately required\n", quadros, conferencias);
  return 0;
}
