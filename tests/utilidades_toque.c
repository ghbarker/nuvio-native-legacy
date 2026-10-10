/* Callbacks reais das telas auxiliares, sem desenho ou servicos externos. */
#ifdef _WIN32
#include <time.h>
static struct tm *utilidades_localtime_r(const time_t *t, struct tm *out) {
  struct tm *r = localtime(t);
  if (r) *out = *r;
  return r ? out : NULL;
}
#define localtime_r utilidades_localtime_r
#endif
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#if defined(TESTE_SPOTLIGHT)
#include "../src/spotlight.c"
#elif defined(TESTE_EXPLORAR)
#include "../src/explorar.c"
#elif defined(TESTE_AGENDA)
#include "../src/agendaui.c"
#elif defined(TESTE_NOTAS)
#include "../src/atualizacao.c"
#elif defined(TESTE_DIAGNOSTICO)
#include "../src/diagnostico.c"
#elif defined(TESTE_LIVETV)
#include "../src/livetvdiag.c"
#elif defined(TESTE_REGISTRO)
#include "../src/registro.c"
#elif defined(TESTE_GUIA)
#include "../src/guia.c"
#else
#error escolha TESTE_* utilidade
#endif
#include <assert.h>

float nv_layout_w = 2400.0f;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
float nv_layout_h = 1080.0f;
#if defined(TESTE_GUIA)
typedef struct { TxtEstilo estilo; char texto[240]; GfxRect r; } GuiaTextoTeste;
static GuiaTextoTeste textosGuia[512], desenhosGuia[512];
static int nTextosGuia, nDesenhosGuia, nCoresGuia, nAlvosGuia, previewGuiaTeste = 1, rotulosLongosGuiaTeste;
static GfxRect formasGuia[512], furoGuia, recorteGuia;
static PonteiroAlvo alvosGuia[128];
static float escalaGuiaTeste = 1;
static PonteiroRolagemFn rolarGuiaTeste;
static EpgProg programasGuia[] = {{3600, 5400, "Jornal da manha e entrevistas"},
                                  {5400, 7200, "Cinema: uma aventura pelo mundo"},
                                  {7200, 10800, "Esportes e noticias ao vivo"}};
