/* Real discovery parser, merge and locale formatting. Unrelated discovery
 * functions are discarded; SDK headers supply only compile-time types. */
#define desc_pedir_titulo fixture_unused_desc_pedir_titulo
#include <descoberta.c>
#undef desc_pedir_titulo

static int testLanguage = IDIOMA_EN;
int ajustes_idioma(void) { return testLanguage; }

int fixture_episodes(int language, const char *release, CatEp *episodes) {
  char metadata[2048], releaseField[160] = "";
  testLanguage = language;
  if (release)
    snprintf(releaseField, sizeof releaseField, ",\"released\":\"%s\"", release);
  snprintf(metadata, sizeof metadata,
    "{\"meta\":{\"id\":\"tt42\",\"videos\":["
    "{\"id\":\"tt42:1:1\",\"season\":1,\"episode\":1,\"name\":\"Current\","
      "\"released\":\"2023-06-18T00:00:00Z\"},"
    "{\"id\":\"tt42:1:2\",\"season\":1,\"episode\":2,\"name\":\"Next\"%s}]}}",
    releaseField);
  return parsearEpisodios(metadata, episodes, 2);
}

void fixture_display(int language, const char *iso, char *out, unsigned size) {
  testLanguage = language;
  desc_data_extenso(iso, out, size);
}
