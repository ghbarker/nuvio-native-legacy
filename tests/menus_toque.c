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
#define NV_TOUCH_UI 1
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
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
void layout_modo_definir(int mobile) { testMobile = mobile != 0; }
float nv_layout_h = 1080.0f;
static void perto(float real, float esperado) { assert(fabsf(real - esperado) < 0.01f); }

#if defined(TESTE_AJUSTES)
static float escalaTeste = 1.5f;
static PonteiroRolagemFn rolarRegistrado;
static int fileirasTeste[5] = {0, 1, 2, 3, 4}, fileirasParadasTeste, movimentosTeste, remontagensTeste;
static GfxRect superficieTeste;
static float raioTeste;
static int sombrasTeste, superficiesTeste, fundosTeste;
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)cr; (void)cg; (void)cb; (void)ca; superficieTeste = r; raioTeste = raio; superficiesTeste++;
}
void gfx_sombra_sob(GfxRect s, float foco, float parx, float raio, float cr, float cg, float cb,
                     float ca, GfxRect painel, float raioPx, float alfaPainel) {
  (void)s; (void)foco; (void)parx; (void)raio; (void)cr; (void)cg; (void)cb;
  (void)ca; (void)painel; (void)raioPx; (void)alfaPainel; sombrasTeste++;
}
float gfx_vidro_opacidade(void) { return 1; }
void gfx_vidro_fosco(GfxRect r, float raio, float a) { (void)r; (void)raio; (void)a; }
void gfx_vidro_miolo(GfxRect r, float raio, float cr, float cg, float cb, float ca, float a) {
  (void)cr; (void)cg; (void)cb; (void)ca; (void)a; superficieTeste = r; raioTeste = raio; superficiesTeste++;
}
void gfx_luz_canto(GfxRect r, float raio, float cx, float cy, float alcance, float cr, float cg, float cb, float ca) {
  (void)r; (void)raio; (void)cx; (void)cy; (void)alcance; (void)cr; (void)cg; (void)cb; (void)ca;
}
float gfx_escala(void) { return escalaTeste; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { rolarRegistrado = fn; }
int cat_n(void) { return 0; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
int home_item_focado(HomeItem *out) { (void)out; return 0; }
void fundo_desenhar_modo(int modo, GfxRect area, float raioPx, const char *arteUrl, float a) {
  assert(modo == FUNDO_FROST); (void)area; (void)raioPx; (void)arteUrl; (void)a; fundosTeste++;
}
int selospacote_n(void) { return 0; }
int txt_largura(TxtEstilo estilo, const char *s) { (void)estilo; return (int)strlen(s) * 10; }
const char *i18n(const char *s) { return s; }
float txt_bloco(TxtEstilo estilo, const char *s, int r, int g, int b, float x, float y, float w, float h, float a, int maxLinhas) {
  (void)estilo; (void)s; (void)r; (void)g; (void)b; (void)x; (void)y; (void)w; (void)a; (void)maxLinhas;
  return h;
}
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
static void dentroAjustes(GfxRect r) {
  assert(r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0);
  assert(r.x + r.w <= NV_VTELA_W + 0.01f && r.y + r.h <= NV_VTELA_H + 0.01f);
}
static void testaBarraRetrato(void) {
  int rail = valor[AJ_RAIL], moderna = valor[AJ_RAIL_MODERNA], layout = valor[AJ_HOME_LAYOUT];
  for (int l = 0; l < HOME_LAYOUT_N; l++) {
    valor[AJ_HOME_LAYOUT] = l;
    for (int r = 0; r < 2; r++) for (int m = 0; m < 2; m++) {
      valor[AJ_RAIL] = r; valor[AJ_RAIL_MODERNA] = m;
      nv_layout_w = 2400; nv_layout_h = 1080;
      int recolhida = ajustes_rail_recolhida();
      float largura = ajustes_rail_largura_fixa();
      assert(recolhida == (m == 0 ? 0 : r == 0));
      nv_layout_w = 1080; nv_layout_h = 2340;
      assert(ajustes_rail_recolhida()); perto(ajustes_rail_largura_fixa(), 0);
      perto(ajustes_conteudo_x(), NV_CONTENT_PAD);
      float x, w; ajustes_area_conteudo(96, 96, &x, &w);
      perto(x, 96); perto(x + w * 0.5f, nv_layout_w * 0.5f);
      assert(valor[AJ_RAIL] == r && valor[AJ_RAIL_MODERNA] == m && valor[AJ_HOME_LAYOUT] == l);
      nv_layout_h = 1920;
      assert(ajustes_rail_recolhida()); perto(ajustes_rail_largura_fixa(), 0);
      testMobile = 0; nv_layout_h = 1728; /* TV mode keeps its sidebar preference. */
      assert(ajustes_rail_recolhida() == recolhida); perto(ajustes_rail_largura_fixa(), largura);
      testMobile = 1; nv_layout_w = 2400; nv_layout_h = 1080;
      assert(ajustes_rail_recolhida() == recolhida); perto(ajustes_rail_largura_fixa(), largura);
    }
  }
  valor[AJ_RAIL] = rail; valor[AJ_RAIL_MODERNA] = moderna; valor[AJ_HOME_LAYOUT] = layout;
}
static void testaAjustesRetrato(void) {
  const float alturas[] = {1920, 2340};
  int layoutSalvo = valor[AJ_LAYOUT_AJUSTES], escalaSalva = valor[AJ_TAMANHO_AJUSTES];
  valor[AJ_LAYOUT_AJUSTES] = 1;
  for (int h = 0; h < 2; h++) {
    nv_layout_w = 1080; nv_layout_h = alturas[h];
    for (int s = 0; s < 3; s++) {
      valor[AJ_TAMANHO_AJUSTES] = s;
      assert(ajRetrato() && ajustes_layout_lista() && !aj2TemInsp());
      assert(valor[AJ_LAYOUT_AJUSTES] == 1);
      perto(NV_VTELA_H * ajustes_tamanho_ajustes(), alturas[h]);
      GfxRect grade = aj2GradeR(), lista = aj2ListaR(), editor = aj2EditorR(), pagina = aj2PaginaR();
      dentroAjustes(grade); dentroAjustes(lista); dentroAjustes(editor); dentroAjustes(pagina);
      perto(lista.x, grade.x); perto(editor.x, grade.x);
      assert(editor.w > 740 && lista.w > 740 && pagina.y + pagina.h < lista.y);
      GfxRect topo[AJ2_T_N];
      for (int t = 0; t < AJ2_T_N; t++) {
        if (!aj2TopoVisivel(t)) { assert(t == AJ2_T_LAYOUT); continue; }
        topo[t] = aj2TopoRetratoR(t); dentroAjustes(topo[t]);
        assert(topo[t].x >= grade.x && topo[t].x + topo[t].w <= grade.x + grade.w);
        for (int j = 0; j < t; j++) if (aj2TopoVisivel(j))
          assert(topo[t].x >= topo[j].x + topo[j].w || topo[j].x >= topo[t].x + topo[t].w ||
                 topo[t].y >= topo[j].y + topo[j].h || topo[j].y >= topo[t].y + topo[t].h);
      }
      perto(topo[AJ2_T_PERFIL].w, grade.w - 2 * AJ2_G_PAD);
      assert(aj2TopoRetratoR(AJ2_T_LAYOUT).w == 0 && AJ_CAB_PAGINA == 136);
      assert(aj2LTopo() >= topo[AJ2_T_RESOLVER].y + topo[AJ2_T_RESOLVER].h);
      assert(aj2LBase() > aj2LTopo() + 500);
      // Choices stop before the restore row, which is above Confirm/Cancel.
      float botoes = editor.y + editor.h - 104;
      assert(aj2EditorBase(editor) + 24 <= botoes - 76);
      assert(aj2EditorBase(editor) > editor.y + 250);
      GfxRect modal = {560, 300, 800, 600}; ajCentraModal(&modal); dentroAjustes(modal);
      perto(modal.x + modal.w * 0.5f, NV_VTELA_W * 0.5f);
      perto(modal.y + modal.h * 0.5f, NV_VTELA_H * 0.5f);
      // Fileiras uses raw full-screen units and keeps all three action cells
      // beside an actual readable title column, with the preview out of way.
      AjFilMedidas f = ajFilMedir();
      assert(f.ilha.x >= 0 && f.ilha.x + f.ilha.w <= NV_TELA_W);
      assert(f.ilha.y + f.ilha.h <= NV_TELA_H);
      perto(f.painel.x, f.ilha.x + f.ilha.w);
      assert(f.cw - f.estado - f.card - f.tamanho - 122 > 250);
      assert(f.estado > 100 && f.card > 100 && f.tamanho > 100);
      float camposX = f.cx + f.cw - 14 - f.estado - f.card - f.tamanho;
      assert(camposX > f.cx && camposX + f.estado + f.card + f.tamanho < NV_TELA_W);
      // The drawn scale is cached; event time is unscaled after the draw.
      ajToqueLimpar(); ajToqueCamada(); escalaTeste = ajustes_tamanho_ajustes();
      float off = 0, desenho = escalaTeste;
      ajToqueRegistrar(AJT_LISTA, lista, &off, 500, 0, s + 30);
      PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0,
          (lista.x + 60) * desenho, (lista.y + 60) * desenho};
      assert(ajToqueRolar(&e)); escalaTeste = 1;
      e.fase = PONT_ROL_MOVER; e.delta = -13.5f * desenho; ajToqueRolar(&e); perto(off, 13.5f);
      e.fase = PONT_ROL_FIM; ajToqueRolar(&e); perto(off, 13.5f);
      assert(valor[AJ_LAYOUT_AJUSTES] == 1);
    }
  }
  valor[AJ_LAYOUT_AJUSTES] = 0;
  testMobile = 0; nv_layout_w = 1080; nv_layout_h = 1728; valor[AJ_TAMANHO_AJUSTES] = 2;
  assert(!ajRetrato() && !ajustes_layout_lista());
  nv_layout_w = 1920; nv_layout_h = 1080;
  assert(!ajRetrato() && !ajustes_layout_lista());
  perto(aj2EditorR().x, aj2X0());
  valor[AJ_LAYOUT_AJUSTES] = layoutSalvo; valor[AJ_TAMANHO_AJUSTES] = escalaSalva;
}
static void testaListaTelefone(void) {
  testMobile = 1;
  int layout = valor[AJ_LAYOUT_AJUSTES], tamanho = valor[AJ_TAMANHO_AJUSTES], secoes = nSecoes;
  nSecoes = AJ_MAX_SECOES;
  for (int h = 0; h < 2; h++) for (int s = 0; s < 3; s++) for (int lista = 0; lista < 2; lista++) {
    nv_layout_w = 1080; nv_layout_h = h ? 2340 : 1920;
    valor[AJ_TAMANHO_AJUSTES] = s; valor[AJ_LAYOUT_AJUSTES] = lista;
    assert(ajustes_layout_lista() && !aj2TemInsp());
    perto(ajustes_tamanho_ajustes(), (s == 0 ? 0.8f : s == 1 ? 0.9f : 1.0f) * 1.25f);
    GfxRect g = aj2GradeR(), l = aj2ListaR(), ed = aj2EditorR();
    dentroAjustes(g); dentroAjustes(l); dentroAjustes(ed);
    perto(AJ_TOPO, l.y + AJ_A3_CAB);
    perto(g.x + g.w * 0.5f, NV_VTELA_W * 0.5f);
    perto(l.x + l.w * 0.5f, NV_VTELA_W * 0.5f);
    assert(l.w > 740 && l.h > 580);
    perto(l.y, AJ2_CORPO);
    aj2LRol = 0; focoIndice = 1; uxTopo = -1; uxIndice = 2;
    ajToqueLimpar(); aj2LAtualizar(0.1f);
    assert(aj2LRh == 128 && AJ2_LN_H == 96);
    float total; aj2LYRel(0, &total);
    assert(total > aj2LBase() - aj2LTopo());
    GfxRect primeiro = aj2LLinhaR(0);
    perto(primeiro.x + primeiro.w * 0.5f, NV_VTELA_W * 0.5f);
    assert(primeiro.h >= 128);
    escalaTeste = ajustes_tamanho_ajustes();
    ajToqueRegistrar(AJT_INDICE, (GfxRect){g.x, aj2LTopo(), g.w, aj2LBase() - aj2LTopo()}, &aj2LRol,
                     total - (aj2LBase() - aj2LTopo()), 0, 0);
    PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, (g.x + 60) * escalaTeste, (aj2LTopo() + 90) * escalaTeste};
    assert(ajToqueRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -200 * escalaTeste;
    assert(ajToqueRolar(&e)); e.fase = PONT_ROL_FIM; ajToqueRolar(&e);
    float scroll = aj2LRol; assert(scroll > 0);
    aj2LAtualizar(0.1f); perto(aj2LRol, scroll);
    GfxRect movido = aj2LLinhaR(0);
    perto(movido.y, primeiro.y - scroll);
    assert(valor[AJ_LAYOUT_AJUSTES] == lista && valor[AJ_TAMANHO_AJUSTES] == s);
  }
  nSecoes = secoes; valor[AJ_LAYOUT_AJUSTES] = layout; valor[AJ_TAMANHO_AJUSTES] = tamanho;
  nv_layout_w = 2400; nv_layout_h = 1080; escalaTeste = 1;
}
static void testaAjustesTelaCheia(void) {
  const float telas[][2] = {{1080, 1920}, {1080, 2340}, {1920, 1080}, {2340, 1080}, {2400, 1080}};
  int salvo[AJ_N]; memcpy(salvo, valor, sizeof salvo);
  for (int d = 0; d < 5; d++) for (int s = 0; s < 3; s++) for (int modo = 0; modo < 2; modo++) for (int rail = 0; rail < 3; rail++) {
    nv_layout_w = telas[d][0]; nv_layout_h = telas[d][1];
    valor[AJ_TAMANHO_AJUSTES] = s; valor[AJ_LAYOUT_AJUSTES] = modo;
    valor[AJ_HOME_LAYOUT] = rail == 1 ? HOME_LAYOUT_PADRAO : HOME_LAYOUT_MODERNA;
    valor[AJ_RAIL] = rail == 0 ? 0 : 1; valor[AJ_RAIL_MODERNA] = rail == 2 ? 0 : 1;
    assert(ajTelaCheia());
    assert(ajustes_layout_lista() == (telefoneui_ativo() || modo));
    assert(valor[AJ_LAYOUT_AJUSTES] == modo);
    float escala = ajustes_tamanho_ajustes(), reserva = ajustes_rail_largura_fixa();
    GfxRect tela = ajTelaR(), grade = aj2GradeR(), lista = aj2ListaR(), editor = aj2EditorR(), pagina = aj2PaginaR();
    perto(tela.x, 0); perto(tela.y, 0); perto(tela.w * escala, telas[d][0]); perto(tela.h * escala, telas[d][1]);
    dentroAjustes(grade); dentroAjustes(lista); dentroAjustes(editor); dentroAjustes(pagina);
    perto(grade.y, 0); perto(grade.x * escala, reserva > 0 ? 48 + reserva : 0);
    perto(grade.x + grade.w, NV_VTELA_W); perto(grade.y + grade.h, NV_VTELA_H);
    perto(editor.x, grade.x); perto(editor.x + editor.w, NV_VTELA_W); perto(editor.y + editor.h, NV_VTELA_H);
    perto(lista.x + lista.w, NV_VTELA_W); perto(lista.y + lista.h, NV_VTELA_H);
    perto(pagina.y, 0); perto(AJ_TOPO, lista.y + AJ_A3_CAB);
    assert(AJ_BASE > AJ_TOPO + 250);
    // The portrait geometry wraps the controls independently of account labels.
    // The landscape label formatter reads live providers and belongs to UI tests.
    if (ajRetrato()) for (int t = 0; t < AJ2_T_N; t++) {
      if (!aj2TopoVisivel(t)) { assert(t == AJ2_T_LAYOUT); continue; }
      GfxRect botao = aj2TopoRetratoR(t); dentroAjustes(botao);
      assert(botao.x >= grade.x + AJ2_G_PAD - 0.01f);
      assert(botao.x + botao.w <= grade.x + grade.w - AJ2_G_PAD + 0.01f);
      assert(botao.y >= 40);
    }
    if (aj2TemInsp()) {
      GfxRect insp = aj2InspR(); dentroAjustes(insp);
      assert(insp.x >= grade.x + 28);
      if (ajRetrato()) assert(AJ_TOPO >= insp.y + insp.h + 24 + AJ_A3_CAB - 0.01f);
      else assert(insp.x + insp.w + 36 <= lista.x + 0.01f);
    }
    GfxRect modal = {560, 300, 800, 600}; ajCentraModal(&modal); dentroAjustes(modal);
    perto(modal.x + modal.w * 0.5f, NV_VTELA_W * 0.5f);
    perto(modal.y + modal.h * 0.5f, NV_VTELA_H * 0.5f);
    AjFilMedidas f = ajFilMedir();
    perto(f.ilha.x, reserva > 0 ? 48 + reserva : 0); perto(f.ilha.y, 0);
    perto(f.ilha.x + f.ilha.w, NV_TELA_W); perto(f.ilha.y + f.ilha.h, NV_TELA_H);
    assert(f.cx >= f.ilha.x + 30 && f.cw > 300);
    // Category rows and touch/scroll clips use the effective layout.
    ajToqueLimpar(); ajToqueCamada(); escalaTeste = escala;
    float off = 0;
    ajToqueRegistrar(AJT_LISTA, (GfxRect){lista.x, AJ_TOPO, lista.w, AJ_BASE - AJ_TOPO}, &off, 500, 0, 91);
    int indice = uxIndice, pendente = uxPendente, foco = focoIndice;
    PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, (lista.x + 50) * escala, (AJ_TOPO - 1) * escala};
    assert(!ajToqueRolar(&e)); e.y = (AJ_TOPO + 50) * escala; assert(ajToqueRolar(&e));
    escalaTeste = 1; e.fase = PONT_ROL_MOVER; e.delta = -15.75f * escala;
    assert(ajToqueRolar(&e)); perto(off, 15.75f);
    e.fase = PONT_ROL_SOLTAR; ajToqueRolar(&e);
    e.fase = PONT_ROL_INERCIA; e.delta = -1000 * escala; assert(ajToqueRolar(&e)); perto(off, 500);
    assert(!ajToqueRolar(&e)); perto(off, 500);
    e.fase = PONT_ROL_FIM; ajToqueRolar(&e); assert(ajToqueLivre[AJT_LISTA]);
    assert(uxIndice == indice && uxPendente == pendente && focoIndice == foco);
    // Fullscreen page surface, generic chips and actual dialogs keep separate materials.
    fundosTeste = superficiesTeste = 0; ajustes_ui_fundo();
    assert(fundosTeste == 1 && superficiesTeste == 0);
    ajFundoPagina(); assert(fundosTeste == 2 && superficiesTeste == 1);
    sombrasTeste = 0; ajPaginaIlha(tela, 40, 1); perto(raioTeste, 0); assert(!sombrasTeste);
    perto(superficieTeste.x, 0); perto(superficieTeste.w * escala, telas[d][0]);
    GfxRect chip = {100, 100, 160, 56};
    ajustes_ui_ilha(chip, 28, 0); perto(raioTeste, 0.5f); assert(sombrasTeste == 1);
    ajIlha(modal, 36, 1, 1); perto(raioTeste, 36 / modal.h); assert(sombrasTeste == 2);
    ajIlhaMiolo(editor, 40, 0.88f); perto(raioTeste, 0); assert(sombrasTeste == 2);
  }
  // TV mode retains the original floating Settings frames.
  testMobile = 0;
  for (int d = 0; d < 2; d++) {
    nv_layout_w = d ? 1080 : 1728; nv_layout_h = d ? 1728 : 1080;
    valor[AJ_HOME_LAYOUT] = HOME_LAYOUT_DINAMICA; valor[AJ_TAMANHO_AJUSTES] = 2;
    assert(!ajTelaCheia()); GfxRect g = aj2GradeR();
    perto(g.x, 48); perto(g.y, 112); perto(g.x + g.w, NV_VTELA_W - 40); perto(g.y + g.h, NV_VTELA_H - 40);
    perto(aj2EditorR().x, aj2X0() + AJ2_EDITOR_DX);
    sombrasTeste = 0; ajPaginaIlha(g, 40, 1); perto(raioTeste, 40 / g.h); assert(sombrasTeste == 1);
    AjFilMedidas f = ajFilMedir(); perto(f.ilha.x, 48); perto(f.ilha.y, 112); perto(f.ilha.x + f.ilha.w, NV_TELA_W - 48);
  }
  testMobile = 1; memcpy(valor, salvo, sizeof salvo); nv_layout_w = 2400; nv_layout_h = 1080; escalaTeste = 1;
}
#elif defined(TESTE_MENU)
int ajustes_home_layout(void) { return HOME_LAYOUT_MODERNA; }
#elif defined(TESTE_PERFIL)
static SvAmigo amigosTeste[3];
static int recomendaTeste = 1;
static float perfilEscalaTeste = 1.5f, alvoCorpoTopo, alvoCorpoFim;
static PonteiroRolagemFn perfilRolarRegistrado;
int recomenda_ativo(void) { return recomendaTeste; }
float gfx_escala(void) { return perfilEscalaTeste; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { perfilRolarRegistrado = fn; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar,
                   PonteiroFn ativar, int a, int b) {
  (void)x; (void)y; (void)w; (void)h; (void)focar; (void)ativar; (void)a; (void)b;
  assert(!"portrait body target missing clip");
}
void ponteiro_alvo_faixa(float x, float y, float w, float h, float topo, float fim,
                         PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  (void)x; (void)y; (void)w; (void)h; (void)focar; (void)ativar; (void)a; (void)b;
  alvoCorpoTopo = topo; alvoCorpoFim = fim;
}
int menu_pilula_rect(float *x, float *y, float *w, float *h) {
  (void)x; (void)y; (void)w; (void)h; return 0;
}
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
  testaAjustesRetrato();
  testaBarraRetrato();
  testaListaTelefone();
  testaAjustesTelaCheia();
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
  nv_layout_w = 1080; nv_layout_h = 2340;
  for (int i = 0; i < nNumeros(); i++) {
    GfxRect r = rNumero(i);
    assert(r.x >= PF_X && r.x + r.w <= NV_TELA_W);
    assert(r.w > 400);
    if (i >= 2) assert(r.y > rNumero(i - 2).y + PF_NUM_H);
  }
  for (int i = 0; i < 3; i++) {
    GfxRect r = rCartao(i);
    perto(r.w, PF_W);
    if (i) assert(r.y > rCartao(i - 1).y + alturaCartoes());
    assert(r.y + r.h < PF_CONTEUDO_H);
  }
  assert(rCartao(0).y > rNumero(4).y + PF_NUM_H);
  recomendaTeste = 0;
  assert(rCartao(0).y > rNumero(3).y + PF_NUM_H);
  assert(rDuelo(2).y > rDuelo(0).y + PF_DUELO_H);
  assert(rCartazes().y > rDuelo(3).y + PF_DUELO_H);
  recomendaTeste = 1; modo = PF_RESUMO; temDados = 1; secao = 0;
  nv_layout_h = 1728;
  pfToquePreparar(); assert(perfilRolarRegistrado == pfToqueRolar && pfToque.maximo > 0);
  e = (PonteiroRolagem){PONT_ROL_INICIO, 1, 0, 0,
                        (PF_X + 100) * perfilEscalaTeste, (PF_NUM_Y + 100) * perfilEscalaTeste};
  assert(perfilRolarRegistrado(&e));
  perfilEscalaTeste = 1;
  e.fase = PONT_ROL_MOVER; e.delta = -37.5f;
  pfToqueRolar(&e); perto(pfScroll, 25); assert(secao == 0);
  e.fase = PONT_ROL_FIM; pfToqueRolar(&e); pfToquePreparar(); perto(pfScroll, 25);
  e.fase = PONT_ROL_INERCIA; e.delta = -1e6f; pfToqueRolar(&e); perto(pfScroll, pfToque.maximo);
  GfxRect fim = rCartao(2); assert(fim.y + fim.h <= PF_CONTEUDO_H + .01f);
  pfAlvoCorpo(PF_X, 20, 100, 200, NULL, NULL, 0, 0);
  perto(alvoCorpoTopo, PF_NUM_Y - 16); perto(alvoCorpoFim, PF_CONTEUDO_H);
  e.fase = PONT_ROL_INICIO; e.y = 100; assert(!pfToqueRolar(&e));
  pfToqueLimpar(); secao = 2; pfToquePreparar();
  assert(pfScroll > 0 && rCartao(2).y + rCartao(2).h <= PF_CONTEUDO_H + .01f);
  nv_layout_w = 2400; nv_layout_h = 1080; recomendaTeste = 1;
  pfToquePreparar(); perto(pfScroll, 0);
  perto(rNumero(4).y, PF_NUM_Y);
  perto(rCartao(0).w, PF_CAL_W); perto(rCartao(2).y, PF_CARD_Y);
  perto(rDuelo(3).y, rDuelo(0).y);
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
