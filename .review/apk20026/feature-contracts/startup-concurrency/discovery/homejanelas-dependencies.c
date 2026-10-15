// Typed dependencies newly required by upstream, absent in old Home fixtures.
// Account "next up" is deliberately absent; this fixture selects Trakt only.
#include "contalib.h"
int ajustes_cw_do_episodio_mais_alto(void) { return 1; }
int contalib_sementes_a_seguir(ContaSemente *out, int max, int highest) {
  (void)out; (void)max; (void)highest; return 0;
}
int vistoep_estado(const char *id, int season, int episode) {
  (void)id; (void)season; (void)episode; return -1;
}
int trakt_cw_concluiu(const char *id, long long ms) { (void)id; (void)ms; return 0; }
int trakt_ativo(void) { return 1; }