float gfx_tex_aspect_atual;
static int letrasGuiaTeste(const char *s) { int n = 0; for (; *s; s++) if (((unsigned char)*s & 0xc0) != 0x80) n++; return n; }
static int fonteGuiaTeste(TxtEstilo estilo) {
  switch (estilo) {
    case TXT_TITULO2: return 56;
    case TXT_ILHA_TITULO: case TXT_HEADLINE: return 40;
    case TXT_V2_ROT: return 32;
    case TXT_CALLOUT: case TXT_CW_TITULO: case TXT_V2_28: return 28;
    case TXT_ILHA_HORA: case TXT_MINI: return 15;
    case TXT_DET_META2: return 23;
    case TXT_V2_SEG: case TXT_ILHA_NOME: return 26;
    default: return 24;
  }
}
const char *i18n(const char *s) {
  if (rotulosLongosGuiaTeste && (!strcmp(s, "Buscar") || !strcmp(s, "Categorias") || !strcmp(s, "Cartões") ||
      !strcmp(s, "Lista") || !strcmp(s, "Addons") || !strcmp(s, "Diagnóstico") || !strncmp(s, "Preview:", 8)))
    return "Este e um controle do guia com uma traducao muito longa para testar a largura dos botoes";
  return s;
}
TxtLinha txt_linha(TxtEstilo estilo, const char *s, int r, int g, int b, int a) {
  (void)estilo; (void)r; (void)g; (void)b; (void)a;
  assert(nTextosGuia < 512);
  int corpo = fonteGuiaTeste(estilo), w = (int)(letrasGuiaTeste(s) * corpo * 0.55f);
  textosGuia[nTextosGuia] = (GuiaTextoTeste){estilo, "", {0, 0, w, corpo * 1.2f}};
  snprintf(textosGuia[nTextosGuia].texto, sizeof textosGuia[0].texto, "%s", s);
  return (TxtLinha){.tex = ++nTextosGuia, .w = w, .h = (int)(corpo * 1.2f)};
}
TxtLinha txt_linha_corta(TxtEstilo estilo, const char *s, int r, int g, int b, int a, float max) {
  assert(max >= 0);
  TxtLinha l = txt_linha(estilo, s, r, g, b, a);
  if (l.w > max) l.w = (int)max;
  return l;
}
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) {
  (void)a; assert(l.tex > 0 && l.tex <= (GLuint)nTextosGuia && nDesenhosGuia < 512);
  GuiaTextoTeste t = textosGuia[l.tex - 1]; t.r = (GfxRect){x, y, l.w, l.h};
  desenhosGuia[nDesenhosGuia++] = t;
}
float txt_bloco(TxtEstilo estilo, const char *s, int r, int g, int b, float x, float y, float w, float h, float a, int maxLinhas) {
  TxtLinha l = txt_linha_corta(estilo, s, r, g, b, 255, w);
  int linhas = (int)ceilf(letrasGuiaTeste(s) * fonteGuiaTeste(estilo) * 0.55f / w);
  if (linhas > maxLinhas) linhas = maxLinhas;
  if (linhas < 1) linhas = 1;
  l.h = (int)(linhas * h); txt_desenhar_alpha(l, x, y, a); return linhas * h;
}
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)ca;
  assert(r.w >= 0 && r.h >= 0 && nCoresGuia < 512); formasGuia[nCoresGuia++] = r;
}
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float parx, float pary, float raio,
               float cr, float cg, float cb, float ca) {
  (void)tex; (void)modo; (void)foco; (void)parx; (void)pary; gfx_cor(r, raio, cr, cg, cb, ca);
}
void gfx_furo_raio(GfxRect r, float raio) { (void)raio; furoGuia = r; }
void gfx_recorte(float x, float y, float w, float h) { recorteGuia = (GfxRect){x, y, w, h}; }
void gfx_sem_recorte(void) {}
void gfx_esqueleto(GfxRect r, float raio, float cr, float cg, float cb, float ca) { gfx_cor(r, raio, cr, cg, cb, ca); }
void gfx_icone(GfxRect r, const char *id, float cr, float cg, float cb, float ca) { (void)id; gfx_cor(r, 0, cr, cg, cb, ca); }
void gfx_anel_fora(GfxRect r, float raio, float folga, float esp, float cr, float cg, float cb, float ca) {
  (void)folga; (void)esp; gfx_cor(r, raio, cr, cg, cb, ca);
}
void gfx_luz_canto(GfxRect r, float raio, float cx, float cy, float alcance, float cr, float cg, float cb, float ca) {
  (void)cx; (void)cy; (void)alcance; gfx_cor(r, raio, cr, cg, cb, ca);
}
float gfx_escala(void) { return escalaGuiaTeste; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { rolarGuiaTeste = fn; }
void ponteiro_camada(void) { nAlvosGuia = 0; }
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  assert(nAlvosGuia < 128); alvosGuia[nAlvosGuia++] = (PonteiroAlvo){x, y, w, h, focar, ativar, a, b, 0, NULL};
}
void ponteiro_alvo_faixa(float x, float y, float w, float h, float topo, float fim, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  float cima = fmaxf(y, topo), baixo = fminf(y + h, fim);
  if (baixo > cima) ponteiro_alvo(x, cima, w, baixo - cima, focar, ativar, a, b);
}
int ajustes_vidro(void) { return 0; }
void ajustes_acento(float *r, float *g, float *b) { *r = .5f; *g = .6f; *b = .8f; }
int ajustes_tinta_foco(void) { return 255; }
int ajustes_animacoes_reduzidas(void) { return 1; }
int ajustes_relogio_12h(void) { return 0; }
Uint32 SDL_GetTicks(void) { return 500; }
void plrui_pilula_foco(GfxRect r, float a) { gfx_cor(r, 0, 1, 1, 1, a); }
void plrui_botao_repouso(GfxRect r, float a) { gfx_cor(r, 0, 1, 1, 1, a); }
GLuint tex_obter_larg(const char *s, float w) { (void)s; (void)w; return 0; }
float tex_aspecto(const char *s) { (void)s; return 0; }
int tex_cor_fundo(const char *s, float *r, float *g, float *b) { (void)s; (void)r; (void)g; (void)b; return 0; }
int tex_logo_tom_unico(const char *s) { (void)s; return 0; }
int epg_estado(void) { return EPG_PRONTO; }
int epg_match_id(const char *id) { (void)id; return 0; }
int epg_match(const char *nome) { (void)nome; return 0; }
int epg_agora(int epg, time_t t, EpgProg *p) {
  (void)epg; for (int i = 0; i < 3; i++) if (programasGuia[i].ini <= t && programasGuia[i].fim > t) { *p = programasGuia[i]; return 1; }
  return 0;
}
int epg_proximo(int epg, time_t t, int k, EpgProg *p) {
  (void)epg; for (int i = 0; i < 3; i++) if (programasGuia[i].ini > t && k-- == 0) { *p = programasGuia[i]; return 1; }
  return 0;
}
int epg_faixa(int epg, time_t de, time_t ate, EpgProg *out, int cap) {
  (void)epg; int n = 0;
  for (int i = 0; i < 3; i++) if (programasGuia[i].ini < ate && programasGuia[i].fim > de) { if (n < cap) out[n] = programasGuia[i]; n++; }
  return n;
}
int xtream_e_id(const char *id) { (void)id; return 0; }
void xtepg_querer(const char *id) { (void)id; assert(!"unexpected network request"); }
int xtepg_tem(const char *id) { (void)id; return 0; }
int xtepg_agora(const char *id, time_t t, EpgProg *p) { (void)id; (void)t; (void)p; return 0; }
int xtepg_proximo(const char *id, time_t t, int k, EpgProg *p) { (void)id; (void)t; (void)k; (void)p; return 0; }
int xtepg_faixa(const char *id, time_t de, time_t ate, EpgProg *out, int cap) { (void)id; (void)de; (void)ate; (void)out; (void)cap; return 0; }
int lembrete_achar(const char *canal, time_t ini, const char *titulo) { (void)canal; (void)ini; (void)titulo; return -1; }
const Lembrete *lembrete_item(int i) { (void)i; return NULL; }
void lembrete_ajustar(int i, time_t ini, time_t fim) { (void)i; (void)ini; (void)fim; assert(!"unexpected reminder update"); }
int player_mini_no_guia_ativo(void) { return previewGuiaTeste; }
int player_janela_animando(float *x, float *y, float *w, float *h) { (void)x; (void)y; (void)w; (void)h; return 0; }
int player_carregando(void) { return 0; }
int video_pronto(void) { return 1; }
int marca_resolucao(const char *s) { (void)s; return -1; }
float marca_formato(FormatoMarca f, float x, float y, float altura, float r, float g, float b, float a) {
  (void)f; (void)x; (void)y; (void)altura; (void)r; (void)g; (void)b; (void)a; return 0;
}
float badge_desenhar(float x, float y, const char *s, BadgeEstilo estilo, float a) { (void)x; (void)y; (void)s; (void)estilo; (void)a; return 0; }
void teclado_desenhar(Uint32 agora) { (void)agora; }
static void guiaTesteLimpar(void) { nTextosGuia = nDesenhosGuia = nCoresGuia = nAlvosGuia = 0; }
static void guiaTesteCanais(void) {
  nCanais = 16; nCats = 4; nFavOrd = 0;
  for (int i = 0; i < 4; i++) { catIni[i] = i * 4; catN[i] = 4; snprintf(cats[i], sizeof cats[i], "Categoria %d", i + 1); }
  for (int i = 0; i < 16; i++) {
    memset(&canais[i], 0, sizeof canais[i]); canais[i].cat = i / 4; canais[i].epg = 0;
    snprintf(canais[i].id, sizeof canais[i].id, "teste-%d", i);
    snprintf(canais[i].nome, sizeof canais[i].nome, "Canal %d - Noticias e documentarios", i + 1);
  }
  aberta = 1; overlay = catAberto = buscaEstado = painel = 0; focoTopo = focoLin = focoCol = 0;
}
#endif
#if defined(TESTE_AGENDA)
static Noticia manchetesTeste[8];
static float escalaAgendaTeste = 1.5f;
static float uiAgendaTeste = 1.5f;
static float railAgendaTeste;
static AgItem linhasAgendaTeste[32];
static PonteiroRolagemFn rolagemAgendaTeste;
float gfx_escala(void) { return escalaAgendaTeste; }
float gfx_escala_ui(void) { return uiAgendaTeste; }
float ajustes_rail_largura_fixa(void) { return railAgendaTeste; }
int agenda_n(void) { return 32; }
const AgItem *agenda_lista(int i) { return i >= 0 && i < 32 ? &linhasAgendaTeste[i] : NULL; }
int agenda_dias(const char *iso) { (void)iso; return 0; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { rolagemAgendaTeste = fn; }
const Noticia *noticias_item(const char *imdb, int i) { (void)imdb; return i >= 0 && i < 8 ? &manchetesTeste[i] : NULL; }
#endif
#if defined(TESTE_EXPLORAR)
static float railExplorarTeste;
float ajustes_conteudo_x(void) { return NV_CONTENT_PAD + railExplorarTeste; }
static PonteiroAlvo ultimoAlvo;
static int alvos;
void ponteiro_alvo(float x, float y, float w, float h, PonteiroFn focar, PonteiroFn ativar, int a, int b) {
  ultimoAlvo = (PonteiroAlvo){x, y, w, h, focar, ativar, a, b, 0, NULL};
  alvos++;
}
#endif
static void perto(float a, float b) { assert(fabsf(a - b) < .001f); }
static void exercitar(ToqueRolagem *r, float *offset, PonteiroRolagemFn fn, int eixoY) {
  PonteiroRolagem e = {PONT_ROL_INICIO, eixoY, 0, 0, 220, 480};
  *offset = 0;
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 250, eixoY, offset);
  assert(fn(&e) && r->livre);
  e.fase = PONT_ROL_MOVER; e.delta = -37;
  assert(fn(&e)); perto(*offset, 18.5f);
  e.fase = PONT_ROL_SOLTAR; assert(fn(&e));
  e.fase = PONT_ROL_INERCIA; e.delta = -11;
  assert(fn(&e)); perto(*offset, 24);
  e.delta = -10000; fn(&e); perto(*offset, 250);
  assert(!fn(&e));
  e.delta = 10000; fn(&e); perto(*offset, 0);
  assert(!fn(&e));
  e.fase = PONT_ROL_MOVER; e.delta = -37; fn(&e);
  e.fase = PONT_ROL_FIM; fn(&e); perto(*offset, 18.5f); assert(r->livre);
  e.fase = PONT_ROL_CANCELAR; fn(&e); perto(*offset, 18.5f);
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 10, eixoY, offset);
  perto(*offset, 10); assert(r->livre);
  toquerol_limpar(r); assert(!r->livre);
  e.fase = PONT_ROL_INICIO; e.x = 190; assert(!fn(&e));
  e.x = 220; e.eixoY = !eixoY; assert(!fn(&e));
  e.eixoY = eixoY;
  toquerol_vincular(r, (GfxRect){100, 200, 300, 400}, 2, 0, 0, eixoY, offset);
  assert(!fn(&e));
}

