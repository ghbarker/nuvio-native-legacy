/* Review-only stable idle identity/cache for the combined test-app's existing
 * profile ownership wrapper. These tests exercise episode/list ownership;
 * no account login, cache persistence or profile transition is claimed.
 * All queue/publication/list/selection paths remain the actual product C.
 */
const char *sessao_usuario(void) {return "";}
int perfis_ativo(void) {return 0;}
unsigned fontecache_vod_geracao(void) {return 0;}
