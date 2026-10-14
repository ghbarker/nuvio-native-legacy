// #481: render only addon-reported audio/subtitle metadata. No media probing.
#define _POSIX_C_SOURCE 200809L
#define SDL_MAIN_HANDLED
#include <assert.h>
#include "streams.c"
#include "fontecache.h"
#include "mkv.h"

static int textoAddon, ordemAddon, mobile;
static int consultasRede, consultasDebrid, aberturasVideo, consultasMkv;
float nv_layout_w = 1920, nv_layout_h = 1080;
int layout_modo_mobile(void) { return mobile; }
float gfx_escala_ui(void) { return 1; }
Uint32 SDL_GetTicks(void) { return 1000; }
int ajustes_fonte_texto_addon(void) { return textoAddon; }
int ajustes_fonte_ordem_addon(void) { return ordemAddon; }
int ajustes_selos_coloridos(void) { return 0; }
int debrid_ativo(void) { return 1; }
int p2p_ativo(void) { return 0; }
int vazao_url_aviso(const char *url) { (void)url; return 0; }
int ondever_n(const char *id) { (void)id; return 0; }
int selospacote_ativo(void) { return -1; }
unsigned selospacote_versao(void) { return 1; }
void selospacote_colorido(int sim) { (void)sim; }
int selospacote_embutidos_ok(void) { return 0; }
const SeloFiltro *selospacote_filtro(unsigned short id) { (void)id; return NULL; }
int selospacote_casar(const char *const *p, int q, unsigned short *o, int m) {
  (void)p; (void)q; (void)o; (void)m; return 0;
}
const char *i18n(const char *s) { return s; }
char *dados_ler(const char *nome) { (void)nome; return NULL; }
int dados_gravar(const char *nome, const char *s) { (void)nome; (void)s; return 1; }

// Every preview path in this fixture must leave these boundaries untouched.
int debrid_resolver_cache(const char *h, int f, char *u, unsigned t, int *fora) {
  (void)h; (void)f; (void)u; (void)t; (void)fora; consultasDebrid++;
  assert(!"reported metadata cannot resolve debrid"); return 0;
}
int debrid_resolver_escolhido(const char *h, int f, char *u, unsigned t,
                              char *servico, unsigned ns, int *pct) {
  (void)servico; (void)ns; (void)pct;
  return debrid_resolver_cache(h, f, u, t, NULL);
}
int rede_url_final_tipo(const char *u, int s, const char *const *c, char *d,
                        unsigned t, int *st, char *m, unsigned mt, long *b) {
  (void)u; (void)s; (void)c; (void)d; (void)t; (void)st; (void)m; (void)mt; (void)b;
  consultasRede++; assert(!"reported metadata cannot probe a URL"); return 0;
}
char *rede_baixar(const char *u, int s) {
  (void)u; (void)s; consultasRede++;
  assert(!"reported metadata cannot download a URL"); return NULL;
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) {
  (void)c; (void)st; return rede_baixar(u, s);
}
int video_tocar(const char *url) {
  (void)url; aberturasVideo++;
  assert(!"reported metadata cannot open video"); return 0;
}
void player_abrir(int i, const char *url) {
  (void)i; (void)video_tocar(url);
}
int mkv_faixas(const char *url, MkvFaixa *saida, int max) {
  (void)url; (void)saida; (void)max; consultasMkv++;
  assert(!"reported metadata cannot inspect media tracks"); return 0;
}

