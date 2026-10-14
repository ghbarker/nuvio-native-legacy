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
         "\"description\":\"English Patient — a French film\","
         "\"behaviorHints\":{\"filename\":\"Film.German.mkv\"}", "", "");
  expect("\"description\":\"Audio: English | Subtitles: French\"", "English", "French");
  expect("\"title\":\"Film.1080p.ENG.SPA.mkv\\nLanguages: English, Japanese | Subtitles: English, French\"",
         "English,Japanese", "English,French");
  expect("\"description\":\"Audio languages: en-US / ES-419; pt_BR • yue-Hant-HK\\n"
         "Subtitle languages: fr, zh-Hant\"", "en-US,ES-419,pt_BR,yue-Hant-HK", "fr,zh-Hant");
  expect("\"title\":\"  Áudio : Português (Brasil) | Inglês\\nLegendas: Português (Portugal)\"",
         "Português (Brasil),Inglês", "Português (Portugal)");
  expect("\"description\":\"idiomas de áudio: Português (PT) / Japonês\"", "Português (PT),Japonês", "");
  expect("\"description\":\"áudio: Inglês | Subtitles: French\"", "Inglês", "French");
  expect("\"description\":\"Audio: Dolby Atmos | DTS | AAC | 5.1\\nSubtitles: English Patient | Multi | Unknown\"", "", "");
  expect("\"description\":\"The Audio: English | The Subtitles: French\\n"
         "Film.1080p.English.mkv\\nEnglish / French\\n🇬🇧 / 🇫🇷\"", "", "");
  expect("\"name\":\"Audio: English | Subtitles: French\","
         "\"other\":{\"description\":\"Audio: Japanese\"}", "", "");
  expect("\"description\":\"Audio: English | Provider: French | German\\n"
         "Subtitles: Spanish | URL: https://video.invalid/ja | Japanese\"", "English", "Spanish");
  expect("\"audioLanguages\":[\"en\"],\"description\":\"Audio: Japanese | Subtitles: French\","
         "\"title\":\"Audio: Spanish | Subtitles: German\"", "en", "French");
  expect("\"subtitleLanguages\":[\"de\"],\"description\":\"Audio: English | Subtitles: French\"", "English", "de");
  expect("\"description\":\"Audio: English\",\"title\":\"Audio: Japanese | Subtitles: French\"", "English", "French");
  expect("\"description\":\"Audio: English\\u0000 | Subtitles: French\"", "", "");
  expect("\"description\":\"Audio: English\\q\"", "", "");
  expect("\"description\":\"Audio: English\\uD800\"", "", "");
  expect("\"description\":\"Audio: English\\u0085\"", "", "");
  expect("\"description\":\"Audio: English\xC0\xAF\"", "", "");
  expect("\"title\":\"Audio: English\\u000aSubtitles: French\"", "English", "French");
  expect("\"description\":\"Audio: English\\rSubtitles: French\"", "English", "French");
  expect("\"description\":\"Audio: English\\u000dSubtitles: French\"", "English", "French");
  expect("\"description\":\"Audio: English\\\\rSubtitles: French\"", "", "");
  expect("\"description\":\"Audio: English\"broken", "", "");
  expect("\"title\":\"Film\\n🌎 English | Japanese📝 English | French\\n💾 1.2 GB\"",
         "English,Japanese", "English,French");
  expect("\"description\":\"🌎 English • Japanese📝 English • French\"",
         "English,Japanese", "English,French");
  expect("\"description\":\"🌐 🇬🇧 / 🇯🇵📝 🇬🇧 / 🇫🇷\"",
         "English,Japanese", "English,French");
  expect("\"title\":\"🎧 AAC 🔊 5.1 🗣️ 🇬🇧 / 🇯🇵📝 🇬🇧 / 🇫🇷\"",
         "English,Japanese", "English,French");
  expect("\"description\":\"Audio: 🇧🇷 / 🇵🇹 | Subtitles: 🇧🇩 / 🇵🇰\"",
         "Portuguese (Brazil),Portuguese", "Bengali,Punjabi");
  expect("\"description\":\"🌐 🇮🇳 / 🇬🇧 / 🇺🇸 / 🇲🇽📝 🇮🇳 / 🇫🇷\"", "English", "French");
  expect("\"description\":\"🌎 / 🇬🇧 / 🇫🇷\"", "", "");
  expect("\"description\":\"🌐 🇬🇧 🇯🇵\"", "", "");
  expect("\"description\":\"🌎 Multi / Dual Audio / Dubbed / Original / Unknown📝 None\"", "", "");
  expect("\"description\":\"🌐 💃🏻📝 Latino\"", "Latino", "Latino");
  expect("\"description\":\"🎧 English / 🔊 French / ⏱ Japanese / 💾 German\"", "", "");
  expect("\"description\":\"🌐 🇬🇧 🎧 🇫🇷 / French 🔊 Japanese ⏱ Italian 💾 German📝 🇪🇸💾 French\"",
         "English", "Spanish");
  expect("\"title\":\"🇬🇧 / 🇯🇵 Subs / 🇬🇧 / 🇫🇷\"", "", "English,French");
  expect("\"title\":\"Multi Subs / 🇫🇷 / 🇮🇳\"", "", "French");
  expect("\"audioLanguages\":[\"pt-BR\"],\"subtitleLanguages\":[\"de\"],"
         "\"description\":\"🌐 🇬🇧📝 🇫🇷\"", "pt-BR", "de");
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


  // PR487 maintainer regressions: exact language tokens, declared AIOStreams
  // audio marker, and separators/unknown values must keep their meaning.
  expect("\"description\":\"Audio: Malayalam | Malay\\nSubtitles: Malayalam / Malay\"",
         "Malayalam,Malay", "Malayalam,Malay");
  expect("\"audioLanguages\":[\"Malayalam\",\"Malay\",\"MalayalamDubbed\"],"
         "\"subtitleLanguages\":[\"mal\",\"msa\",\"Cantonese\"]",
         "Malayalam,Malay,MalayalamDubbed", "mal,msa,Cantonese");
  expect("\"description\":\"Audio: MalayalamDubbed | Malayan | English Patient | PortugueseRemux\\n"
         "Subtitles: CantoneseHD | ptMovie | eng123\"", "", "");
  expect("\"description\":\"Audio commentary: English | French\\n"
         "Audio tracks available: Spanish | German\\nSubtitle codecs: Portuguese | Japanese\"", "", "");
  expect("\"description\":\"Audio: English | Language filter: Portuguese | German\\n"
         "Subtitles: French | Subtitle provider: Japanese | Spanish\"", "English", "French");
  expect("\"description\":\"Audio: en_US / eng / msa / mal\\nSubtitles: yue-Hant-HK / zh-Hant\"",
         "en_US,eng,msa,mal", "yue-Hant-HK,zh-Hant");
  expect("\"description\":\"Audio: en--US | en- | en-USA.mkv | aac | dts | Commentary\"", "", "");
  expect("\"description\":\"English | Portuguese\\n🇬🇧 / 🇵🇹\\n⚐ English | French\"", "", "");
  expect("\"description\":\"⚑ English | Portuguese\"", "English,Portuguese", "");
  expect("\"description\":\"⚑️: English / Portuguese (Brazil) • pt_PT; yue-Hant-HK📝 French\"",
         "English,Portuguese (Brazil),pt_PT,yue-Hant-HK", "French");
  expect("\"title\":\"Film.1080p.mkv\\n⚑ English | Portuguese\\n💾 1.2 GB\"", "English,Portuguese", "");
  expect("\"description\":\"\\u2691 English | Portuguese\"", "English,Portuguese", "");
  expect("\"description\":\"⚑🇬🇧 / 🇧🇷 / 🇮🇳 / 🇲🇾📝 🇫🇷 / 🇮🇳\"",
         "English,Portuguese (Brazil),Malay", "French");
  expect("\"description\":\"⚑ Malayalam / Malay / Cantonese / Mandarin / Chinese (Traditional)\"",
         "Malayalam,Malay,Cantonese,Mandarin,Chinese (Traditional)", "");
  expect("\"description\":\"⚑ MalayalamDubbed | Malayan | English Patient | PortugueseRemux\"", "", "");
  expect("\"description\":\"⚑ Multi / Multilingual / Unknown / Original / Dolby Atmos / AAC\"", "", "");
  expect("\"description\":\"⚑ 🇮🇳 / 🇺🇸 / 🇲🇽 / 🇬🇧 🇵🇹\"", "", "");
  expect("\"description\":\"⚑ English | Provider: Portuguese | German\"", "English", "");
  expect("\"description\":\"⚑ English 🎧 Portuguese / German 🔊 Japanese 💾 Spanish📝 French\"",
         "English", "French");
  expect("\"description\":\"📝 French | ⚑ English | Portuguese\"", "English,Portuguese", "French");
  expect("\"description\":\"⚑ English | Subtitles: French | Malayalam\"", "English", "French,Malayalam");
  expect("\"audioLanguages\":[\"mal\"],\"description\":\"⚑ English | Portuguese📝 French\"",
         "mal", "French");
  expect("\"subtitleLanguages\":[\"de\"],\"description\":\"⚑ English | Portuguese📝 French\"",
         "English,Portuguese", "de");
  expect("\"description\":\"Subtitles: French\",\"title\":\"⚑ English | Portuguese📝 German\"",
         "English,Portuguese", "French");
  expect("\"name\":\"⚑ English | Portuguese\",\"other\":{\"title\":\"⚑ Malayalam\"}", "", "");
  expect("\"description\":\"⚑ English\\u0000 | Portuguese\"", "", "");
  expect("\"description\":\"⚑ English\\uD800\",\"title\":\"Subtitles: French\"", "", "French");
  expect("\"description\":\"⚑ English\\r📝 Portuguese\"", "English", "Portuguese");
  expect("\"description\":\"⚑ English\\nPortuguese | French\"", "English", "");
  expect("\"description\":\"Audio: Portugues | Portuguese (BR) | Portuguese (PT)\"",
         "Portugues,Portuguese (BR),Portuguese (PT)", "");
  expect("\"description\":\"⚑ Português (PT) | Português (BR) | Brazilian Portuguese\"",
         "Português (PT),Português (BR),Brazilian Portuguese", "");
  expect("\"description\":\"Áudio : Português | Malayalam\\nIdiomas de áudio : Portuguese\"",
         "Português,Malayalam,Portuguese", "");


  // Slash separators must not restart metadata labels inside a URL or turn
  // ordinary prose about subtitles into a declared subtitle list.
  expect("\"description\":\"https://video.invalid/Audio:English\"", "", "");
  expect("\"description\":\"https://video.invalid/subtitles:fr\"", "", "");
  expect("\"description\":\"Audio: English | URL: https://video.invalid/subtitles:fr\"", "English", "");
  expect("\"description\":\"Subtitles: French | URL: https://video.invalid/audio:pt\"", "", "French");
  expect("\"description\":\"https://video.invalid/Audio:English | Subtitles: French\"", "", "French");
  expect("\"description\":\"No Subs / English\\nThe Subs / French\\nEnglish Subs / Japanese\"", "", "");
  expect("\"description\":\"Audio: No Subs / English\"", "English", "");
  expect("\"title\":\"🇮🇳 Subs / French / Malayalam\"", "", "French,Malayalam");
  expect("\"title\":\"🇬🇧 🇮🇳 Subs / French / Portuguese\"", "", "French,Portuguese");
  expect("\"description\":\"🌐 🇮🇳 Subs / French\"", "", "French");
  expect("\"description\":\"⚑ ptbr / Portuguese (PT)\"", "ptbr,Portuguese (PT)", "");

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

  assert(stream_extrair("{\"title\":\"Audio: Italian | Subtitles: Spanish\",\"streams\":["
    "{\"url\":\"https://video.invalid/1\"},"
    "{\"url\":\"https://video.invalid/2\",\"description\":\"🌐 🇬🇧📝 🇫🇷\"},"
    "{\"url\":\"https://video.invalid/3\",\"title\":\"Audio: German\"}]}",
    "Audio: Japanese | Subtitles: Portuguese", &v) == 3);
  assert(!v[0].audioIdiomas[0] && !v[0].legendaIdiomas[0]);
  assert(!strcmp(v[1].audioIdiomas, "English") && !strcmp(v[1].legendaIdiomas, "French"));
  assert(!strcmp(v[2].audioIdiomas, "German") && !v[2].legendaIdiomas[0]);
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

  // A long description cannot leave a seemingly complete language prefix.
  used = (unsigned)snprintf(fields, sizeof fields, "\"description\":\"Audio: English ");
  memset(fields + used, ' ', 3000); used += 3000;
  snprintf(fields + used, sizeof fields - used, "\",\"title\":\"Subtitles: French\"");
  s = parse(fields);
  assert(!s.audioIdiomas[0] && !strcmp(s.legendaIdiomas, "French"));
  used = (unsigned)snprintf(fields, sizeof fields, "\"description\":\"Audio: ");
  for (unsigned k = 0; k < 150; k++)
    used += (unsigned)snprintf(fields + used, sizeof fields - used, "%sen-%u", k ? "," : "", k);
  snprintf(fields + used, sizeof fields - used, "\"");
  s = parse(fields);
  assert(!strncmp(s.audioIdiomas, "en-0,en-1,", strlen("en-0,en-1,")) && strlen(s.audioIdiomas) < sizeof s.audioIdiomas);
  last = strrchr(s.audioIdiomas, ',');
  assert(last && last[1] && !strncmp(last + 1, "en-", 3));
  assert(s.audioIdiomas[strlen(s.audioIdiomas) - 1] >= '0' && s.audioIdiomas[strlen(s.audioIdiomas) - 1] <= '9');

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
