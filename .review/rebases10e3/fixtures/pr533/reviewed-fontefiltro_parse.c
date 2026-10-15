// Addon metadata ingestion, with the production parser and classifier.
#include "streams.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static int checks;
static Stream parse(const char *fields) {
  char json[8192];
  Stream *lista = NULL;
  int n = snprintf(json, sizeof json, "{\"streams\":[{%s}]}", fields);
  assert(n > 0 && (size_t)n < sizeof json);
  assert(stream_extrair(json, "Fixture provider", &lista) == 1);
  Stream s = lista[0];
  free(lista);
  assert(s.filtroLido);
  checks++;
  return s;
}
static void mask(const Stream *s, int cat, const char *value) {
  uint64_t expected = fontefiltro_valores(cat, value);
  assert(expected && s->filtro.valores[cat] == expected);
}
static void unknown(const Stream *s) {
  for (int k = 0; k < FONTEFILTRO_N; k++) assert(!s->filtro.valores[k]);
  assert(!s->filtro.tamanhoBytes && !s->filtro.grupo[0]);
}

int main(void) {
  Stream s = parse(
    "\"infoHash\":\"abcd\",\"fileIdx\":2,\"name\":\"AIOStreams\","
    "\"description\":\"A cached episode\","
    "\"clientResolve\":{\"filename\":\"Episode.2160p.H264-SPARKS.mkv\",\"stream\":{\"raw\":{"
      "\"torrentName\":\"Series.Whole.Season.2160p-BADGROUP\",\"size\":\"12340000000\","
      "\"parsed\":{\"resolution\":\"1080p\",\"quality\":\"WEB-DL\","
      "\"hdr\":[\"Dolby Vision\",\"HDR10+\"],\"audio\":[\"TrueHD\",\"Atmos\"],"
      "\"channels\":[\"5.1\",\"7.1\"],\"codec\":\"H265\","
      "\"languages\":[\"English\",\"Portuguese\"],\"group\":\"NTb\"}}}}"
  );
  assert(!strcmp(s.arquivo, "Episode.2160p.H264-SPARKS.mkv"));
  assert(!strcasecmp(s.filtro.grupo, "NTb"));
  mask(&s, FONTEFILTRO_RESOLUCAO, "1080p");
  mask(&s, FONTEFILTRO_QUALIDADE, "WEB-DL");
  mask(&s, FONTEFILTRO_VISUAL, "Dolby Vision|HDR10+");
  mask(&s, FONTEFILTRO_AUDIO, "TrueHD|Atmos");
  mask(&s, FONTEFILTRO_CANAIS, "5.1|7.1");
  mask(&s, FONTEFILTRO_ENCODE, "H265");
  mask(&s, FONTEFILTRO_LINGUAS, "English|Portuguese");
  assert(s.filtro.tamanhoBytes == UINT64_C(12340000000));
  assert(!s.tamanhoBytes); // raw.size is not exact per-file StreamFit evidence.

  s = parse("\"url\":\"https://media.invalid/a\","
    "\"behaviorHints\":{\"filename\":\"Film.1080p.WEB-DL.H265-NTb.mkv\",\"videoSize\":\"10000000001\"},"
    "\"clientResolve\":{\"filename\":\"Wrong.2160p-OTHER.mkv\",\"stream\":{\"raw\":{\"size\":20000000000}}}");
  assert(!strcmp(s.arquivo, "Film.1080p.WEB-DL.H265-NTb.mkv"));
  assert(s.tamanhoBytes == UINT64_C(10000000001));
  assert(s.filtro.tamanhoBytes == s.tamanhoBytes);
  FonteFiltroConfig cfg = {0}; cfg.maxGB = 10;
  assert(!fontefiltro_permite(&cfg, &s.filtro));
  s = parse("\"url\":\"https://media.invalid/b\",\"behaviorHints\":{\"videoSize\":10000000000}");
  assert(fontefiltro_permite(&cfg, &s.filtro)); // exact limit passes.
  s = parse("\"url\":\"https://media.invalid/c\",\"videoSize\":10000000001");
  assert(!s.tamanhoBytes && s.filtro.tamanhoBytes == UINT64_C(10000000001));

  // Both title and description participate; the title is not discarded when
  // a formatter also supplies a separate description.
  s = parse("\"url\":\"https://media.invalid/d\",\"name\":\"Source\","
    "\"description\":\"Separate description\",\"title\":\"Film.2160p.WEB-DL.DV.Atmos.HEVC.mkv\"");
  mask(&s, FONTEFILTRO_RESOLUCAO, "2160p");
  mask(&s, FONTEFILTRO_QUALIDADE, "WEB-DL");
  mask(&s, FONTEFILTRO_ENCODE, "HEVC");
  assert(s.filtro.valores[FONTEFILTRO_AUDIO] & fontefiltro_valores(FONTEFILTRO_AUDIO, "Atmos"));
  assert(s.filtro.valores[FONTEFILTRO_VISUAL] & fontefiltro_valores(FONTEFILTRO_VISUAL, "DV"));
  assert(!strcmp(s.descricao, "Separate description"));

  // Header names, nested unrelated objects and URL tokens are not metadata.
  s = parse("\"url\":\"https://english.HDR10.2160p.invalid/WEB-DL-NTb.mkv\","
    "\"behaviorHints\":{\"proxyHeaders\":{\"request\":{"
      "\"filename\":\"Film.2160p.DV.TrueHD.7.1.H265.ENG-NTb.mkv\",\"videoSize\":50000000000,"
      "\"resolution\":\"2160p\",\"quality\":\"REMUX\",\"codec\":\"AV1\","
      "\"languages\":[\"English\"],\"group\":\"NTb\",\"title\":\"1080p HDR10+\"}}},"
    "\"other\":{\"parsed\":{\"quality\":\"REMUX\",\"group\":\"SPARKS\"}},"
    "\"name\":\"Source\",\"description\":\"Adventure\",\"title\":\"Adventure\"");
  unknown(&s);
  assert(!s.arquivo[0]);

  s = parse("\"url\":\"https://media.invalid/e\",\"name\":\"Source\","
    "\"title\":\"The English Patient\",\"description\":\"Subtitles: English | French\"");
  assert(!s.filtro.valores[FONTEFILTRO_LINGUAS]);
  s = parse("\"url\":\"https://media.invalid/f\",\"name\":\"Source\","
    "\"description\":\"Audio languages: English | French\"");
  mask(&s, FONTEFILTRO_LINGUAS, "English|French");

  s = parse("\"infoHash\":\"efgh\",\"clientResolve\":{\"stream\":{\"raw\":{"
    "\"torrentName\":\"Film.1080p.WEB-DL.H265-NTb.mkv\",\"size\":1500000000,"
    "\"parsed\":{\"resolution\":1080,\"channels\":\"2.0\",\"videoCodec\":\"AV1\","
    "\"audioCodec\":\"AAC\",\"audioLanguages\":[\"German\"],\"release_group\":\"GROUPX\"}}}}");
  assert(!strcmp(s.arquivo, "Film.1080p.WEB-DL.H265-NTb.mkv"));
  mask(&s, FONTEFILTRO_RESOLUCAO, "1080p");
  mask(&s, FONTEFILTRO_CANAIS, "2.0");
  mask(&s, FONTEFILTRO_ENCODE, "AV1");
  mask(&s, FONTEFILTRO_AUDIO, "AAC");
  mask(&s, FONTEFILTRO_LINGUAS, "German");
  assert(!strcasecmp(s.filtro.grupo, "GROUPX"));

  const char *invalid[] = {"-1", "0", "1.5", "1e9", "1e99", "null", "false", "[]", "{}",
    "\"NaN\"", "\"1.5\"", "\"1e9\"", "\"-1\"", "18446744073709551616"};
  for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
    char fields[512];
    snprintf(fields, sizeof fields, "\"infoHash\":\"hash\",\"clientResolve\":{\"stream\":{\"raw\":{\"size\":%s}}}", invalid[i]);
    s = parse(fields);
    assert(!s.filtro.tamanhoBytes && !s.tamanhoBytes);
  }
  s = parse("\"url\":\"https://media.invalid/g\",\"title\":\"Film 2.5 GB\"");
  assert(s.filtro.tamanhoBytes == UINT64_C(2500000000) && !s.tamanhoBytes);
  s = parse("\"url\":\"https://media.invalid/h\",\"title\":\"Film 2.5 GiB\"");
  assert(s.filtro.tamanhoBytes == UINT64_C(2684354560) && !s.tamanhoBytes);

  // A missing field in one stream does not inherit the next stream's fields.
  Stream *v = NULL;
  assert(stream_extrair("{\"streams\":[{\"url\":\"https://media.invalid/first\"},"
    "{\"url\":\"https://media.invalid/second\",\"behaviorHints\":{\"filename\":\"Film.2160p-NTb.mkv\",\"videoSize\":10000000000}}]}",
    "Fixture", &v) == 2);
  unknown(&v[0]);
  assert(v[1].filtro.tamanhoBytes == UINT64_C(10000000000));
  free(v); checks++;
  printf("fontefiltro parser: PASS (%d structured, fallback and provenance cases)\n", checks);
}