int main(void) {
#if defined(TESTE_GUIA)
  const float alturas[] = {1920, 2340};
  focoLin = 2; focoCol = 3; rolY = 27.5f; rolL = 33.25f;
  for (int h = 0; h < 2; h++) {
    guiaTesteLimpar();
    float x, y, w, altura;
    testMobile = 1;
    nv_layout_w = 1080; nv_layout_h = alturas[h];
    guia_preview_rect(&x, &y, &w, &altura);
    assert(gRetrato());
    perto(G_INFO_X + G_INFO_W * .5f, NV_TELA_W * .5f);
    perto(G_INFO_W, 920);
    perto(x, G_PREVIEW_X); perto(y, G_PREVIEW_Y);
    perto(w, G_PREVIEW_W); perto(altura, G_PREVIEW_H);
    perto(x + w * .5f, NV_TELA_W * .5f);
    perto(w / altura, 16.0f / 9.0f);
    assert(G_HERO_Y > gTopoFim);
    assert(y >= G_HERO_Y + G_INFO_H + 24);
    assert(G_TOPO > y + altura && G_L_BASE - G_L_TOPO > G_CARD_H);
    for (int i = 0; i < G_TOPO_N; i++) {
      GfxRect r = gTopoRects[i];
      assert(r.x >= G_AREA_X && r.x + r.w <= G_AREA_DIR);
      assert(r.y >= G_TOPO_Y + G_TOPO_H + 52 && r.y + r.h <= gTopoFim);
      for (int j = 0; j < i; j++) {
        GfxRect q = gTopoRects[j];
        assert(r.x >= q.x + q.w || q.x >= r.x + r.w ||
               r.y >= q.y + q.h || q.y >= r.y + r.h);
      }
    }
    perto(gTopoRects[G_TOPO_CARTOES].y, gTopoRects[G_TOPO_LISTA].y);
    perto(gTopoRects[G_TOPO_CARTOES].x + gTopoRects[G_TOPO_CARTOES].w,
          gTopoRects[G_TOPO_LISTA].x);
    /* Very long translations cannot displace a control or split the pair. */
    float longos[G_TOPO_N] = {1300, 1200, 1400, 1300, 1800, 1600, 1700};
    gTopoDistribuir(longos);
    for (int i = 0; i < G_TOPO_N; i++)
      assert(gTopoRects[i].x >= G_AREA_X && gTopoRects[i].x + gTopoRects[i].w <= G_AREA_DIR);
    perto(gTopoRects[G_TOPO_CARTOES].y, gTopoRects[G_TOPO_LISTA].y);
    assert(G_L_BASE - G_L_TOPO >= G_L_HEAD + 3 * G_L_ROW - .01f);
    /* The video API refreshes after a rotation; old wrapped bounds cannot leak. */
    testMobile = 0;
    nv_layout_w = 1920; nv_layout_h = 1080;
    guia_preview_rect(&x, &y, &w, &altura);
    assert(!gRetrato()); perto(x, 1040); perto(y, 108); perto(w, 800); perto(altura, 450);
    perto(G_INFO_W, 912); perto(G_TOPO, 580);
    testMobile = 1;
    nv_layout_w = 2400;
    guia_preview_rect(&x, &y, &w, &altura);
    perto(x, 1520); perto(y, 108); perto(w, 800); perto(altura, 450);
  }
  assert(focoLin == 2 && focoCol == 3); perto(rolY, 27.5f); perto(rolL, 33.25f);
  const float telas[][2] = {{1080, 1920}, {1080, 2340}, {1920, 1080}, {2400, 1080}};
  const int modosTela[] = {1, 1, 0, 1};
  guiaTesteCanais();
  for (int d = 0; d < 4; d++) {
    testMobile = modosTela[d];
    nv_layout_w = telas[d][0]; nv_layout_h = telas[d][1];
    guiaTesteLimpar(); gTopoMedir();
    desenharTopo(1);
    assert(nAlvosGuia == G_TOPO_N);
    for (int i = 0; i < G_TOPO_N; i++) {
      PonteiroAlvo r = alvosGuia[i];
      assert(r.x >= 0 && r.x + r.w <= NV_TELA_W && r.y >= 0 && r.y + r.h <= gTopoFim + 1);
      if (gRetrato()) { perto(r.x, gTopoRects[i].x); perto(r.y, gTopoRects[i].y); perto(r.h, 72); }
    }
    guiaTesteLimpar();
    float x, y, w, h; guia_preview_rect(&x, &y, &w, &h);
    desenharHero(1, 4200, 4200);
    perto(furoGuia.x, x); perto(furoGuia.y, y); perto(furoGuia.w, w); perto(furoGuia.h, h);
    for (int i = 0; i < nDesenhosGuia; i++) {
      GfxRect r = desenhosGuia[i].r;
      assert(r.x >= G_AREA_X && r.x + r.w <= G_AREA_DIR + .01f);
      if (gRetrato() ? r.y >= G_PREVIEW_Y : r.x >= G_PREVIEW_X)
        assert(r.y >= G_PREVIEW_Y && r.y + r.h <= G_PREVIEW_Y + G_PREVIEW_H);
      else assert(r.y >= G_HERO_Y && r.y + r.h <= G_HERO_Y + G_INFO_H);
    }
    guiaTesteLimpar(); desenharHero(1, 4200, 6000);
    for (int i = 0; i < nDesenhosGuia; i++) {
      GfxRect r = desenhosGuia[i].r;
      if (gRetrato() ? r.y < G_PREVIEW_Y : r.x < G_PREVIEW_X)
        assert(r.y >= G_HERO_Y && r.y + r.h <= G_HERO_Y + G_INFO_H);
    }
    // The two title/meta lines are drawn inside the same taller programme cell.
    guiaTesteLimpar(); gCel = G_B_CEL; overlay = 0;
    GfxRect alvo;
    float rowY = G_L_TOPO + G_L_HEAD, agoraX = G_L_FAIXA_X + 10 * G_L_FAIXA_W / G_L_JANELA_MIN;
    desenharLinhaLista(&canais[0], rowY, 1, 1, 4200, 3600, 4200, 0, agoraX, &alvo);
    perto(alvo.h, G_L_CEL);
    desenharLinhaLista(&canais[0], rowY, 1, 1, 4200, 3600, 4200, 1, agoraX, NULL);
    int titulos = 0, horarios = 0;
    for (int i = 0; i < nDesenhosGuia; i++) {
      GuiaTextoTeste t = desenhosGuia[i];
      assert(t.r.y >= rowY && t.r.y + t.r.h <= rowY + G_L_CEL);
      assert(t.r.x >= G_AREA_X && t.r.x + t.r.w <= G_AREA_DIR);
      if (!strcmp(t.texto, programasGuia[0].titulo)) { titulos++; if (gRetrato()) assert(t.estilo == TXT_V2_ROT && t.r.y + t.r.h < rowY + 58); }
      if (gRetrato() && t.estilo == TXT_V2_28) { horarios++; perto(t.r.y, rowY + 58); }
    }
    assert(titulos == 1); if (gRetrato()) assert(horarios > 0);
    guiaTesteLimpar(); overlay = 1; gCel = G_B_CEL;
    desenharLinhaLista(&canais[0], rowY, 1, 1, 4200, 3600, 4200, 1, agoraX, &alvo);
    perto(alvo.h, G_B_CEL);
    for (int i = 0; i < nDesenhosGuia; i++) {
      assert(desenhosGuia[i].r.y >= rowY && desenhosGuia[i].r.y + desenhosGuia[i].r.h <= rowY + G_B_CEL);
      // The compact mini-guide gets programme times from its ruler.
      assert(desenhosGuia[i].estilo != TXT_V2_28 && desenhosGuia[i].estilo != TXT_V2_ROT);
    }
    overlay = 0;
    guiaTesteLimpar(); desenharReguaEm(1, 3660, G_TOPO);
    if (gRetrato()) for (int i = 0; i < nDesenhosGuia; i++)
      assert(desenhosGuia[i].r.x >= G_L_FAIXA_X && desenhosGuia[i].r.x + desenhosGuia[i].r.w <= G_AREA_DIR);
    if (gRetrato()) for (int i = 0; i < nCoresGuia; i++)
      assert(formasGuia[i].x >= G_AREA_X && formasGuia[i].x + formasGuia[i].w <= G_AREA_DIR + 1);
    // A programme ending two minutes into the window has no negative progress rectangles.
    guiaTesteLimpar();
    desenharLinhaLista(&canais[0], rowY, 1, 1, 5340, 5280, 5340, 0, G_L_FAIXA_X, &alvo);
    assert(nCoresGuia > 0);
    // Cards at either edge use the intersection of their drawn rectangle and the content gutters.
    guiaTesteLimpar();
    desenharCardVisivel(&canais[1], G_AREA_X - 100, G_TOPO + G_HEAD_H, 0, 1, 0, 1, 4200);
    assert(nAlvosGuia == 1); perto(formasGuia[0].x, G_AREA_X - 100); perto(formasGuia[0].w, G_CARD_W);
    perto(alvosGuia[0].x, G_AREA_X); perto(alvosGuia[0].w, G_CARD_W - 100);
    alvosGuia[0].focar(alvosGuia[0].a, alvosGuia[0].b); assert(focoLin == 0 && focoCol == 1 && !pediuCanal);
    desenharCardVisivel(&canais[2], G_AREA_DIR - 100, G_TOPO + G_HEAD_H, 0, 2, 0, 1, 4200);
    assert(nAlvosGuia == 2); perto(alvosGuia[1].x + alvosGuia[1].w, G_AREA_DIR); perto(alvosGuia[1].w, 100);
    int cores = nCoresGuia;
    desenharCardVisivel(&canais[3], G_AREA_DIR, G_TOPO + G_HEAD_H, 0, 3, 0, 1, 4200);
    assert(nAlvosGuia == 2 && nCoresGuia == cores);
    // Direct vertical and timeline movement keep the chosen channel and playback request unchanged.
    modoLista = 1; overlay = 0; toqueLimpar(); escalaGuiaTeste = 1.5f;
    toquerol_vincular(&toqueLista, (GfxRect){G_AREA_X, G_L_TOPO, G_AREA_W, G_L_BASE - G_L_TOPO}, escalaGuiaTeste,
                     0, fmaxf(0, listaAltura() + G_L_FADE - (G_L_BASE - G_L_TOPO)), 1, &rolL);
    toquerol_vincular(&toqueTempo, (GfxRect){G_L_FAIXA_X, G_TOPO, G_L_FAIXA_W, G_L_BASE - G_TOPO}, escalaGuiaTeste,
                     0, G_L_DESL_MAX, 0, &toqueTempoMin);
    rolL = toqueTempoMin = 0; toquePpm = G_L_FAIXA_W / G_L_JANELA_MIN;
    PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, (G_L_FAIXA_X + 20) * escalaGuiaTeste, (G_L_TOPO + 60) * escalaGuiaTeste};
    assert(toqueGuiaRolar(&e)); escalaGuiaTeste = 1;
    e.fase = PONT_ROL_MOVER; e.delta = -20.625f; assert(toqueGuiaRolar(&e)); perto(rolL, 13.75f);
    e.fase = PONT_ROL_SOLTAR; toqueGuiaRolar(&e);
    e.fase = PONT_ROL_INERCIA; e.delta = -9; assert(toqueGuiaRolar(&e)); perto(rolL, 19.75f);
    e.fase = PONT_ROL_FIM; toqueGuiaRolar(&e);
    e.fase = PONT_ROL_INICIO; e.eixoY = 0; assert(toqueGuiaRolar(&e));
    e.fase = PONT_ROL_MOVER; e.delta = -toquePpm * 20.625f; assert(toqueGuiaRolar(&e)); perto(tempoDesloc(), 13.75f);
    e.fase = PONT_ROL_INERCIA; e.delta = -1e6f; toqueGuiaRolar(&e); perto(toqueTempoMin, G_L_DESL_MAX); assert(!toqueGuiaRolar(&e));
    e.fase = PONT_ROL_FIM; toqueGuiaRolar(&e);
    assert(focoLin == 0 && focoCol == 1 && !pediuCanal);
    // Search results draw and register channel/programme rows inside the portrait sheet.
    guiaTesteLimpar(); buscaEstado = 2; buscaAnim = 1; buscaRol = 0; buscaFoco = -1; buscaNC = buscaNP = 1;
    buscaCanal[0] = 0; buscaProg[0].canal = 1; buscaProg[0].ini = time(NULL) + 3600; buscaProg[0].fim = buscaProg[0].ini + 1800;
    snprintf(buscaTexto, sizeof buscaTexto, "noticias"); snprintf(buscaProg[0].titulo, sizeof buscaProg[0].titulo, "%s", programasGuia[0].titulo);
    buscaDesenhar(500);
    assert(nAlvosGuia == 3);
    for (int i = 0; i < nAlvosGuia; i++) assert(alvosGuia[i].x >= 0 && alvosGuia[i].x + alvosGuia[i].w <= NV_TELA_W);
    for (int i = 0; i < nDesenhosGuia; i++) assert(desenhosGuia[i].r.x >= 0 && desenhosGuia[i].r.x + desenhosGuia[i].r.w <= NV_TELA_W);
    if (gRetrato()) { perto(recorteGuia.x, G_AREA_X - 20); perto(G_BUSCA_ROW, 120); perto(G_L_JANELA_MIN, 60); }
    else { perto(G_BUSCA_ROW, 76); perto(G_L_JANELA_MIN, 120); }
    buscaEstado = 0; focoCol = 0;
  }
  for (int h = 0; h < 2; h++) {
    nv_layout_w = 1080; nv_layout_h = alturas[h]; rotulosLongosGuiaTeste = 1;
    guiaTesteLimpar();
    float x, y, w, altura; guia_preview_rect(&x, &y, &w, &altura);
    desenharTopo(1); desenharHero(1, 4200, 4200);
    assert(G_L_BASE - G_L_TOPO >= G_L_HEAD + 3 * G_L_ROW - .01f);
    assert(w <= G_AREA_W); perto(x + w * .5f, NV_TELA_W * .5f);
    perto(furoGuia.x, x); perto(furoGuia.y, y); perto(furoGuia.w, w); perto(furoGuia.h, altura);
    for (int i = 0; i < nAlvosGuia; i++) assert(alvosGuia[i].x >= G_AREA_X && alvosGuia[i].x + alvosGuia[i].w <= G_AREA_DIR);
    guiaTesteLimpar(); previewGuiaTeste = 0; pedPreview = 1; desenharHero(1, 4200, 4200);
    for (int i = 0; i < nDesenhosGuia; i++) {
      GfxRect r = desenhosGuia[i].r;
      assert(r.x >= G_AREA_X && r.x + r.w <= G_AREA_DIR);
      if (r.y >= y) assert(r.y + r.h <= y + altura);
    }
    previewGuiaTeste = 1; pedPreview = 0;
    rotulosLongosGuiaTeste = 0;
  }
