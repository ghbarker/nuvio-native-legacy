// Reported addon language metadata only: no probes, track inspection or
// inference from release titles. Uses the production stream parser.
#include "streams.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static Stream parse(const char *fields) {
  char json[16384];
  Stream *v = NULL;
  int n = snprintf(json, sizeof json, "{\"streams\":[{\"url\":\"https://video.invalid/a\",%s}]}", fields);
  assert(n > 0 && (size_t)n < sizeof json);
  assert(stream_extrair(json, "Fixture provider", &v) == 1);
  Stream s = v[0];
  free(v);
  checks++;
  return s;
}
static void expect(const char *fields, const char *audio, const char *subs) {
  Stream s = parse(fields);
  if (strcmp(s.audioIdiomas, audio) || strcmp(s.legendaIdiomas, subs)) {
    fprintf(stderr, "Fixture %u: audio=[%s] expected=[%s]; subs=[%s] expected=[%s]\n",
            checks, s.audioIdiomas, audio, s.legendaIdiomas, subs);
    abort();
  }
}

int main(void) {
  expect("\"name\":\"Source\"", "", "");
  expect("\"name\":\"English/French\",\"title\":\"Film.1080p.ENG.SPA.mkv\","
         "\"description\":\"Audio: English | Subtitles: French\","
         "\"behaviorHints\":{\"filename\":\"Film.German.mkv\"}", "", "");
  expect("\"languages\":[\"en\"],\"language\":\"fr\",\"other\":{\"audioLanguages\":[\"es\"]}", "", "");
  expect("\"audioLanguages\":[\" en \",\"ES-419\",\"en\",\"es-419\",\"pt_BR\"],"
         "\"subtitleLanguages\":[\"fr\",\"pt-BR\"]", "en,ES-419,pt_BR", "fr,pt-BR");
  expect("\"audio_languages\":\"English, French | 日本語; Português (Brasil)\","
         "\"subtitle_languages\":\"zh-Hant, yue-Hant-HK\"",
         "English,French,日本語,Português (Brasil)", "zh-Hant,yue-Hant-HK");
  expect("\"audioLanguages\":[\"\",\"  \",\"und\",\"UNKNOWN\",\"null\",\"none\","
         "\"n/a\",\"undefined\",\"zxx\",\"mul\",null,false,47],"
         "\"subtitleLanguages\":null", "", "");
  expect("\"audioLanguages\":[\"https://token.invalid/en\",\"Bearer: token\",\"en\\nHeader\","
         "\"en\\rHeader\",\"en\\tHeader\",\"en\\u0000hidden\",\"<b>en</b>\",\"@en\"],"
         "\"subtitleLanguages\":[\"fr\"]", "", "fr");
  expect("\"audioLanguages\":[{\"code\":\"en\",\"name\":\"English\"},"
         "{\"language\":{\"code\":\"pt-BR\",\"name\":\"Brazilian Portuguese\"}},"
         "{\"name\":\"日本語\"},{\"lang\":\"en\",\"code\":\"junk\"}]",
         "en,pt-BR,日本語", "");
  expect("\"audioTracks\":[{\"index\":0,\"language\":\"en\",\"codec\":\"AAC\"},"
         "{\"lang\":\"ja\",\"title\":\"Commentary\"},{\"name\":\"Stereo\",\"code\":\"AAC\"},"
         "{\"title\":\"French\",\"codec\":\"fr\"},{\"language_code\":\"yue\"}],"
         "\"subtitleTracks\":[{\"lang\":\"fr\",\"forced\":true},{\"languageCode\":\"de\"}]",
         "en,ja,yue", "fr,de");
  expect("\"audio_tracks\":[{\"lang\":\"it\"}],\"subtitle_tracks\":[{\"lang\":\"es\"}]", "it", "es");
  expect("\"tracks\":{\"audio\":[{\"lang\":\"ko\"}],\"subtitles\":[{\"language\":\"zh-Hans\"}],"
         "\"video\":[{\"lang\":\"ru\"}]}", "ko", "zh-Hans");
  expect("\"tracks\":{\"audio\":\"en\",\"subtitle\":[{\"lang\":\"fr\"}]}", "en", "fr");
  expect("\"subtitles\":[{\"id\":\"external-1\",\"url\":\"https://subs.invalid/file.fr.srt\","
         "\"lang\":\"fr\"},{\"id\":\"en\",\"url\":\"https://subs.invalid/en.srt\"},"
         "{\"language\":\"pt-PT\",\"url\":\"https://subs.invalid/pt.srt\"}]", "", "fr,pt-PT");
  expect("\"subtitles\":[{\"name\":\"English\",\"code\":\"en\"}]", "", "");
  expect("\"clientResolve\":{\"stream\":{\"raw\":{\"parsed\":{"
         "\"languages\":[\"English\",\"Spanish\"],\"subtitles\":[\"French\"],"
         "\"audioTracks\":[{\"lang\":\"ja\"}],\"subtitleTracks\":[{\"lang\":\"fr\"}]}}}}",
         "ja,English,Spanish", "fr,French");
  expect("\"streamData\":{\"parsedFile\":{\"languages\":[\"English\"],"
         "\"subtitles\":[\"Italian\"],\"audioTracks\":[{\"lang\":\"en\"}],"
         "\"subtitleTracks\":[{\"lang\":\"it\"}]}}", "en,English", "it,Italian");
  expect("\"raw\":{\"parsed\":{\"languages\":[\"fr\"],\"subtitles\":[\"de\"]}}", "fr", "de");
  expect("\"clientResolve\":{\"stream\":{\"parsed\":{\"languages\":[\"fr\"],"
         "\"subtitles\":[\"de\"],\"audioTracks\":[{\"lang\":\"en\"}]}}}", "en,fr", "de");
  expect("\"clientResolve\":{\"parsed\":{\"languages\":[\"ja\"],\"subtitles\":[\"pt-BR\"]}}", "ja", "pt-BR");
  expect("\"parsed\":{\"languages\":[\"fr\"],\"subtitles\":[{\"lang\":\"de\"}]},"
         "\"audioLanguages\":[\"en\"]", "en", "");
  expect("\"parsed\":{\"audioLanguages\":[\"en\"],\"subtitleLanguages\":[\"fr\"],"
         "\"languages\":[\"ignored\"],\"subtitles\":[\"ignored\"]}", "en", "fr");
  expect("\"parsed\":{\"audio_tracks\":[{\"lang\":\"ja\"}],"
         "\"tracks\":{\"subtitles\":[{\"lang\":\"ko\"}]},\"languages\":[\"ignored\"]}", "ja", "ko");
  expect("\"behaviorHints\":{\"proxyHeaders\":{\"request\":{\"audioLanguages\":[\"en\"],"
         "\"subtitleTracks\":[{\"lang\":\"fr\"}],\"tracks\":{\"audio\":[\"ja\"]}}}},"
         "\"other\":{\"streamData\":{\"parsedFile\":{\"languages\":[\"de\"]}}}", "", "");
  expect("\"audioTracks\":[{\"headers\":{\"language\":\"en\"},\"other\":{\"lang\":\"fr\"}}],"
         "\"audioLanguages\":[{\"other\":{\"code\":\"ja\"}}]", "", "");
  expect("\"audioLanguages\":[[\"en\"],{\"language\":{\"language\":{\"language\":{\"code\":\"fr\"}}}}]", "", "");
  expect("\"audioLanguages\":[\"en\",,\"fr\"],\"subtitleLanguages\":[\"de\"]", "", "de");
  expect("\"audioLanguages\":[\"en\",],\"subtitleLanguages\":[\"fr\"]", "", "fr");
  expect("\"audioLanguages\":[{\"lang\" \"en\"}],\"subtitleLanguages\":[\"fr\"]", "", "fr");
  expect("\"audioLanguages\":[\"en\\q\"],\"subtitleLanguages\":[\"fr\"]", "", "fr");
  expect("\"audioLanguages\":[\"en\\uD800\",\"en\\uDC00\",\"en\\u00ZZ\"],"
         "\"subtitleLanguages\":[\"fr\"]", "", "fr");
  expect("\"audioLanguages\":[\"Portugu\\u00eas\",\"\\u65e5\\u672c\\u8a9e\","
         "\"\\uD840\\uDC00\"],\"subtitleLanguages\":[\"中文\"]", "Português,日本語,𠀀", "中文");
  expect("\"audioLanguages\":[\"en\xC0\xAF\"],\"subtitleLanguages\":[\"fr\"]", "", "fr");

  // No borrowing metadata from the next source or from the title response.
  Stream *v = NULL;
  assert(stream_extrair("{\"subtitles\":[{\"lang\":\"it\"}],\"streams\":["
    "{\"url\":\"https://video.invalid/1\"},"
    "{\"url\":\"https://video.invalid/2\",\"audioLanguages\":[\"en\"],\"subtitles\":[{\"lang\":\"fr\"}]},"
    "{\"url\":\"https://video.invalid/3\",\"subtitleLanguages\":[\"de\"]}]}", "Fixture", &v) == 3);
  assert(!v[0].audioIdiomas[0] && !v[0].legendaIdiomas[0]);
  assert(!strcmp(v[1].audioIdiomas, "en") && !strcmp(v[1].legendaIdiomas, "fr"));
  assert(!v[2].audioIdiomas[0] && !strcmp(v[2].legendaIdiomas, "de"));
  free(v); checks++;

  // Bounds keep complete entries and UTF-8, never a partial language.
  char fields[8192];
  unsigned used = (unsigned)snprintf(fields, sizeof fields, "\"audioLanguages\":[");
  for (unsigned k = 0; k < 150; k++)
    used += (unsigned)snprintf(fields + used, sizeof fields - used, "%s\"日本語%u\"", k ? "," : "", k);
  snprintf(fields + used, sizeof fields - used, "]");
  Stream s = parse(fields);
  assert(s.audioIdiomas[0] && strlen(s.audioIdiomas) < sizeof s.audioIdiomas);
  char *last = strrchr(s.audioIdiomas, ',');
  assert(last && last[1] && !strncmp(last + 1, "日本語", strlen("日本語")));
  assert(s.audioIdiomas[strlen(s.audioIdiomas) - 1] >= '0' && s.audioIdiomas[strlen(s.audioIdiomas) - 1] <= '9');
  used = (unsigned)snprintf(fields, sizeof fields, "\"audioLanguages\":[\"");
  memset(fields + used, 'a', 1500); used += 1500;
  snprintf(fields + used, sizeof fields - used, "\",\"fr\"]");
  s = parse(fields);
  assert(!strcmp(s.audioIdiomas, "fr"));

  // Incomplete and very deeply nested values must not claim tracks.
  s = parse("\"audioLanguages\":[\"en");
  assert(!s.audioIdiomas[0] && !s.legendaIdiomas[0]);
  used = (unsigned)snprintf(fields, sizeof fields, "\"audioLanguages\":[");
  for (unsigned k = 0; k < 40; k++) fields[used++] = '[';
  used += (unsigned)snprintf(fields + used, sizeof fields - used, "\"en\"");
  for (unsigned k = 0; k < 40; k++) fields[used++] = ']';
  snprintf(fields + used, sizeof fields - used, "]");
  s = parse(fields);
  assert(!s.audioIdiomas[0]);

  printf("stream_faixas_parse: %u reported metadata fixtures passed\n", checks);
  return 0;
}
