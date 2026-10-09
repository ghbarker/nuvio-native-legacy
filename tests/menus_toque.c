/* Offsets e alvos das telas reais, sem janela, rede ou persistencia. */
#ifdef _WIN32
#include <time.h>
static struct tm *menus_localtime_r(const time_t *t, struct tm *out) {
  struct tm *r = localtime(t);
  if (r) *out = *r;
  return r ? out : NULL;
}
#define localtime_r menus_localtime_r
#endif
#define NV_TOUCH_PREVIEW 1
#if defined(TESTE_AJUSTES)
#include "../src/ajustes.c"
#elif defined(TESTE_MENU)
#include "../src/menu.c"
#elif defined(TESTE_PERFILSEL)
#include "../src/perfilsel.c"
#elif defined(TESTE_PERFIL)
#include "../src/perfil.c"
#else
#error escolha uma tela TESTE_*
#endif
#include <assert.h>

float nv_layout_w = 2400.0f;
static void perto(float real, float esperado) { assert(fabsf(real - esperado) < 0.01f); }

#if defined(TESTE_AJUSTES)
static float escalaTeste = 1.5f;
static PonteiroRolagemFn rolarRegistrado;
static int fileirasTeste[5] = {0, 1, 2, 3, 4}, fileirasParadasTeste, movimentosTeste, remontagensTeste;
float gfx_escala(void) { return escalaTeste; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { rolarRegistrado = fn; }
int selospacote_n(void) { return 0; }
int txt_largura(TxtEstilo estilo, const char *s) { (void)estilo; return (int)strlen(s) * 10; }
const char *i18n(const char *s) { return s; }
const char *atualizacao_nova(void) { return NULL; }
const char *fil_linha_addon(int i) { return i < 2 ? "addon A" : "addon B"; }
int fil_n(void) { return 5; }
int fil_estado(int i) { return i == 1 ? FIL_FORA : FIL_NA_HOME; }
int fil_linha_origem(int i) { (void)i; return FIL_ORIGEM_CATALOGO; }
const char *fil_titulo(int i) { (void)i; return "Fileira"; }
int fil_mover(int i, int dir) {
  int j = i + dir; movimentosTeste++;
  if (fileirasParadasTeste) return i;
  while (j >= 0 && j < 5 && fil_estado(j) == FIL_FORA) j += dir;
  if (j < 0 || j >= 5) return i;
  int t = fileirasTeste[i]; fileirasTeste[i] = fileirasTeste[j]; fileirasTeste[j] = t;
  return j;
}
int fil_mover_grupo(int i, int dir) { return fil_mover(i, dir); }
int fil_limite(void) { return 10; }
int fil_linha_oculta(int i) { (void)i; return 0; }
int fil_linha_vista(int i) { (void)i; return 0; }
int fil_linha_na_home(int i) { (void)i; return 1; }
void desc_remontar_fileiras(void) { remontagensTeste++; }
void desc_repetir(void) { assert(!"unexpected network request"); }
#elif defined(TESTE_MENU)
int ajustes_home_layout(void) { return HOME_LAYOUT_MODERNA; }
#elif defined(TESTE_PERFIL)
static SvAmigo amigosTeste[3];
float ajustes_conteudo_x(void) { return 104.0f; }
int socialvis_n_amigos(void) { return 3; }
const SvAmigo *socialvis_amigo(int i) { return i >= 0 && i < 3 ? &amigosTeste[i] : NULL; }
int socialvis_perfil(const char *id, SvPerfil *p) { (void)id; memset(p, 0, sizeof *p); return 1; }
unsigned socialvis_revisao(void) { return 1; }
void socialvis_abrir_perfil(const char *id) { (void)id; }
Uint32 SDL_GetTicks(void) { return 500; }
#endif

int main(void) {
  PonteiroRolagem e = { PONT_ROL_INICIO, 1, 0, 0, 300, 450 };
#if defined(TESTE_AJUSTES)
  float esquerdo = 0, direito = 0;
  int antes = valor[AJ_TEMA];
  ajToqueLimpar(); ajToqueCamada();
  ajToqueRegistrar(AJT_MENU, (GfxRect){100, 200, 200, 300}, &esquerdo, 600, 0, 1);
  ajToqueRegistrar(AJT_LISTA, (GfxRect){400, 200, 300, 300}, &direito, 900, 0, 2);
  assert(rolarRegistrado == ajToqueRolar && rolarRegistrado(&e));
  assert(ajToqueAtivo == AJT_MENU);
  escalaTeste = 1.0f; /* A escala do desenho ja terminou no fio de eventos. */
  e.fase = PONT_ROL_MOVER; e.delta = -37.5f;
  assert(ajToqueRolar(&e)); perto(esquerdo, 25); perto(direito, 0);
  assert(valor[AJ_TEMA] == antes);
  e.fase = PONT_ROL_SOLTAR; ajToqueRolar(&e);
  e.fase = PONT_ROL_INERCIA; e.delta = -75; ajToqueRolar(&e); perto(esquerdo, 75);
  e.fase = PONT_ROL_FIM; ajToqueRolar(&e); assert(ajToqueLivre[AJT_MENU]);
  ajToqueCamada(); escalaTeste = 1.5f;
  ajToqueRegistrar(AJT_MENU, (GfxRect){100, 200, 200, 300}, &esquerdo, 600, 0, 1);
  perto(esquerdo, 75); /* O foco nao puxa a lista de volta depois da soltura. */
  e.fase = PONT_ROL_INICIO; assert(ajToqueRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -1e6f; ajToqueRolar(&e); perto(esquerdo, 600);
  e.fase = PONT_ROL_INERCIA; assert(!ajToqueRolar(&e));
  e.delta = 1e6f; ajToqueRolar(&e); perto(esquerdo, 0);
  e.fase = PONT_ROL_CANCELAR; ajToqueRolar(&e); assert(ajToqueAtivo == -1);
  e.fase = PONT_ROL_INICIO; e.eixoY = 0; assert(!ajToqueRolar(&e));
  e.eixoY = 1; e.x = 30; assert(!ajToqueRolar(&e));
  e.x = 300; assert(ajToqueRolar(&e));
  ajToqueCamada();
  ajToqueRegistrar(AJT_MENU, (GfxRect){100, 200, 200, 300}, &esquerdo, 600, 123, 3);
  assert(ajToqueAtivo == -1 && !ajToqueLivre[AJT_MENU]); perto(esquerdo, 123);
  e.fase = PONT_ROL_MOVER; e.delta = -100; assert(!ajToqueRolar(&e)); perto(esquerdo, 123);
  ajToqueCamada(); assert(!ajToqueRolar(&e));
  ajToqueLimpar(); assert(!ajToqueLivre[AJT_LISTA]);

  filListaN = 4; for (int i = 0; i < 4; i++) filLista[i] = i;
  filAba = 0; filSep = 2;
  perto(ajFilToqueTopo(4), 4 * 70.0f + 50.0f);
  filAba = 1; filForaAgrupada = 1;
  perto(ajFilToqueTopo(4), 4 * 76.0f + 2 * 50.0f);
  filForaAgrupada = 0; perto(ajFilToqueTopo(4), 4 * 76.0f);
  filAberta = 1; filPegou = 0; filNaBarra = 1;
  ajFilToqueFocar(2, 3); assert(filFoco == 2 && filCampo == 3 && !filNaBarra);
  filPegou = 1; ajFilToqueFocar(0, 1); assert(filFoco == 2 && filCampo == 3);
  /* Pick and tap a destination: skip the hidden row, move, then drop. */
  filAba = 0; filFoco = 1; filPegou = 1; filPegouDe = 0; filMontarLista();
  ajFilToqueSoltar(4, 0);
  assert(filFoco == 4 && !filPegou && fileirasTeste[4] == 0 && movimentosTeste == 3 && remontagensTeste == 1);
  filPegou = 1; filPegouDe = 4; ajFilToqueSoltar(1, 0);
  assert(filFoco == 1 && !filPegou && fileirasTeste[0] == 0 && movimentosTeste == 6 && remontagensTeste == 2);
  filPegou = 1; filPegouDe = 0; fileirasParadasTeste = 1;
  ajFilToqueSoltar(4, 0); assert(!filPegou && filFoco == 1 && movimentosTeste == 7);
  filPegou = 1; ajFilToqueSoltar(0, 0); assert(filPegou && movimentosTeste == 7); /* Hero is fixed. */

  uxIndice = 2; uxOp = AJ_TEMA; uxEditor = 1; uxRestaurar = uxAvisoRisco = 0;
  ajPontInline = 1; uxPendente = 0;
  ajToqueInlineValor(2, AJ_TEMA); assert(uxPendente == 2 && valor[AJ_TEMA] == antes);
  ajToqueInlineValor(-1, AJ_TEMA); assert(uxPendente == 2);
  ajToqueInlineValor(nValores(AJ_TEMA), AJ_TEMA); assert(uxPendente == 2);
  uxAvisoRisco = 1; ajToqueInlineValor(3, AJ_TEMA); assert(uxPendente == 2);
  perto(NV_VTELA_W, 2400.0f / ajustes_tamanho_ajustes());
#elif defined(TESTE_MENU)
  aberto = 1; tvToqueRegiao = (GfxRect){100, 200, 500, 600};
  tvToqueEscala = 1.5f; tvToqueMaximo = 500; tvRolar = 0; tvRolarV = 10;
  buscaOk = buscaLongo = 1; linha = MENU_INICIO;
  assert(tvToqueRolar(&e) && !buscaOk && !buscaLongo); perto(tvRolarV, 0);
  e.fase = PONT_ROL_MOVER; e.delta = -37.5f; tvToqueRolar(&e); perto(tvRolar, 25);
  assert(linha == MENU_INICIO);
  e.fase = PONT_ROL_FIM; tvToqueRolar(&e); assert(tvToqueLivre);
  e.fase = PONT_ROL_MOVER; e.delta = -1e6f; tvToqueRolar(&e); perto(tvRolar, 500);
  e.fase = PONT_ROL_INERCIA; assert(!tvToqueRolar(&e));
  e.delta = 1e6f; tvToqueRolar(&e); perto(tvRolar, 0);
  tvToqueLimpar(); assert(!tvToqueLivre);
  e.fase = PONT_ROL_INICIO; e.x = 20; assert(!tvToqueRolar(&e));
  e.x = 300; e.eixoY = 0; assert(!tvToqueRolar(&e));
  e.eixoY = 1; aberto = 0; assert(!tvToqueRolar(&e));
  perto(NV_TELA_W, 2400.0f / NV_MENU_ESCALA);
#elif defined(TESTE_PERFILSEL)
  e.eixoY = 0; psToqueRegiao = (GfxRect){100, 200, 500, 600};
  psToqueMaximo = 600; psToqueX = 0; preparando = 0; pinDe = -1;
  foco = concluido = 0; assert(psToqueRolar(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37; psToqueRolar(&e); perto(psToqueX, 37);
  assert(!foco && !concluido);
  e.fase = PONT_ROL_FIM; psToqueRolar(&e); assert(psToqueLivre);
  e.fase = PONT_ROL_MOVER; e.delta = -1e6f; psToqueRolar(&e); perto(psToqueX, 600);
  e.fase = PONT_ROL_INERCIA; assert(!psToqueRolar(&e));
  e.delta = 1e6f; psToqueRolar(&e); perto(psToqueX, 0);
  e.fase = PONT_ROL_INICIO; preparando = 1; assert(!psToqueRolar(&e));
  preparando = 0; pinDe = 2; assert(!psToqueRolar(&e));
  pinDe = -1; psToqueMaximo = 0; assert(!psToqueRolar(&e));
#elif defined(TESTE_PERFIL)
  modo = PF_DUELO; temDados = 1; amigo = 0; dLinha = 0;
  for (int i = 0; i < 3; i++) snprintf(amigosTeste[i].id, sizeof amigosTeste[i].id, "friend%d", i);
  toqueDueloAmigo(1, 0); assert(amigo == 1 && !pedirAmigo);
  toqueDueloAmigo(1, 0); assert(pedirAmigo && dLinha == 0);
  pedirAmigo = 0; toqueDueloAmigo(8, 0); assert(amigo == 1 && !pedirAmigo);
  dPerf.nGostou = 3; toqueDueloCartaz(1, 0); assert(dLinha == 1 && dCol == 1);
  toqueDueloCartaz(9, 0); assert(dCol == 1);
  modo = PF_RESUMO; toqueDueloCartaz(0, 0); assert(dCol == 1);
  carregando = 1; toquePerfilRepetir(0, 0); assert(!pedirAtualizar);
  carregando = 0; toquePerfilRepetir(0, 0); assert(pedirAtualizar);
#endif
  puts("menus_toque: OK");
  return 0;
}