#elif defined(TESTE_SPOTLIGHT)
  focoL = 3; temPedido = 0; okPress = okLongo = 1; velY = 123;
  exercitar(&toqueSpot, &scrollY, toqueSpotRolar, 1);
  assert(focoL == 3 && !temPedido && !okPress && !okLongo && !velY);
  /* O callback mantem o destino da mola junto com o deslocamento direto. */
  perto(scrollAlvo, 18.5f);
#elif defined(TESTE_EXPLORAR)
  modo = MODO_CLIMA; caLinha = 0; caCol[0] = 3; caCol[1] = 2; pediuAbrir = 0;
  exercitar(&toqueCa[0], &caRolar[0], toqueExplorarRolar, 0);
  assert(caLinha == 0 && caCol[0] == 3 && caCol[1] == 2 && !pediuAbrir);
  exercitar(&toqueCa[1], &caRolar[1], toqueExplorarRolar, 0);
  assert(toqueCaAtiva == -1 && !pediuAbrir);
  modo = MODO_VIZ; vzLinha = 0; vzCol = 3;
  exercitar(&toqueVz[2], &toqueVzOffset[2], toqueExplorarRolar, 0);
  exercitar(&toqueVz[3], &toqueVzOffset[3], toqueExplorarRolar, 0);
  assert(toqueVzAtiva == -1 && vzLinha == 0 && vzCol == 3 && !pediuAbrir);
  /* O quinto vizinho cabe depois de arrastar, sem levar o foco junto. */
  memset(&viz, 0, sizeof viz); memset(toqueVz, 0, sizeof toqueVz);
  memset(toqueVzOffset, 0, sizeof toqueVzOffset);
  viz.g[2].n = MAPA_VIZ_ITENS; vzLinha = 0; vzCol = 4;
  toqueVz[2].livre = 1; toqueVzOffset[2] = 37.5f;
  perto(vizRolarFileira(2, 0, 1000), 278);
  perto(toqueVzOffset[2], 37.5f); assert(vzCol == 4);
  toquerol_limpar(&toqueVz[2]);
  vizRolarFileira(2, 0, 1000); perto(toqueVzOffset[2], 278);
  /* As zonas tocaveis acompanham o recorte nos dois extremos. */
  explorarAlvoFileira(90, 200, 246, 146, 100, 1100, ponteiroViz, 0, 4);
  perto(ultimoAlvo.x, 100); perto(ultimoAlvo.w, 236);
  toqueVz[2].livre = 1; vzCol = 0; ultimoAlvo.focar(ultimoAlvo.a, ultimoAlvo.b);
  assert(vzCol == 4 && !toqueVz[2].livre && !pediuAbrir);
  explorarAlvoFileira(1000, 200, 246, 146, 100, 1100, ponteiroViz, 0, 4);
  perto(ultimoAlvo.x, 1000); perto(ultimoAlvo.w, 100);
  int antes = alvos;
  explorarAlvoFileira(1200, 200, 246, 146, 100, 1100, ponteiroViz, 0, 4);
  assert(alvos == antes);
  testMobile = 0;
  nv_layout_w = 1920; perto(EX_DIR, 1840);
  testMobile = 1;
  nv_layout_w = 2400; perto(EX_DIR, 2320);
  const float alturas[] = {1920, 2340};
  const float rails[] = {0, 48, 113.6f};
  for (int h = 0; h < 2; h++) {
    nv_layout_w = 1080; nv_layout_h = alturas[h];
    for (int b = 0; b < 3; b++) {
      railExplorarTeste = rails[b];
      float x = ajustes_conteudo_x(), largura = EX_DIR - x;
      perto(x + largura * 0.5f, NV_TELA_W * 0.5f);
      assert(vzX() < EX_DIR && EX_DIR <= NV_TELA_W);
      float card = (largura - EX_CL_GAP * (EX_CL_COLS - 1)) / EX_CL_COLS;
      assert(card > 0); perto(x + card * EX_CL_COLS + EX_CL_GAP * (EX_CL_COLS - 1), EX_DIR);
      /* The current window width also determines the horizontal endpoint. */
      viz.g[2].n = MAPA_VIZ_ITENS; toqueVz[2].livre = 1;
      toqueVzOffset[2] = 1e6f;
      float fim = viz.g[2].n * (EX_VZ_CARD + EX_VZ_GAP) - EX_VZ_GAP;
      perto(vizRolarFileira(2, 0, EX_DIR - vzX()), fim - (EX_DIR - vzX()));
      explorarAlvoFileira(EX_DIR - 100, 200, 246, 146, x, EX_DIR, ponteiroViz, 0, 4);
      perto(ultimoAlvo.x + ultimoAlvo.w, EX_DIR);
    }
  }
  testMobile = 0;
  nv_layout_w = 1080; nv_layout_h = 1728; railExplorarTeste = 113.6f;
  perto(EX_DIR, 1000); /* Ordinary portrait tablet retains the original inset. */
  testMobile = 1;
  nv_layout_w = 2400; nv_layout_h = 1080; perto(EX_DIR, 2320);
  railExplorarTeste = 0;
