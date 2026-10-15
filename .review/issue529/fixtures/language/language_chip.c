#include NV_STREAMS_SOURCE
#include "idiomaauto.h"
#include <assert.h>

static int falhas, casos, idioma = IDIOMA_EN;
static int sdr;
#ifdef NV_TOUCH_UI
float nv_layout_w = 1080, nv_layout_h = 2340;
int layout_modo_mobile(void) { return 1; }
#endif
int ajustes_idioma(void) { return idioma; }
float gfx_escala_ui(void) { return 1; }
int video_pode_forcar_sdr(void) { return sdr; }
const char *i18n(const char *s) { return s; }
TxtLinha txt_linha(TxtEstilo tipo, const char *s, int r, int g, int b, int a) {
  (void)tipo; (void)r; (void)g; (void)b; (void)a;
  TxtLinha t = {0}; t.w = (int)strlen(s) * 11; t.h = 26; return t;
}

static void check(const char *what, int value) {
  casos++;
  if (!value) { printf("FAIL %s\n", what); falhas++; }
}
static int matches(const Stream *s) {
#ifdef NV_LANGUAGE_FIX
  return temAudioPreferido(s);
#else
  return temAudioPt(s);
#endif
}
static void prefs(const char *account, const char *local) {
  ling_conta_audio(account); ling_local_audio(local);
}
static void csv(Stream *s, const char *audio, const char *subs) {
  memset(s, 0, sizeof *s);
  snprintf(s->audioIdiomas, sizeof s->audioIdiomas, "%s", audio);
  snprintf(s->legendaIdiomas, sizeof s->legendaIdiomas, "%s", subs);
}
int main(void) {
  Stream st;
  prefs("", "");
  const char *locales[] = {"en-US", "pt-BR", "ro-RO", "ja-JP", "fr-FR", "de-DE", "es-419", "zh-Hant", "cs-CZ", "sv-SE", "id-ID", "uk-UA"};
  const char *metadata[] = {"eng", "pob", "ron", "jpn", "fra", "deu", "spa", "zho", "ces", "swe", "ind", "ukr"};
  for (unsigned i = 0; i < sizeof locales / sizeof *locales; i++) {
    int origin;
    idioma = idiomaauto_resolver("", "", locales[i], &origin);
    csv(&st, metadata[i], "");
    check(locales[i], matches(&st));
    csv(&st, i == 1 ? "en" : "pt", "");
    check("automatic device-language mismatch excluded", !matches(&st));
  }
  idioma = IDIOMA_PT;
  prefs("en", ""); csv(&st, "English", "Portuguese");
  check("explicit account English beats Portuguese UI", matches(&st));
  csv(&st, "Portuguese", "English");
  check("subtitle English cannot satisfy audio", !matches(&st));
  prefs("en", "fr"); csv(&st, "French", "");
  check("local audio preference beats account", matches(&st));
  csv(&st, "en", "fr"); check("account language not accepted over local", !matches(&st));
  prefs("en", ""); csv(&st, "und, ?, unknown, en_US", "");
  check("mixed unknown/BCP47 CSV matches valid audio", matches(&st));
  csv(&st, "Klingon, unknown, und", "en"); check("unknown audio not inferred from subs", !matches(&st));
  csv(&st, "", "en"); strcpy(st.arquivo, "Movie.English.DUB.DUAL.mkv");
  check("filename generic dub does not prove English", !matches(&st));
  prefs("", "pt"); csv(&st, "", ""); strcpy(st.rotulo, "Dublado Dual Portuguese");
  check("generic Portuguese labels need declared audio too", !matches(&st));
  prefs("en", "*"); csv(&st, "ro", "");
  check("local All keeps nonmatching languages", matches(&st));
  check("All does not change effective playback preference", !ling_audio()[0]);
  prefs("*", ""); csv(&st, "ja", ""); check("account All preserved", matches(&st));
  prefs("ORIGINAL", ""); check("account Original preserves file choice", matches(&st));
  prefs("en", "~"); ling_definir_original("ja"); csv(&st, "Japanese", "");
  check("local Original resolves title language", matches(&st));
  csv(&st, "en", "ja"); check("Original is not account/default language", !matches(&st));
  ling_definir_original(""); check("unknown Original remains unfiltered", matches(&st));
  prefs("DEVICE", ""); idioma = IDIOMA_IT; csv(&st, "Italian", "");
  check("account DEVICE uses resolved device UI", matches(&st));
  check("source filter leaves playback DEVICE preference unchanged", !ling_audio()[0]);
  prefs("DEFAULT", ""); check("account DEFAULT uses resolved device UI", matches(&st));
  prefs("SYSTEM", ""); check("account SYSTEM uses resolved device UI", matches(&st));
  prefs("*", "fr"); csv(&st, "fr", ""); check("explicit local language overrides account All", matches(&st));
  prefs("*", ""); prefs("en", ""); csv(&st, "ja", ""); check("new profile/account preference clears prior All mode", !matches(&st));
  prefs("en", ""); idioma = IDIOMA_PT;
  check("chip states selected audio language", strstr(rotuloBotao(BT_DUB), "EN") != NULL);
#ifdef NV_LANGUAGE_FIX
  soDub = 0; check("disabled chip help shows chosen language and enable action", strstr(ajudaFiltroAudio(), "Inglês") && strstr(ajudaFiltroAudio(), "liga"));
  soDub = 1; check("enabled chip help shows chosen language and disable action", strstr(ajudaFiltroAudio(), "Inglês") && strstr(ajudaFiltroAudio(), "tira"));
#endif
  Stream values[3];
  csv(&values[0], "en", ""); strcpy(values[0].provedor, "First"); values[0].mp4=1;
  csv(&values[1], "pt", "en"); strcpy(values[1].provedor, "Second"); values[1].mp4=1;
  csv(&values[2], "en", ""); strcpy(values[2].provedor, "First"); values[2].foraCache=1;
  lista=values; n=3; soMp4=0; soCache=0; soDub=1;
  check("source list selected chip passes English", passaChips(0));
  check("source list selected chip rejects Portuguese audio", !passaChips(1));
  soCache=1; check("cache intersection excludes uncached English", !passaChips(2));
  soCache=0; soMp4=1; check("MP4 intersection excludes non-MP4 English", !passaChips(2));
  soMp4=0; soDub=0; check("disabled audio chip keeps Portuguese", passaChips(1));
  for (sdr=0; sdr<=1; sdr++) {
    grupo=-1; foco=sdr?3:2; medirCabecalho();
    check("header widths valid", titMaxCab > 0 && bwCab[0] > 0);
    check("audio chip selected state remains owned by chip", !botaoLigado(BT_DUB));
  }
  // Current parser's audioTracks/subtitleTracks feed distinct validated CSVs.
  Stream *parsed=NULL;
  const char *json="{\"streams\":[{\"url\":\"https://example.invalid/video\",\"audioTracks\":[{\"language\":\"en\"}],\"subtitleTracks\":[{\"language\":\"pt\"}]}]}";
  check("parse one source", stream_extrair(json, "AIOStreams", &parsed)==1);
  if (parsed) {
    soDub=1; check("actual parsed English audio passes", matches(parsed));
    prefs("", "pt"); check("actual parsed Portuguese subtitles do not pass audio chip", !matches(parsed));
    char audio[512], subs[512];
    check("audio display stays English", stream_faixas_texto(parsed,0,audio,sizeof audio)>0 && strstr(audio,"Inglês"));
    check("subtitle display stays Portuguese", stream_faixas_texto(parsed,1,subs,sizeof subs)>0 && strstr(subs,"Português"));
    free(parsed);
  }
  // #529: execute the real source-list chip for Polish preferences/app fallback.
  Stream polish[3];
  csv(&polish[0], "pol", "pt");
  csv(&polish[1], "pt", "pl");
  csv(&polish[2], "", "Polish");
  lista=polish; n=3; soDub=1; soCache=soMp4=0;
  idioma=IDIOMA_PT; prefs("fr", "pl");
  check("529 explicit Polish list accepts declared Polish audio", passaChips(0));
  check("529 explicit Polish list rejects Portuguese audio", !passaChips(1));
  check("529 explicit Polish list rejects subtitle-only Polish", !passaChips(2));
  check("529 explicit Polish chip label follows audio rather than Portuguese app", strstr(rotuloBotao(BT_DUB), "PL") != NULL);
  idioma=IDIOMA_PL; prefs("", "");
  check("529 Polish app fallback list accepts declared Polish audio", passaChips(0));
  check("529 Polish app fallback list rejects Portuguese audio", !passaChips(1));
  check("529 Polish app fallback list rejects subtitle-only Polish", !passaChips(2));
  check("529 Polish app fallback chip label is Polish", strstr(rotuloBotao(BT_DUB), "PL") != NULL);
  lista=NULL; n=0;
  printf("language-chip cases=%d failures=%d\n", casos, falhas);
  return falhas ? 1 : 0;
}
