// Actual Next controller + actual posplay; the existing boundary fixture
// supplies metadata/resolver/native doubles and the real FonteFalha ledger.
#define main original_controller_main
#include "next_preload_controller.c"
#undef main
#include "posplay.c"

static CatItem title;
static CatEp episodeList[2];
static double position, duration=2000, credits=1900;
static int velocity=100, autoNext;

double video_creditos(void) { return credits; }
double intro_creditos_seg(void) { return credits; }
double intro_creditos_janela_filme(double d) { (void)d;return 900; }
double intro_fim_estimado_filme(double d) { (void)d;return 180; }
double vel_tempo_real(double s,int rate) { return s*100.0/rate; }
int player_velocidade_efetiva(void) { return velocity; }
void player_episodio_atual(int *t,int *e) { *t=1;*e=1; }
void player_aprender_creditos(void) {}
int player_janela_proximo(double before) { return position+before*velocity/100.0>=credits; }
const CatItem *cat_item(int i) { (void)i;return &title; }
int cat_n_episodios(int i) { (void)i;return 2; }
const CatEp *cat_episodio(int i,int ep) { (void)i;return ep>=0&&ep<2?&episodeList[ep]:NULL; }
int cat_id_stream(int i,int t,int e,char *id,unsigned n) { (void)i;snprintf(id,n,"tt42:%d:%d",t,e);return 1; }
int cat_indice_por_imdb(const char *id) { (void)id;return 0; }
int cat_indice_vivo(int i,const char *id) { (void)id;return i; }
int ajustes_fonte_preparar(void) { return 1; }
int ajustes_proximo_auto(void) { return autoNext; }
int ajustes_proximo_contagem(void) { return 5; }
void fontecache_cancelar_proximo(void) {}
void fontecache_vod_apagar(const char *a,const char *t,const char *o,const FontecacheEscopo *e) {
  (void)a;(void)t;(void)o;(void)e;
}
int fontecache_precarregar_proximo(const char *id,const FontecacheEscopo *e) {
  (void)id;(void)e;assert(!"video-enabled Next must not use legacy metadata worker");return 0;
}
int extras_n_relacionados(void) { return 0; }
const char *extras_relacionado_imdb(int i) { (void)i;return ""; }
void desc_pedir_titulo(const char *id) { (void)id; }

static void frameAt(double at) {
  position=at;atomic_fetch_add(&relogio,1000);
  posplay_atualizar(1,SDL_GetTicks(),at,duration,1,0,player_janela_proximo(0));
}
static void resetJoined(void) {
  posplay_fechar();reset();autoNext=0;velocity=100;
  memset(&title,0,sizeof title);strcpy(title.imdb,"tt42");strcpy(title.tipo,"series");
  memset(episodeList,0,sizeof episodeList);
  for(int i=0;i<2;i++) {
    episodeList[i].temporada=1;episodeList[i].episodio=i+1;
    strcpy(episodeList[i].data,"2020-01-01");
  }
  for(int i=0;i<10;i++)frameAt(0);
}
static void manualNext(unsigned token) {
  int t=0,e=0;
  assert(!posplay_pediu_episodio(&t,&e)); // Auto remains OFF.
  SDL_Event event={0};event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_RETURN;
  assert(posplay_evento(&event));
  assert(posplay_pediu_episodio(&t,&e)&&t==1&&e==2);
  assert(!posplay_pediu_episodio(&t,&e));
  ProximoPreparoResultado result;
  assert(proximopreparo_consumir("tt42:1:2",&result));
  assert(result.token==token && result.indiceFonte>=0 && fontefalha_tentativas(result.politica)==4);
  assert(result.sessaoPerfil==perfis_sessao_geracao());
  proximopreparo_liberar_resultado(&result);
}
int main(void) {
  ui=pthread_self();
  // A slow full metadata query finishes after entering the end-of-duration
  // renewal window. The freshly READY native buffer survives EOF and manual OK.
  resetJoined();atomic_store(&bloquearMeta,1);frameAt(credits-60);entrou(&entrouMeta);
  for(int i=0;i<20;i++)frameAt(duration-15);
  assert(consultas==1&&!resolucoes&&!renova&&!preVideoRenovou);
  atomic_store(&bloquearMeta,0);prontoFonte();unsigned token=tokenAtual;
  frameAt(duration);
  assert(token && tokenAtual==token && iniciaBuffer==1&&!cancelaBuffer&&!renova&&!preVideoRenovou);
  assert(reservas==4&&http==4&&resolucoes==1);
  manualNext(token);
  puts("PASS joined: late metadata -> fresh READY at EOF -> manual Next preserves original token/budget; Auto OFF");

  // Seek directly from the early preparation window to credits/end. Playback
  // position changes do not age or invalidate the already prepared native bytes.
  resetJoined();frameAt(credits-60);prontoFonte();token=tokenAtual;
  frameAt(duration);
  assert(tokenAtual==token && iniciaBuffer==1&&!cancelaBuffer&&!renova&&!preVideoRenovou);
  manualNext(token);
  puts("PASS joined: seek to credits/end keeps fresh READY token; manual Next consumes once");

  // Preserving now must leave the single refresh available after a real-clock
  // pause at EOF. At the exact buffer TTL refresh the same winner once.
  resetJoined();frameAt(credits-60);prontoFonte();token=tokenAtual;
  frameAt(duration);assert(!preVideoRenovou&&!renova&&tokenAtual==token);
  atomic_store(&relogio,proximo->resolvidaEm+FA_BUFFER_VALE_MS-1000u);
  frameAt(duration);assert(preVideoRenovou);prontoFonte();unsigned refreshed=tokenAtual;
  assert(refreshed&&refreshed!=token&&cancelaBuffer==1&&iniciaBuffer==2&&renova==1);
  for(int i=0;i<20;i++)frameAt(duration);
  assert(renova==1&&reservas==4&&http==4&&resolucoes==1);
  manualNext(refreshed);
  puts("PASS joined: stale READY at EOF renews same spent winner once, no new candidate reservation");
  posplay_fechar();proximopreparo_encerrar();
  return 0;
}
