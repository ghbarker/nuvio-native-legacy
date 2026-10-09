/* Roteamento e ordem de camadas reais do app, com desenho inerte. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/app.c"
#include <assert.h>

float nv_layout_w = 2400.0f;
static int loginOk = 1, perfilOk = 1, mini, player, detalhe, lista, contexto;
static int menu, menuSobre, modalTela, modalDetalhe, modalLista, abertura = -1;
enum { TESTE_CENTRAL, TESTE_ILHA, TESTE_CELULAR, TESTE_SALVOS, TESTE_REGISTRO,
       TESTE_SPOTLIGHT, TESTE_TECLADO, TESTE_INTRO, TESTE_GUIA20,
       TESTE_N201, TESTE_N202, TESTE_NOVCARTAO, TESTE_BLOQUEIOS_N };
static int bloqueios[TESTE_BLOQUEIOS_N];
static PonteiroBordaFn borda;
static float bordaLargura;
static PonteiroAlvo ultimoAlvo;
static int nAlvos;
int login_concluido(void) { return loginOk; }
int perfilsel_concluido(void) { return perfilOk; }
int player_mini_ativo(void) { return mini; }
int player_aberto(void) { return player; }
int detail_aberto(void) { return detalhe; }
int detail_cobre_tela(void) { return detalhe; }
int vertudo_aberta(void) { return lista; }
int ctx_aberto(void) { return contexto; }
int central_aberta(void) { return bloqueios[TESTE_CENTRAL]; }
int ilha_modal_aberto(void) { return bloqueios[TESTE_ILHA]; }
int celb_aberto(void) { return bloqueios[TESTE_CELULAR]; }
int spainel_aberto(void) { return bloqueios[TESTE_SALVOS]; }
int registro_aberto(void) { return bloqueios[TESTE_REGISTRO]; }
int spot_aberto(void) { return bloqueios[TESTE_SPOTLIGHT]; }
int teclado_aberto(void) { return bloqueios[TESTE_TECLADO]; }
int diagnostico_intro_aberto(void) { return bloqueios[TESTE_INTRO]; }
int novidades20_aberto(void) { return bloqueios[TESTE_GUIA20]; }
int novidades201_aberto(void) { return bloqueios[TESTE_N201]; }
int novidades202_aberto(void) { return bloqueios[TESTE_N202]; }
int novcartao_aberto(void) { return bloqueios[TESTE_NOVCARTAO]; }
int spainel_visivel(void) { return 0; }
int menu_aberto(void) { return menu; }
int menu_visivel(void) { return 1; }
int menu_sobre(void) { return menuSobre; }
void menu_abrir(void) { menu = 1; abertura = 0; }
void menu_abrir_sobre(int semRail) { menu = 1; menuSobre = 1; abertura = semRail ? 1 : 2; }
void menu_pilula_mostrar(float a) { (void)a; }
void menu_desenhar(Uint32 agora) { (void)agora; }
float home_topo_fracao(void) { return 1; }
void ponteiro_camada(void) { borda = NULL; nAlvos = 0; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  ultimoAlvo = (PonteiroAlvo){x, y, w, h, focar, ativar, a, b, 0, NULL}; nAlvos++;
}
void ponteiro_borda_esquerda(float largura, PonteiroBordaFn fn) { bordaLargura = largura; borda = fn; }
static void desenhoTela(Uint32 agora) { (void)agora; if (modalTela) ponteiro_camada(); }
#define DESENHO_INERTE(fn) void fn(Uint32 agora) { desenhoTela(agora); }
DESENHO_INERTE(home_desenhar)
DESENHO_INERTE(explorar_desenhar)
DESENHO_INERTE(guia_desenhar)
DESENHO_INERTE(busca_desenhar)
DESENHO_INERTE(biblioteca_desenhar)
DESENHO_INERTE(agendaui_desenhar)
DESENHO_INERTE(perfil_desenhar)
DESENHO_INERTE(amigoperfil_desenhar)
DESENHO_INERTE(addonsui_desenhar)
DESENHO_INERTE(pluginsui_desenhar)
DESENHO_INERTE(ajustes_desenhar)
DESENHO_INERTE(diagnostico_desenhar)
DESENHO_INERTE(livetvdiag_desenhar)
#undef DESENHO_INERTE
void detail_desenhar(Uint32 agora) { (void)agora; if (modalDetalhe) ponteiro_camada(); }
void vertudo_desenhar(Uint32 agora) { (void)agora; if (modalLista) ponteiro_camada(); }
void ctx_desenhar(Uint32 agora) { (void)agora; }

static void quadro(void) {
  Uint32 agora = 1000;
  ponteiro_camada();
  desenharAtrasDoPainel(&agora);
}
static void reiniciar(Tela t) {
  tela = t; loginOk = perfilOk = 1;
  mini = player = detalhe = lista = contexto = menu = menuSobre = 0;
  modalTela = modalDetalhe = modalLista = 0;
  memset(bloqueios, 0, sizeof bloqueios);
  abertura = -1; saiuPorEsquerda = 1; borda = NULL;
  nAlvos = 0;
}
int main(void) {
  /* Uma camada em animacao nao entrega OK generico nem conserva alvos de tras. */
  reiniciar(TELA_HOME); sidebarToqueRegistrar(0);
  ponteiro_alvo(0, 0, 100, 100, NULL, NULL, 7, 8);
  CAMADA_SE(1);
  assert(!borda && nAlvos == 1 && !ultimoAlvo.focar && !ultimoAlvo.ativar);
  assert(ultimoAlvo.x == 0 && ultimoAlvo.y == 0 && ultimoAlvo.w == nv_layout_w && ultimoAlvo.h == NV_TELA_H);
  assert(ultimoAlvo.a == 0 && ultimoAlvo.b == 0);
  CAMADA_SE(0); assert(nAlvos == 1);
  Tela telas[] = {TELA_HOME, TELA_EXPLORAR, TELA_BUSCA, TELA_BIBLIOTECA,
    TELA_PERFIL, TELA_AJUSTES, TELA_DIAGNOSTICO, TELA_SOCIAL, TELA_ADDONS,
    TELA_AGENDA, TELA_LIVETV_DIAG, TELA_PLUGINS};
  PonteiroBordaFn callbacks[sizeof telas / sizeof *telas];
  for (unsigned i = 0; i < sizeof telas / sizeof *telas; i++) {
    reiniciar(telas[i]); quadro(); assert(borda && bordaLargura == 120);
    callbacks[i] = borda;
    for (unsigned j = 0; j < i; j++) assert(callbacks[i] != callbacks[j]);
    borda(); assert(menu && abertura == 0 && !saiuPorEsquerda && tela == telas[i]);
  }
  reiniciar(TELA_HOME); detalhe = 1; quadro();
  assert(borda == sidebarToqueDetalhe); borda();
  assert(abertura == 1 && detalhe && tela == TELA_HOME && !saiuPorEsquerda);
  reiniciar(TELA_BIBLIOTECA); lista = 1; quadro();
  assert(borda == sidebarToqueLista); borda();
  assert(abertura == 2 && lista && tela == TELA_BIBLIOTECA);
  reiniciar(TELA_HOME); lista = detalhe = 1; quadro();
  assert(borda == sidebarToqueDetalhe);
  /* O registro vem antes do desenho: as camadas internas o eliminam. */
  reiniciar(TELA_AGENDA); modalTela = 1; quadro(); assert(!borda);
  reiniciar(TELA_AJUSTES); modalTela = 1; quadro(); assert(!borda);
  reiniciar(TELA_HOME); detalhe = modalDetalhe = 1; quadro(); assert(!borda);
  reiniciar(TELA_HOME); lista = modalLista = 1; quadro(); assert(!borda);
  reiniciar(TELA_HOME); contexto = 1; quadro(); assert(!borda);
  assert(nAlvos == 1 && !ultimoAlvo.focar && !ultimoAlvo.ativar);
  reiniciar(TELA_HOME); detalhe = contexto = 1; quadro(); assert(!borda);
  assert(nAlvos == 1 && !ultimoAlvo.focar && !ultimoAlvo.ativar);
  /* Mesmo um callback antigo confere se sua tela ainda esta na frente. */
  reiniciar(TELA_HOME); quadro(); PonteiroBordaFn antigo = borda;
  tela = TELA_BUSCA; antigo(); assert(!menu && abertura == -1);
  reiniciar(TELA_HOME); quadro(); antigo = borda;
  detalhe = 1; antigo(); assert(!menu);
  /* A modal pode ganhar o teclado antes do primeiro quadro de sua animacao. */
  for (int i = 0; i < TESTE_BLOQUEIOS_N; i++) {
    reiniciar(TELA_HOME); quadro(); antigo = borda;
    bloqueios[i] = 1; antigo(); assert(!menu && abertura == -1);
    quadro(); assert(!borda);
    reiniciar(TELA_HOME); detalhe = 1; quadro(); antigo = borda;
    bloqueios[i] = 1; antigo(); assert(!menu);
    reiniciar(TELA_HOME); lista = 1; quadro(); antigo = borda;
    bloqueios[i] = 1; antigo(); assert(!menu);
  }
  reiniciar(TELA_HOME); quadro(); antigo = borda;
  contexto = 1; antigo(); assert(!menu);
  Tela bloqueadas[] = {TELA_LOGIN, TELA_ESCOLHA_PERFIL, TELA_GUIA, TELA_PLAYER};
  for (unsigned i = 0; i < sizeof bloqueadas / sizeof *bloqueadas; i++) {
    reiniciar(bloqueadas[i]); quadro(); assert(!borda);
  }
  reiniciar(TELA_HOME); mini = 1; quadro(); assert(!borda);
  reiniciar(TELA_HOME); player = 1; quadro(); assert(!borda);
  reiniciar(TELA_HOME); menu = 1; quadro(); assert(!borda);
  reiniciar(TELA_HOME); loginOk = 0; quadro(); assert(!borda);
  reiniciar(TELA_HOME); perfilOk = 0; quadro(); assert(!borda);
  /* A Home sem catalogo usa a mesma porta de registro, sem depender de alvos. */
  reiniciar(TELA_HOME); homePronta = 0; sidebarToqueRegistrar(0);
  assert(borda == sidebarToqueHome); borda(); assert(menu && !homePronta);
  puts("app_sidebar_toque: OK");
  return 0;
}
