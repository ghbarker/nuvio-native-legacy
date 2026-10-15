/* Explicit idle service boundaries. Unexpected retained dependencies are
 * aborting traps in the runner, never default-success implementations. */
static CatItem fixtureItem;
static CatEp fixtureEpisodes[16];
static int fixtureEpisodeCount, fixtureReady=1, fixtureAutoNext, fixturePauseCalls, fixtureManualClears, fixtureModal, fixtureExtras, fixtureReconnect, fixtureHasURL=1;
static Uint32 fixtureClock=5000;
Uint32 SDL_GetTicks(void) { return fixtureClock; }
int dvtela_ativa(void) { return 0; }
int reacao_evento(const SDL_Event *e,int visible) { (void)e;(void)visible;return 0; }
int reacao_visivel(void) { return 0; }
int posplay_evento(const SDL_Event *e) { (void)e;return fixtureModal; }
int posplay_visivel(void) { return 0; }
void pausao_activity(void) {}
int pausao_visivel(void) { return 0; }
int stream_folha_aberta(void) { return 0; }
int video_pronto(void) { return fixtureReady; }
int video_ativo(void) { return 1; }
int video_tocando(void) { return 1; }
void video_pausar(int p) { (void)p;fixturePauseCalls++; }
double video_pos(void) { return 120.; }
double video_duracao(void) { return 2400.; }
int ajustes_proximo_auto(void) { return fixtureAutoNext; }
int ajustes_cw_concluido(void) { return 90; }
int extras_n_relacionados(void) { return fixtureExtras; }
const CatItem *cat_item(int i) { return i==0?&fixtureItem:NULL; }
int cat_n(void) { return 1; }
int cat_indice_titulo(const char *id,int preferred) { (void)preferred;return !strcmp(id,fixtureItem.imdb)?0:-1; }
int cat_acrescentar(const CatItem *c) { (void)c;assert(!"Unexpected catalog publication");return -1; }
int cat_n_episodios(int i) { return i==0?fixtureEpisodeCount:0; }
const CatEp *cat_episodio(int i,int e) { return i==0&&e>=0&&e<fixtureEpisodeCount?&fixtureEpisodes[e]:NULL; }
#ifdef HAVE_EPISODE_BUTTONS
int cat_id_stream(int i,int t,int e,char *out,unsigned n) {
  if(i!=0||t<1||e<1) return 0;
  snprintf(out,n,"ttfixture:%d:%d",t,e);return 1;
}
void posplay_troca_manual(void) { fixtureManualClears++; }
#endif

int video_reconectando(void){return fixtureReconnect;}
const char *video_url_atual(void){return fixtureHasURL?"https://fixture.invalid/video":"";}
