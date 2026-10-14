#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "idiomacod.h"
#include "descoberta.h"
#include "proximo.h"

int fixture_episodes(int, const char *, CatEp *);
void fixture_display(int, const char *, char *, unsigned);

#define RELEASE_MS 1687651200000LL
#define NEAR_RELEASE_MS (RELEASE_MS + 86400000LL)
#define TEST_NOW_MS 1791936000000LL

static CatItem current(void) {
  CatItem item = {0};
  strcpy(item.tipo, "series");
  item.temporada = 1; item.episodio = 1; item.progresso = 100;
  return item;
}

int main(void) {
  assert(!setenv("TZ", "America/New_York", 1));
  tzset(); /* UTC release policies must not use the phone's local midnight. */
  CatItem item = current();
  CatEp episodes[2];
  ProxSugestao suggestion;
  for (int language = 0; language < IDIOMA_N; language++) {
    char display[40];
    assert(fixture_episodes(language, "2023-06-25T00:00:00Z", episodes) == 2);
    fixture_display(language, "2023-06-25T00:00:00Z", display, sizeof display);
    assert(!strcmp(episodes[1].data, display));
    assert(!strcmp(episodes[1].dataIso, "2023-06-25T00:00:00Z"));
    assert(prox_episodio_data_ms(&episodes[1]) == RELEASE_MS);
    assert(prox_para_item(&item, episodes, 2, NEAR_RELEASE_MS, &suggestion));
    assert(suggestion.temporada == 1 && suggestion.episodio == 2);
    assert(!strcmp(suggestion.data, display));
    assert(prox_seguinte(&item, episodes, 2, 90, TEST_NOW_MS, &suggestion));
    printf("PASS locale %d (%s): localized '%s', UTC %lld, both Next policies\n",
           language, idioma_iso(language), display, RELEASE_MS);
  }
  assert(fixture_episodes(IDIOMA_EN, "2023-06-25T00:00:00Z", episodes) == 2);
  assert(!strcmp(episodes[1].data, "25 June 2023"));
  assert(prox_data_ms(episodes[1].data) == PROX_SEM_DATA);
  assert(prox_episodio_data_ms(&episodes[1]) == RELEASE_MS);

  assert(fixture_episodes(IDIOMA_EN, "2099-01-01T00:00:00Z", episodes) == 2);
  assert(prox_episodio_data_ms(&episodes[1]) > TEST_NOW_MS);
  strcpy(episodes[1].data, "2020-01-01"); /* stale display cannot bypass raw future */
  assert(!prox_seguinte(&item, episodes, 2, 90, TEST_NOW_MS, &suggestion));
  assert(!prox_para_item(&item, episodes, 2, TEST_NOW_MS, &suggestion));

  const char *unknown[] = {NULL, "", "2023", "not-a-release"};
  for (unsigned i = 0; i < sizeof unknown / sizeof *unknown; i++) {
    assert(fixture_episodes(IDIOMA_EN, unknown[i], episodes) == 2);
    assert(prox_episodio_data_ms(&episodes[1]) == PROX_SEM_DATA);
    assert(!prox_para_item(&item, episodes, 2, TEST_NOW_MS, &suggestion));
    /* Existing Continue policy deliberately allows known episodes with no
     * known release; speculative preload independently rejects unknown. */
    assert(prox_seguinte(&item, episodes, 2, 90, TEST_NOW_MS, &suggestion));
    if (unknown[i] && unknown[i][0]) {
      strcpy(episodes[1].data, "2020-01-01");
      assert(prox_episodio_data_ms(&episodes[1]) == PROX_SEM_DATA);
    }
  }
  assert(prox_episodio_data_ms(NULL) == PROX_SEM_DATA);
  CatEp legacy = {0};
  strcpy(legacy.data, "25 de junho de 2023");
  assert(prox_episodio_data_ms(&legacy) == RELEASE_MS);
  strcpy(legacy.data, "2023-06-25");
  assert(prox_episodio_data_ms(&legacy) == RELEASE_MS);
  strcpy(legacy.data, "25 June 2023");
  assert(prox_episodio_data_ms(&legacy) == PROX_SEM_DATA);
  puts("PASS raw-date precedence: future/unknown cannot be bypassed by stale display; legacy Portuguese/ISO preserved");

  const char *validCalendar[] = {"2000-02-29", "2024-02-29T00:00:00Z", "1969-12-31", "1970-01-01", "2023-06-25"};
  for (unsigned i = 0; i < sizeof validCalendar / sizeof *validCalendar; i++) {
    CatEp e = {0}; strcpy(e.dataIso, validCalendar[i]);
    assert(prox_episodio_data_ms(&e) == prox_data_ms(validCalendar[i]));
  }
  const char *badCalendar[] = {"1900-02-29", "2023-02-29", "2023-02-31", "2023-04-31", "2023-00-25", "2023-13-25", "2023-06-00", "0000-06-25", "2023-6-25", "2023-06-25junk", "2023/06/25"};
  for (unsigned i = 0; i < sizeof badCalendar / sizeof *badCalendar; i++) {
    CatEp e = {0}; strcpy(e.dataIso, badCalendar[i]); strcpy(e.data, "2020-01-01");
    assert(prox_episodio_data_ms(&e) == PROX_SEM_DATA);
  }
  puts("PASS raw calendar: leap centuries, epoch/pre-epoch dates, impossible/partial dates with valid stale display; UTC in America/New_York");

  for (int mode = DESC_MESCLA_TEXTO; mode <= DESC_MESCLA_VAZIOS; mode++) {
    CatEp base[2], other[2];
    assert(fixture_episodes(IDIOMA_EN, "2023-06-25T00:00:00Z", base) == 2);
    assert(fixture_episodes(IDIOMA_PT, "2023-06-25T00:00:00Z", other) == 2);
    char retainedDisplay[40]; strcpy(retainedDisplay, base[1].data);
    base[1].dataIso[0] = 0; /* localized display already present, raw missing */
    desc_mesclar_episodios(base, 2, other, 2, mode);
    assert(!strcmp(base[1].data, retainedDisplay));
    assert(!strcmp(base[1].dataIso, "2023-06-25T00:00:00Z"));
    assert(prox_episodio_data_ms(&base[1]) == RELEASE_MS);
    strcpy(base[1].dataIso, "2099-01-01T00:00:00Z");
    desc_mesclar_episodios(base, 2, other, 2, mode);
    assert(!strcmp(base[1].dataIso, "2099-01-01T00:00:00Z"));
    assert(!prox_seguinte(&item, base, 2, 90, TEST_NOW_MS, &suggestion));
    base[1].data[0] = base[1].dataIso[0] = 0;
    desc_mesclar_episodios(base, 2, other, 2, mode);
    assert(!strcmp(base[1].data, other[1].data));
    assert(!strcmp(base[1].dataIso, other[1].dataIso));
  }
  puts("PASS actual discovery merge: raw-date supplementation in both modes preserves existing display and raw precedence");
  return 0;
}
