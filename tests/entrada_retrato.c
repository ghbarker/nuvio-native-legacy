#define NV_TOUCH_PREVIEW 1
#if defined(TESTE_LOGIN)
#include "../src/login.c"
#elif defined(TESTE_TECLADO)
#include "../src/teclado.c"
#else
#error escolha TESTE_LOGIN ou TESTE_TECLADO
#endif
#include <assert.h>

float nv_layout_w = 1080, nv_layout_h = 2340;
static int larguraTitulo = 700, blocos;
static float desenhadoX, blocoW;
const char *i18n(const char *s) { return s; }
int txt_largura(TxtEstilo estilo, const char *s) {
  return estilo == TXT_TITULO1 ? larguraTitulo : (int)strlen(s) * 10;
}
TxtLinha txt_linha(TxtEstilo estilo, const char *s, int r, int g, int b, int a) {
  (void)r; (void)g; (void)b; (void)a;
  return (TxtLinha){0, txt_largura(estilo, s), 28, 0, 0};
}
TxtLinha txt_linha_corta(TxtEstilo estilo, const char *s, int r, int g, int b, int a, float w) {
  TxtLinha l = txt_linha(estilo, s, r, g, b, a);
  if (l.w > w) l.w = (int)w;
  return l;
}
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) {
  (void)l; (void)y; (void)a; desenhadoX = x;
}
float txt_bloco_corta(TxtEstilo estilo, const char *s, int r, int g, int b,
                       float x, float y, float w, float leading, float a, int maxLinhas) {
  (void)estilo; (void)s; (void)r; (void)g; (void)b; (void)y; (void)a;
  assert(maxLinhas == 2); blocos++; desenhadoX = x; blocoW = w;
  return leading * 2;
}
#if defined(TESTE_TECLADO)
int st_ime_disponivel(void) { return 0; }
int st_voz_disponivel(void) { return 0; }
int celb_disponivel(void) { return 0; }
int celb_embutido(int dono) { (void)dono; return 0; }
float celb_embutido_altura(int dono, float w, float h) { (void)dono; (void)w; (void)h; return 0; }
float txt_bloco(TxtEstilo estilo, const char *s, int r, int g, int b,
                 float x, float y, float w, float leading, float a, int maxLinhas) {
  (void)estilo; (void)r; (void)g; (void)b; (void)x; (void)y; (void)w; (void)a;
  return s[0] ? leading * maxLinhas : 0;
}
static void confereGrade(void) {
  teMedir();
  assert(teX() >= 0 && teX() + teW() <= NV_TELA_W);
  assert(teY() >= 0 && teY() + teH() <= NV_TELA_H);
  assert(teGradeY() >= teY() + TE_ILHA_PY + teEsqH());
  for (int f = 0; f < nFileiras; f++) for (int c = 0; c < colunasDe(f); c++) {
    GfxRect r = retangulo(f, c);
    assert(r.w > 0 && r.x >= teX() && r.x + r.w <= teX() + teW() + .01f);
    assert(r.y >= teGradeY() && r.y + r.h <= teY() + teH());
  }
}
static void selecionar(char ch) {
  for (int f = 0; f < fileirasChar(); f++) for (int c = 0; c < nCel[camada][f]; c++)
    if (celula[camada][f][c] == ch) { fileira = voltaF = f; coluna = voltaC = c; return; }
  assert(!"caractere ausente");
}
static void confereAlfabeto(const char *a, int email) {
  unsigned char visto[256] = {0};
  alfabetoAtual = a; teMontar(email); maxN = TECLADO_LONGO;
  confereGrade();
  for (int cam = 0; cam <= (emCamadas ? 1 : 0); cam++) {
    camada = cam;
    for (int cx = 0; cx <= (emCamadas ? 1 : 0); cx++) {
      caixa = cx;
      for (int f = 0; f < fileirasChar(); f++) for (int c = 0; c < nCel[cam][f]; c++) {
        char ch = teCaixa(celula[cam][f][c]);
        assert(strchr(a, ch)); visto[(unsigned char)ch] = 1;
      }
    }
  }
  if (emCamadas) for (int i = 0; i < nTeclasCam; i++) if (teclaCam[i] == TE_K_ESPACO) visto[' '] = 1;
  for (const unsigned char *p = (const unsigned char *)a; *p; p++) assert(visto[*p]);
}
#endif

int main(void) {
#if defined(TESTE_LOGIN)
  assert(LG_CAMPO_W < NV_TELA_W - 2 * NV_MARGEM_X);
  assert(LG_EMAIL_PILL_W < LG_CAMPO_W);
  assert(LG_QR_LADO + 32 < NV_TELA_W - 2 * NV_MARGEM_X);
  assert(tituloLogin(118) == 118 && !blocos && desenhadoX > 0);
  larguraTitulo = 1500;
  assert(tituloLogin(118) == 214 && blocos == 1);
  assert(desenhadoX == NV_MARGEM_X && blocoW == NV_TELA_W - 2 * NV_MARGEM_X);
  nv_layout_w = 2400; nv_layout_h = 1080;
  assert(LG_BLOCO_W == 1100 && tituloLogin(118) == 118 && blocos == 1);
  assert(desenhadoX > 0);
#else
  const char *senha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,_-@!#$%&*+=?/\\:;'\"()[]{}<>^~`|";
  const char *email = "abcdefghijklmnopqrstuvwxyz0123456789@._-+";
  char longo[81];
  for (int i = 0; i < 80; i++) longo[i] = (char)('A' + i % 26);
  longo[80] = 0;
  confereAlfabeto(longo, 0);
  int total = 0;
  for (int f = 0; f < fileirasChar(); f++) total += nCel[0][f];
  assert(total == 80 && nCols == 10 && nFileiras == 9);
  confereAlfabeto(ALFABETO, 0);
  confereAlfabeto(email, 1); assert(nCols == 10 && nAtalhos == 4);
  selecionar('@'); strcpy(texto, "user@"); n = 5;
  nv_layout_w = 2400; nv_layout_h = 1080; teRefluir(); teMedir();
  assert(nCols == 13 && celula[camada][fileira][coluna] == '@');
  assert(celula[camada][voltaF][voltaC] == '@' && !strcmp(texto, "user@"));
  fileira = nFileiras - 1; coluna = 2;
  nv_layout_w = 1080; nv_layout_h = 2340; teRefluir();
  assert(fileira == nFileiras - 1 && coluna == 2 && nAtalhos == 4);
  confereGrade();
  confereAlfabeto(senha, 0); camada = 0; caixa = 2; mascarar = ehSenha = 1;
  selecionar('z'); nv_layout_w = 2400; nv_layout_h = 1080; teRefluir();
  assert(teCaixa(celula[camada][fileira][coluna]) == 'Z' && caixa == 2 && mascarar);
  fileira = -1; coluna = TE_B_IME;
  nv_layout_w = 1080; nv_layout_h = 1920; teRefluir();
  assert(fileira == -1 && coluna == TE_B_IME && caixa == 2 && mascarar);
  confereGrade();
#endif
  puts("entrada_retrato: OK");
  return 0;
}