#elif defined(TESTE_AGENDA)
  {
    const float telas[][2] = {{1080,2340}, {2340,1080}, {1080,1920}, {1920,1080}};
    const float escalas[] = {1.0f, 1.2f, 1.5f, 2.0f};
    calAno=2026;calMes=9;calDia=16;calCelula=17;
    for (int i = 0; i < 32; i++) snprintf(linhasAgendaTeste[i].dataProx, sizeof linhasAgendaTeste[i].dataProx, "2026-09-16");
    for (int t = 0; t < 4; t++) for (int s = 0; s < 4; s++) for (int r = 0; r < 2; r++) {
      nv_layout_w = telas[t][0]; nv_layout_h = telas[t][1];
      escalaAgendaTeste = uiAgendaTeste = escalas[s]; railAgendaTeste = r ? 144 * agEscala() : 0;
      assert(agColunaUnica());
      float escala = agEscala();
      AgC1 L = c1(); AgMes M = agMesMedir();
      /* Both orientations keep a full-width page and scale readable phone rows. */
      perto(L.x0 * escala, railAgendaTeste + 48); perto(L.pnX, L.x0);
      perto((L.pnX + L.pnW) * escala, nv_layout_w - 48);
      perto(L.artW, 0); perto(L.artH, 0); assert(L.lsW > P(400));
      perto(agLinhaH()*escala,184*escalas[s]);perto(agGrupoH()*escala,72*escalas[s]);
      perto(M.celH*escala,128*escalas[s]);
      perto((L.lsY + L.lsH) * escala, nv_layout_h - 66);
      /* Seven calendar columns fit, and the episode list follows below them. */
      perto(M.x, L.pnX); perto(M.gradeW, L.pnW);
      perto(M.celW * 7 + M.espacX * 6, M.gradeW);
      perto(M.painel.x, L.pnX); perto(M.painel.w, L.pnW);
      assert(M.painel.y >= M.gradeY + M.celH * 6 + M.espacY * 5 + P(24));
      /* The larger dates and all episodes form one scrolling document. */
      perto(M.painel.h, P(agTelefonePx(144 + 32 * 156)) + P(20));
      assert(M.painel.h > P(190));
      /* Real callback converts base finger motion using cached draw scale. */
      ctxAberto = 0; escalaAgendaTeste = escala;
      toqueCalendario.offset=NULL;
      toqueAgAtiva = NULL; toquerol_limpar(&toqueAgenda); scrollY = 0;
      toquerol_vincular(&toqueAgenda, (GfxRect){L.pnX,L.lsY,L.pnW,L.lsH}, escala,
                       0, alturaDoc() - L.lsH, 1, &scrollY);
      PonteiroRolagem e = {PONT_ROL_INICIO,1,0,0,(L.pnX + P(100))*escala,(L.lsY + P(20))*escala};
      assert(toqueAgendaRolar(&e)); e.fase = PONT_ROL_MOVER; e.delta = -1e6f;
      assert(toqueAgendaRolar(&e));
      perto(L.lsY + yDe(31) - scrollY + agLinhaH(), L.lsY + L.lsH);
      e.fase = PONT_ROL_FIM; toqueAgendaRolar(&e); assert(!toqueAgAtiva);
      GfxRect eventos = agMesEventos(&M);
      perto(eventos.h,32*P(agTelefonePx(156)));
      GfxRect corpoMes={M.x,M.semanaY,M.gradeW,NV_TELA_H-P(44)-M.semanaY};
      assert(corpoMes.h>0 && corpoMes.y>0);
      float maxMes=M.painel.y+M.painel.h-corpoMes.y-corpoMes.h;
      assert(maxMes>0);
      toqueAgenda.offset=NULL;
      toquerol_limpar(&toqueCalendario); toqueCalOffset = 0;
      toquerol_vincular(&toqueCalendario, corpoMes, escala, 0, maxMes, 1, &toqueCalOffset);
      e.fase = PONT_ROL_INICIO; e.x = (corpoMes.x + P(100)) * escala; e.y = (corpoMes.y + P(20)) * escala;
      assert(toqueAgendaRolar(&e));e.fase=PONT_ROL_MOVER;e.delta=-37;
      assert(toqueAgendaRolar(&e));perto(toqueCalOffset,37/escala);
      e.fase=PONT_ROL_SOLTAR;assert(toqueAgendaRolar(&e));
      assert(toqueCalendario.livre && !ctxAberto);
      e.fase = PONT_ROL_MOVER; e.delta = -1e6f;
      assert(toqueAgendaRolar(&e));
      perto(toqueCalOffset,maxMes);
      perto(M.painel.y+M.painel.h-toqueCalOffset,corpoMes.y+corpoMes.h);
      float ultimoEp=eventos.y+31*P(agTelefonePx(156))-toqueCalOffset;
      assert(ultimoEp+P(agTelefonePx(142))<=corpoMes.y+corpoMes.h);
      e.fase=PONT_ROL_MOVER;e.delta=1e6f;assert(toqueAgendaRolar(&e));perto(toqueCalOffset,0);
      e.fase = PONT_ROL_FIM; toqueAgendaRolar(&e); assert(!toqueAgAtiva);
      /* Modals remain centered dialogs; phone body/actions fit and scroll. */
      escModal = 1; escalaAgendaTeste = 1;
      const float larguras[] = {AGC_W, AGN_W, AGL_W};
      for (int m = 0; m < 3; m++) {
        GfxRect modal = agModalR(larguras[m], 3000, 1);
        assert(modal.x >= 48 && modal.y >= 48);
        assert(modal.x + modal.w <= nv_layout_w - 48);
        assert(modal.y + modal.h <= nv_layout_h - 48);
        perto(modal.x + modal.w * .5f, nv_layout_w * .5f);
        perto(modal.y + modal.h * .5f, nv_layout_h * .5f);
      }
      GfxRect modal = agModalR(AGC_W, 1800, 1), corpo = agModalCorpo(modal);
      assert(corpo.x > modal.x && corpo.x + corpo.w < modal.x + modal.w);
      assert(corpo.h > 300 && corpo.w > 400);
      ctxAberto = 1; ctxFoco = 0; toquerol_limpar(&toqueModal); toqueModalOffset = 0;
      toquerol_vincular(&toqueModal, corpo, 1, 0, 3000 - corpo.h, 1, &toqueModalOffset);
      e.fase = PONT_ROL_INICIO; e.x = corpo.x + 100; e.y = corpo.y + 20;
      assert(toqueAgendaRolar(&e) && toqueAgAtiva == &toqueModal);
      e.fase = PONT_ROL_MOVER; e.delta = -1e6f; assert(toqueAgendaRolar(&e));
      perto(toqueModalOffset + corpo.h, 3000); assert(ctxFoco == 0);
      /* A modal switch cancels captured content rather than scrolling the page. */
      ctxAberto = 2; assert(!toqueAgendaRolar(&e) && !toqueAgAtiva);
      ctxAberto = 1; e.fase = PONT_ROL_INICIO; assert(toqueAgendaRolar(&e));
      e.fase = PONT_ROL_CANCELAR; toqueAgendaRolar(&e); assert(!toqueAgAtiva);
      /* Rotation clamps the same list without reopening or losing its title. */
      toquerol_vincular(&toqueModal, corpo, 1, 0, 20, 1, &toqueModalOffset); perto(toqueModalOffset,20);
      ctxAberto = 0; toquerol_limpar(&toqueModal); toqueModal.offset = NULL;
      escModal = 0; escalaAgendaTeste = escala;
      /* Embedded Social rows do not acquire the phone page layout. */
      escForcada = 1; assert(!agColunaUnica());
      AgC1 social = lPainel(100, 500, 200, 300);
      perto(social.lsX,100); perto(social.lsW,500); perto(P(AG_ROW_H),116);
      escForcada = 0;
    }
    /* Ordinary tablets retain the original two-column layout. */
    testMobile = 0;
    nv_layout_w = 1728; nv_layout_h = 1080; escalaAgendaTeste = 1.2f; railAgendaTeste = 0;
    assert(!agColunaUnica()); AgC1 L = c1();
    perto(L.x0, P(96)); assert(L.artW > 0 && L.pnX > L.x0 + L.artW);
    AgMes M = agMesMedir(); perto(M.painel.w,366); perto(M.painel.y,222);
    nv_layout_w = 1080; nv_layout_h = 1728; assert(!agColunaUnica());
    testMobile = 1;
    nv_layout_w = 2400; nv_layout_h = 1080; escalaAgendaTeste = 1.5f;
    toquerol_limpar(&toqueAgenda); scrollY = 0;
    toquerol_limpar(&toqueCalendario); toqueCalOffset = 0;
  }
  foco = 3; calEvento = 2; ctxAberto = 0; pediuAbrir[0] = 0;
  exercitar(&toqueAgenda, &scrollY, toqueAgendaRolar, 1);
  ctxAberto = 3;
  exercitar(&toqueNoticia, &notRol, toqueAgendaRolar, 1);
  ctxAberto = 0;
  exercitar(&toqueCalendario, &toqueCalOffset, toqueAgendaRolar, 1);
  assert(!toqueAgAtiva && foco == 3 && calEvento == 2 && !ctxAberto && !pediuAbrir[0]);
  {
    AgItem it = {0};
    GfxRect janela = {100, 200, 300, 300};
    PonteiroRolagem e = {PONT_ROL_INICIO, 1, 0, 0, 220, 450};
    snprintf(it.imdb, sizeof it.imdb, "tt123");
    for (int i = 0; i < 8; i++) snprintf(manchetesTeste[i].titulo, sizeof manchetesTeste[i].titulo, "Headline %d", i);
    notFoco = 3; ctxAberto = 2;
    toqueManchetesReiniciar();
    perto(toqueMancheteY(3), 3 * AGN_LINHA);
    perto(toqueMancheteY(4), 3 * AGN_LINHA + AGN_FOCO);
    perto(toqueMancheteY(8), 7 * AGN_LINHA + AGN_FOCO);
    perto(toqueManchetePreparar(&it, 8, janela, 0, 1), 0);
    assert(rolagemAgendaTeste == toqueAgendaRolar);
    perto(toqueManchetes.maximo, 7 * AGN_LINHA + AGN_FOCO - janela.h);
    assert(toqueAgendaRolar(&e) && toqueAgAtiva == &toqueManchetes);
    e.fase = PONT_ROL_MOVER; e.delta = -37.5f;
    assert(toqueAgendaRolar(&e)); perto(toqueManchetesOffset, 25);
    assert(notFoco == 3 && !pediuAbrir[0] && ctxAberto == 2);
    /* Focus-window calculations cannot pull a captured list back. */
    perto(toqueManchetePreparar(&it, 8, janela, 3, 1), 25);
    e.fase = PONT_ROL_INERCIA; e.delta = -1e6f;
    toqueAgendaRolar(&e); perto(toqueManchetesOffset, toqueManchetes.maximo);
    assert(!toqueAgendaRolar(&e));
    assert(toqueMancheteIndice(3 * AGN_LINHA + 10, 8) == 3);
    assert(toqueMancheteIndice(3 * AGN_LINHA + AGN_FOCO + 1, 8) == 4);
    /* A refreshed headline with the same list size resets the offset. */
    snprintf(manchetesTeste[0].titulo, sizeof manchetesTeste[0].titulo, "New headline");
    perto(toqueManchetePreparar(&it, 8, janela, 1, 1), AGN_LINHA);
    assert(!toqueManchetes.livre && !toqueAgAtiva);
    e.fase = PONT_ROL_INICIO; assert(toqueAgendaRolar(&e));
    e.fase = PONT_ROL_MOVER; e.delta = -75; toqueAgendaRolar(&e);
    e.fase = PONT_ROL_CANCELAR; toqueAgendaRolar(&e);
    assert(!toqueAgAtiva && notFoco == 3);
    /* A modal never captures its still-drawn background list. */
    toquerol_vincular(&toqueAgenda, (GfxRect){0, 0, 100, 100}, 1, 0, 300, 1, &scrollY);
    e.fase = PONT_ROL_INICIO; e.x = e.y = 30; assert(!toqueAgendaRolar(&e));
    ctxAberto = 1; e.x = 220; e.y = 450; assert(!toqueAgendaRolar(&e));
    ctxAberto = 2;
    toquerol_limpar(&toqueManchetes);
    perto(toqueManchetePreparar(&it, 8, janela, 2, 1), 2 * AGN_LINHA);
    toqueManchetesReiniciar(); assert(!toqueManchetes.livre);
    perto(toqueManchetesOffset, 0);
  }