// The production preview renderer runs with text drawing captured instead of
// opening an SDL/GL window. The width limit is observed, never fabricated by
// the fixture, and each returned line carries an identity into the draw call.
typedef struct { char texto[4096]; float w, x, y, alpha; } Desenho;
static Desenho desenhos[8];
static int nTextos, nDesenhos;
TxtLinha txt_linha_corta(TxtEstilo estilo, const char *s, int r, int g, int b,
                         int a, float maxw) {
  (void)estilo; (void)r; (void)g; (void)b; (void)a;
  assert(nTextos < (int)(sizeof desenhos / sizeof desenhos[0]));
  Desenho *d = &desenhos[nTextos];
  snprintf(d->texto, sizeof d->texto, "%s", s); d->w = maxw;
  return (TxtLinha){ .tex = (GLuint)++nTextos, .w = (int)maxw, .h = 22 };
}
void txt_desenhar_alpha(TxtLinha linha, float x, float y, float a) {
  assert(linha.tex > 0 && linha.tex <= (GLuint)nTextos);
  Desenho *d = &desenhos[linha.tex - 1];
  d->x = x; d->y = y; d->alpha = a; nDesenhos++;
}
static void desenhoLimpar(void) {
  memset(desenhos, 0, sizeof desenhos); nTextos = nDesenhos = 0;
}
static void semConsultas(void) {
  assert(consultasRede == 0 && consultasDebrid == 0);
  assert(aberturasVideo == 0 && consultasMkv == 0);
}
static Stream extrairUma(const char *json) {
  Stream *s = NULL;
  assert(stream_extrair(json, "Fixture addon", &s) == 1);
  Stream copia = s[0]; free(s); semConsultas(); return copia;
}
static void colocar(const Stream *s) {
  stream_definir_lista(s, 1);
  atual = preferida = melhorFolha = -1;
  grupo = 1; foco = 0; linhaAnt = -1; nOrdem = 0;
  abreFoco = abreAnt = 0; filtro = 0; canalFolha = 0;
  soMp4 = soCache = soDub = 0;
  atualizarProvedores();
}
static float medir(const Stream *s, float abertaLinha) {
  colocar(s); abreFoco = abertaLinha;
  float alturaEsperada = alturaLinha(0, -1);
  montar(-1); assert(nOrdem == 1 && ordem[0] == 0);
  assert(fabsf(linhaH[0] - alturaEsperada) < .001f);
  return linhaH[0];
}
static void alturaEDesenho(void) {
  Stream sem = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/no-metadata.mkv\","
                          "\"name\":\"1080p\",\"description\":\"first\\nsecond\\nthird\\nfourth\"}]}");
  Stream audio = sem, legendas = sem, ambas = sem;
  snprintf(audio.audioIdiomas, sizeof audio.audioIdiomas, "en,es-419");
  snprintf(legendas.legendaIdiomas, sizeof legendas.legendaIdiomas, "pt-BR");
  ambas = audio;
  snprintf(ambas.legendaIdiomas, sizeof ambas.legendaIdiomas, "%s", legendas.legendaIdiomas);
  assert(linhasFaixas(&sem) == 0 && linhasFaixas(&audio) == 1);
  assert(linhasFaixas(&legendas) == 1 && linhasFaixas(&ambas) == 2);
  for (textoAddon = 0; textoAddon <= 1; textoAddon++) {
    for (int aberto = 0; aberto <= 2; aberto++) {
      float fracao = aberto * .5f;
      float base = medir(&sem, fracao);
      if (!textoAddon && aberto == 0) assert(base == FOLHA_LINHA_H);
      assert(fabsf(medir(&audio, fracao) - base - FOLHA_FAIXA_H) < .001f);
      assert(fabsf(medir(&legendas, fracao) - base - FOLHA_FAIXA_H) < .001f);
      assert(fabsf(medir(&ambas, fracao) - base - 2 * FOLHA_FAIXA_H) < .001f);
    }
    desenhoLimpar();
    assert(desenharFaixas(&sem, 10, 20, 300, 0, .7f) == 0);
    assert(nTextos == 0 && nDesenhos == 0);
    desenhoLimpar();
    assert(desenharFaixas(&ambas, 10, 20, 300, 1, .7f) == 2 * FOLHA_FAIXA_H);
    assert(nTextos == 2 && nDesenhos == 2);
    assert(strstr(desenhos[0].texto, i18n("Áudio")));
    assert(strstr(desenhos[1].texto, i18n("Legendas")));
    assert(strstr(desenhos[0].texto, "(419)"));
    assert(strstr(desenhos[1].texto, "(BR)"));
    assert(desenhos[0].x == 10 && desenhos[1].x == 10);
    assert(desenhos[0].y >= 20 && desenhos[0].y < 20 + FOLHA_FAIXA_H);
    assert(fabsf(desenhos[1].y - desenhos[0].y - FOLHA_FAIXA_H) < .001f);
    assert(desenhos[0].w <= 300 && desenhos[1].w <= 300);
    assert(desenhos[0].alpha == .7f && desenhos[1].alpha == .7f);
    semConsultas();
  }
}
static void descricaoAioEDesenho(void) {
  // Default AIO output omits rich language fields; its typed formatter
  // sections still report languages. Technical markers are not languages.
  Stream s = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/aio-default.mkv\","
                        "\"name\":\"AIOStreams 1080p\","
                        "\"description\":\"Movie.2024.1080p.WEB-DL.x265.mkv\\n"
                        "🎥 HEVC | 🎧 AAC 5.1\\n🌐 🇬🇧 / 🇯🇵📝 🇫🇷 🎧 DTS 7.1\"}]}");
  char texto[128];
  assert(stream_faixas_texto(&s, 0, texto, sizeof texto));
  assert(!strcmp(texto, "Inglês, Japonês"));
  assert(stream_faixas_texto(&s, 1, texto, sizeof texto));
  assert(!strcmp(texto, "Francês"));
  assert(linhasFaixas(&s) == 2);
  Stream sem = s;
  memset(sem.audioIdiomas, 0, sizeof sem.audioIdiomas);
  memset(sem.legendaIdiomas, 0, sizeof sem.legendaIdiomas);
  for (textoAddon = 0; textoAddon <= 1; textoAddon++) {
    for (int aberto = 0; aberto <= 2; aberto++) {
      float fracao = aberto * .5f;
      float base = medir(&sem, fracao);
      assert(fabsf(medir(&s, fracao) - base - 2 * FOLHA_FAIXA_H) < .001f);
      const Stream *listada = stream_item(0);
      assert(listada && linhasFaixas(listada) == 2);
      assert(!strcmp(listada->audioIdiomas, s.audioIdiomas));
      assert(!strcmp(listada->legendaIdiomas, s.legendaIdiomas));
      desenhoLimpar();
      assert(desenharFaixas(listada, 10, 20, 300, 1, .7f) == 2 * FOLHA_FAIXA_H);
      assert(nTextos == 2 && nDesenhos == 2);
      assert(!strcmp(desenhos[0].texto, "Áudio: Inglês, Japonês"));
      assert(!strcmp(desenhos[1].texto, "Legendas: Francês"));
      assert(desenhos[0].w <= 300 && desenhos[1].w <= 300);
      assert(fabsf(desenhos[1].y - desenhos[0].y - FOLHA_FAIXA_H) < .001f);
      semConsultas();
    }
  }
}
static void regionalEInfo(void) {
  Stream s = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/reported.mkv\","
                        "\"name\":\"Dual English Spanish\",\"description\":\"Original addon description\","
                        "\"audioLanguages\":[\"en\",\"eng\",\"English\",\"pt-BR\",\"pt-PT\",\"es-419\"],"
                        "\"subtitleLanguages\":[\"fr\",\"ja\"]}]}");
  char resumo[4096];
  assert(stream_faixas_texto(&s, 0, resumo, sizeof resumo));
  assert(!strcmp(resumo, "Inglês, Português (BR), Português (PT), Espanhol (419)"));
  detalheAbrir(&s);
  assert(stream_detalhe_aberto());
  assert(!strcmp(stream_detalhe_texto(), "Original addon description"));
  assert(!strcmp(detAudio, resumo));
  assert(stream_faixas_texto(&s, 1, resumo, sizeof resumo));
  assert(!strcmp(detLegendas, resumo));
  char copiaAudio[sizeof detAudio], copiaLegenda[sizeof detLegendas];
  snprintf(copiaAudio, sizeof copiaAudio, "%s", detAudio);
  snprintf(copiaLegenda, sizeof copiaLegenda, "%s", detLegendas);
  memset(s.audioIdiomas, 0, sizeof s.audioIdiomas);
  memset(s.legendaIdiomas, 0, sizeof s.legendaIdiomas);
  stream_definir_lista(&s, 1);
  assert(!strcmp(detAudio, copiaAudio) && !strcmp(detLegendas, copiaLegenda));
  detalheAbrir(&s);
  assert(!detAudio[0] && !detLegendas[0]);
  semConsultas();
}
static void longaECache(void) {
  Stream s = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/long.mkv\","
                        "\"audioLanguages\":[\"English\",\"French\",\"German\",\"Italian\",\"Spanish\","
                        "\"Portuguese\",\"Russian\",\"Japanese\",\"Korean\",\"Chinese\",\"Arabic\","
                        "\"Hindi\",\"Dutch\",\"Polish\",\"Turkish\",\"Swedish\",\"Norwegian\","
                        "\"Danish\",\"Finnish\",\"Greek\",\"Hungarian\",\"Ukrainian\"],"
                        "\"subtitleLanguages\":[\"en\",\"pt-BR\"]}]}");
  char completo[4096], curto[24];
  assert(stream_faixas_texto(&s, 0, completo, sizeof completo));
  assert(strlen(completo) > sizeof curto);
  memset(curto, '!', sizeof curto);
  assert(stream_faixas_texto(&s, 0, curto, sizeof curto));
  assert(memchr(curto, 0, sizeof curto));
  assert(strlen(curto) < sizeof curto);
  detalheAbrir(&s); assert(!strcmp(detAudio, completo));
  desenhoLimpar();
  assert(desenharFaixas(&s, 0, 0, 160, 0, 1) == 2 * FOLHA_FAIXA_H);
  assert(nDesenhos == 2 && desenhos[0].w <= 160);

  // Use the real VOD cache, whose defensive malloc/memcpy must retain both
  // metadata fields after the caller's source list has gone away.
  FontecacheEscopo escopo = {0};
  escopo.geracao = fontecache_vod_geracao();
  fontecache_vod_guardar("tt481", "movie", "fixture", &escopo, &s, 1, SDL_GetTicks());
  char audio[sizeof s.audioIdiomas], subs[sizeof s.legendaIdiomas];
  snprintf(audio, sizeof audio, "%s", s.audioIdiomas);
  snprintf(subs, sizeof subs, "%s", s.legendaIdiomas);
  memset(&s, 0, sizeof s);
  Stream *copia = NULL; int qtd = -1; Uint32 idade = 999;
  assert(fontecache_vod_pegar("tt481", "movie", "fixture", &escopo, &copia, &qtd, &idade) == FC_ACERTO);
  assert(copia && qtd == 1 && idade == 0);
  assert(!strcmp(copia[0].audioIdiomas, audio) && !strcmp(copia[0].legendaIdiomas, subs));
  colocar(copia); free(copia);
  assert(!strcmp(stream_item(0)->audioIdiomas, audio));
  assert(!strcmp(stream_item(0)->legendaIdiomas, subs));
  fontecache_vod_limpar(); semConsultas();
}
static void ausentesELimites(void) {
  Stream s = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/en_pt-BR.mkv\","
                        "\"name\":\"Dual English Spanish\",\"description\":\"An English traveller discusses French subtitles in a Japanese movie.\","
                        "\"audioLanguages\":[\"unknown\",\"und\",null],\"subtitleLanguages\":null}]}");
  char texto[128] = "must be cleared";
  assert(!stream_faixas_texto(&s, 0, texto, sizeof texto) && !texto[0]);
  assert(!stream_faixas_texto(&s, 1, texto, sizeof texto) && !texto[0]);
  assert(linhasFaixas(&s) == 0);
  assert(!stream_faixas_texto(NULL, 0, texto, sizeof texto) && !texto[0]);
  assert(!stream_faixas_texto(&s, 0, NULL, 10));
  assert(!stream_faixas_texto(&s, 0, texto, 0));
  snprintf(s.audioIdiomas, sizeof s.audioIdiomas, "  unknown, UND,  , N/A  ");
  snprintf(s.legendaIdiomas, sizeof s.legendaIdiomas, "  ,   ");
  assert(linhasFaixas(&s) == 0);
  desenhoLimpar(); assert(desenharFaixas(&s, 0, 0, 160, 0, 1) == 0);
  assert(nTextos == 0 && nDesenhos == 0);
  snprintf(s.audioIdiomas, sizeof s.audioIdiomas, " en , eng,English ,UND,unknown,pt_BR,pt-PT,es-419 ");
  assert(stream_faixas_texto(&s, 0, texto, sizeof texto));
  assert(!strcmp(texto, "Inglês, Português (BR), Português (PT), Espanhol (419)"));
  struct { unsigned char inicio; char texto[2]; unsigned char fim; } pequeno = {0xA5, {'!', '!'}, 0x5A};
  assert(!stream_faixas_texto(&s, 0, pequeno.texto, sizeof pequeno.texto));
  assert(pequeno.texto[0] == 0 && pequeno.inicio == 0xA5 && pequeno.fim == 0x5A);
  // Defensive callers can supply a Stream with no terminator; the formatter
  // must respect the field boundary and must not read subtitle/URL memory.
  memset(s.audioIdiomas, 'z', sizeof s.audioIdiomas);
  memset(s.legendaIdiomas, 'Q', sizeof s.legendaIdiomas);
  char longo[4096];
  assert(stream_faixas_texto(&s, 0, longo, sizeof longo));
  assert(strlen(longo) <= sizeof s.audioIdiomas);
  assert(!strchr(longo, 'Q'));
  semConsultas();
}
static void identidadeEOrdem(void) {
  Stream *a = NULL, *b = NULL;
  assert(stream_extrair("{\"streams\":[{\"infoHash\":\"A\",\"fileIdx\":3,\"name\":\"720p\","
                       "\"audioLanguages\":[\"en\"]},{\"infoHash\":\"B\",\"name\":\"2160p\","
                       "\"subtitleLanguages\":[\"es\"]}]}", "Addon A", &a) == 2);
  assert(stream_extrair("{\"streams\":[{\"url\":\"https://video.invalid/C.mkv\","
                       "\"name\":\"1080p\",\"audioLanguages\":[\"pt-BR\"]}]}", "Addon B", &b) == 1);
  stream_definir_lista(NULL, 0);
  stream_lista_acrescentar(b, 1, 1); stream_lista_acrescentar(a, 2, 0);
  free(a); free(b); atualizarProvedores();
  atual = 1; preferida = 2; melhorFolha = -1;
  grupo = 1; foco = 0; linhaAnt = -1; abreFoco = abreAnt = 0;
  const int porAddon[] = {1, 2, 0}, porQualidade[] = {2, 0, 1};
  for (ordemAddon = 0; ordemAddon <= 1; ordemAddon++) {
    for (textoAddon = 0; textoAddon <= 1; textoAddon++) {
      montar(-1); assert(nOrdem == 3);
      const int *esperada = ordemAddon ? porAddon : porQualidade;
      for (int i = 0; i < 3; i++) assert(ordem[i] == esperada[i]);
      Stream antes[3]; memcpy(antes, lista, sizeof antes);
      for (int i = 0; i < 3; i++) {
        desenhoLimpar(); (void)desenharFaixas(&lista[i], 0, 0, 180, i == atual, 1);
        detalheAbrir(&lista[i]);
      }
      assert(atual == 1 && preferida == 2);
      for (int i = 0; i < 3; i++) {
        assert(!strcmp(lista[i].url, antes[i].url));
        assert(!strcmp(lista[i].infoHash, antes[i].infoHash));
        assert(lista[i].fileIdx == antes[i].fileIdx);
        assert(!strcmp(lista[i].provedor, antes[i].provedor));
      }
      montar(-1); for (int i = 0; i < 3; i++) assert(ordem[i] == esperada[i]);
      semConsultas();
    }
  }
}
static void nomesExatos(void) {
  Stream s = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/exact.mkv\","
                        "\"audioLanguages\":[\"Malay\",\"Malayalam\",\"Mandarin\",\"Chinese\","
                        "\"Portuguese notes\",\"Englishman\",\"Chinese captions\",\"Brazilian\"]}]}");
  char texto[1024];
  assert(stream_faixas_texto(&s, 0, texto, sizeof texto));
  assert(!strcmp(texto, "Malaio, Malayalam, Mandarin, Chinês, Portuguese notes, Englishman, Chinese captions, Brazilian"));
  detalheAbrir(&s); assert(!strcmp(detAudio, texto));
  Stream aio = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/exact-aio.mkv\","
                          "\"description\":\"🌐 Malay | Malayalam | Mandarin | Chinese\"}]}");
  assert(stream_faixas_texto(&aio, 0, texto, sizeof texto));
  assert(!strcmp(texto, "Malaio, Malayalam, Mandarin, Chinês"));
  colocar(&aio); soDub = 1;
  assert(!passaFiltro(0) && stream_folha_n() == 0);
  montar(-1); assert(nOrdem == 0); soDub = 0; semConsultas();
}
static void filtroAudioPortugues(void) {
  static const struct { const char *idioma; int aceita; } casos[] = {
    { "pt", 1 }, { "pt-BR", 1 }, { "pt-PT", 1 }, { "PT_BR", 1 },
    { "por", 1 }, { "pob", 1 }, { "ptb", 1 }, { "pb", 1 },
    { "br", 1 }, { "ptbr", 1 }, { "pt-Latn-BR", 1 },
    { "Portuguese", 1 }, { "Português", 1 }, { "Portugues", 1 },
    { "Brazilian Portuguese", 1 }, { "Portuguese (Brazil)", 1 },
    { "Portuguese (Brasil)", 1 }, { "Portuguese (BR)", 1 },
    { "Portuguese (Portugal)", 1 }, { "Portuguese (PT)", 1 },
    { "Português (Brasil)", 1 }, { "Português (BR)", 1 },
    { "Português (Portugal)", 1 }, { "Português (PT)", 1 },
    { "  en, pt-PT  ", 1 },
    { "en", 0 }, { "Malayalam", 0 }, { "ptx", 0 }, { "x-pt", 0 },
    { "ptbravo", 0 }, { "portugueseish", 0 }, { "Portuguese notes", 0 },
    { "Portuguese (not reported)", 0 }, { "pt--BR", 0 },
    { "pt-BR-", 0 }, { "pt-BRAZILIAN", 0 }
  };
  Stream s = extrairUma("{\"streams\":[{\"url\":\"https://video.invalid/filter.mkv\","
                        "\"subtitleLanguages\":[\"pt-BR\"]}]}");
  for (size_t c = 0; c < sizeof casos / sizeof *casos; c++) {
    snprintf(s.audioIdiomas, sizeof s.audioIdiomas, "%s", casos[c].idioma);
    colocar(&s); soDub = 1;
    assert(passaFiltro(0) == casos[c].aceita);
    assert(passaChips(0) == casos[c].aceita);
    assert(stream_folha_n() == casos[c].aceita);
    montar(-1); assert(nOrdem == casos[c].aceita);
    char texto[1024]; assert(stream_faixas_texto(&s, 0, texto, sizeof texto));
    if (casos[c].aceita) assert(strstr(texto, "Português"));
  }
  // Real parser -> visible list: English audio with Portuguese subtitles
  // stays visible before the chip and disappears when Portuguese audio is on.
  Stream *fontes = NULL;
  assert(stream_extrair("{\"streams\":["
      "{\"url\":\"https://video.invalid/pt-audio.mkv\",\"audioLanguages\":[\"pt-PT\"]},"
      "{\"url\":\"https://video.invalid/pt-subs.mkv\",\"name\":\"Dual Portuguese\","
      "\"audioLanguages\":[\"en\"],\"subtitleLanguages\":[\"pt-BR\"]},"
      "{\"url\":\"https://video.invalid/subs-only.mkv\",\"description\":\"📝 Portuguese\"}"
      "]}", "Fixture addon", &fontes) == 3);
  stream_definir_lista(fontes, 3); free(fontes); atualizarProvedores();
  atual = preferida = melhorFolha = -1;
  grupo = 1; foco = 0; linhaAnt = -1; abreFoco = abreAnt = 0;
  soMp4 = soCache = soDub = 0; filtro = 0;
  assert(stream_folha_n() == 3); montar(-1); assert(nOrdem == 3);
  assert(lista[2].legendaIdiomas[0] && !lista[2].audioIdiomas[0]);
  assert(!idiomaDa(&lista[2]));
  soDub = 1;
  assert(passaFiltro(0) && !passaFiltro(1) && !passaFiltro(2));
  assert(!idiomaDa(&lista[1])); // The row's Dual badge obeys declared English audio.
  assert(stream_folha_n() == 1); montar(-1);
  assert(nOrdem == 1 && ordem[0] == 0);
  // Existing chips and provider selection combine with the same predicate.
  lista[0].mp4 = 0; soMp4 = 1; assert(!passaFiltro(0));
  lista[0].mp4 = 1; assert(passaFiltro(0));
  soCache = 1; lista[0].foraCache = 1; assert(!passaFiltro(0));
  lista[0].foraCache = 0; assert(passaFiltro(0));
  filtro = 1; snprintf(provedores[1], sizeof provedores[1], "Other addon");
  assert(!passaFiltro(0) && passaChips(0)); filtro = 0;
  soMp4 = soCache = soDub = 0;

  // These are the exact text labels consumed by the real row badge path.
  // Both legacy badges are suppressed for English declarations and retained
  // for mixed English/Portuguese declarations accepted by the same PT chip.
  static const struct { const char *rotulo, *badge; } badgesIdioma[] = {
    { "Movie Dublado", "Dublado" }, { "Movie Dual", "Dual áudio" }
  };
  for (size_t c = 0; c < sizeof badgesIdioma / sizeof *badgesIdioma; c++) {
    Stream declarada = {0};
    snprintf(declarada.rotulo, sizeof declarada.rotulo, "%s", badgesIdioma[c].rotulo);
    snprintf(declarada.audioIdiomas, sizeof declarada.audioIdiomas, "en");
    colocar(&declarada); soDub = 1;
    assert(!idiomaDa(&lista[0]));
    assert(!passaFiltro(0) && stream_folha_n() == 0);
    montar(-1); assert(nOrdem == 0);
    snprintf(declarada.audioIdiomas, sizeof declarada.audioIdiomas, "en,pt-BR");
    colocar(&declarada); soDub = 1;
    assert(!strcmp(idiomaDa(&lista[0]), badgesIdioma[c].badge));
    assert(passaFiltro(0) && stream_folha_n() == 1);
    montar(-1); assert(nOrdem == 1 && ordem[0] == 0);
  }
  soDub = 0;

  // Typed formatter languages and legacy filename cues remain supported.
  static const char *const descricoes[] = {
    "⚑ English | Portuguese", "🌐 🇬🇧 | 🇧🇷📝 🇫🇷", "Audio: English | Portuguese (Portugal)"
  };
  for (size_t c = 0; c < sizeof descricoes / sizeof *descricoes; c++) {
    char json[1024];
    snprintf(json, sizeof json, "{\"streams\":[{\"url\":\"https://video.invalid/aio.mkv\",\"description\":\"%s\"}]}", descricoes[c]);
    Stream aio = extrairUma(json); colocar(&aio); soDub = 1;
    assert(aio.audioIdiomas[0]);
    assert(passaFiltro(0) && stream_folha_n() == 1);
    if (c != 1) assert(!strcmp(idiomaDa(&lista[0]), "Dual áudio"));
    montar(-1); assert(nOrdem == 1 && ordem[0] == 0);
  }
  static const char *const legados[] = { "Movie Dublado", "Movie Dual", "Movie pt-BR" };
  for (size_t c = 0; c < sizeof legados / sizeof *legados; c++) {
    Stream legado = {0};
    snprintf(legado.rotulo, sizeof legado.rotulo, "%s", legados[c]);
    colocar(&legado); soDub = 1;
    assert(passaFiltro(0) && stream_folha_n() == 1);
  }
  soDub = 0; semConsultas();
}
int main(void) {
  for (mobile = 0; mobile <= 1; mobile++) {
    nv_layout_w = mobile ? 540 : 1920;
    nv_layout_h = mobile ? 960 : 1080;
#ifdef NV_TOUCH_UI
    assert(fabsf(FOLHA_W - (mobile ? 460 : 820)) < .001f);
#else
    assert(FOLHA_W == 820);
#endif
    alturaEDesenho(); descricaoAioEDesenho(); regionalEInfo(); longaECache(); ausentesELimites(); identidadeEOrdem();
    nomesExatos(); filtroAudioPortugues();
  }
  stream_definir_lista(NULL, 0); fontecache_vod_limpar(); semConsultas();
  puts("stream_faixas_ui: PASS (reported rows, exact names, Portuguese chip/filter/visible counts, subtitle-only negatives, AIO formats, no-data height, TV/Mobile dimensions, regions, bounded text, Info snapshot, real cache, stable order/identity, no media requests)");
  return 0;
}
