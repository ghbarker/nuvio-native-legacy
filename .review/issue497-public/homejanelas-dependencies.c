// Remaining typed neighbours absent from the current upstream Home fixture.
// Other current neighbours are supplied by tests/homejanelas.c itself.
int vistoep_estado(const char *id, int season, int episode) {
  (void)id; (void)season; (void)episode; return -1;
}
int trakt_cw_concluiu(const char *id, long long ms) { (void)id; (void)ms; return 0; }