#elif defined(TESTE_NOTAS)
  foco = 1;
  exercitar(&toqueNotas, &rolar, toqueNotasRolar, 1);
  assert(foco == 1); perto(rolarAlvo, 18.5f);
#elif defined(TESTE_DIAGNOSTICO)
  focoModo = 1; focoLinha = 3; vz.rolagem = 2; vz.botao = 1;
  exercitar(&toqueRanking, &toqueRankingOffset, toqueRankingRolar, 1);
  assert(focoModo == 1 && focoLinha == 3 && vz.rolagem == 2 && vz.botao == 1);
#elif defined(TESTE_LIVETV)
  L.estado = E_PRONTO; L.botao = 1; L.atual = 2;
  exercitar(&toqueLtd, &toqueLtdOffset, toqueLtdRolar, 1);
  assert(L.estado == E_PRONTO && L.botao == 1 && L.atual == 2 && !L.aplicado);
#elif defined(TESTE_REGISTRO)
  foco = 3; focoEtapa = 2; marcaLida = 1234; pausado = 0;
  exercitar(&toqueRegistro, &toqueRegistroOffset, toqueRegistroRolar, 1);
  assert(pausado && marcaPausa == marcaLida && foco == 3 && focoEtapa == 2);
  exercitar(&toqueEtapas, &toqueEtapasOffset, toqueRegistroRolar, 1);
  assert(foco == 3 && focoEtapa == 2 && !detalhe && !focoEnviar);
#endif
  puts("utilidades_toque: OK");
  return 0;
}
