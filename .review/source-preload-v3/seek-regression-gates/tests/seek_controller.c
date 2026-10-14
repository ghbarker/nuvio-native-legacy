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
  posplay_fechar();reset();autoNext=1;velocity=100;
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
static void makeAutomatic(void) {
  frameAt(credits-60);prontoFonte();assert(tokenAtual);
  posplay_marcador_real(1);frameAt(credits);
  atomic_fetch_add(&relogio,4000);frameAt(credits+1);
  assert(fecharEm);
  atomic_store(&relogio,fecharEm);frameAt(credits+6);
  assert(pedAutomatico&&!visivel);
}
static void assertAutomaticOnce(void) {
  int t=0,e=0;
  assert(posplay_pediu_episodio(&t,&e)&&t==1&&e==2);
  assert(!posplay_pediu_episodio(&t,&e));
}
int main(void) {
  ui=pthread_self();resetJoined();makeAutomatic();assertAutomaticOnce();
  unsigned queuedGeneration=proximopreparo_geracao();
  unsigned preparedToken=tokenAtual;assert(preparedToken&&preVideoTentou);
  posplay_busca_usuario();
  assert(proximopreparo_geracao()==queuedGeneration+1&&!proximo&&!tokenAtual);
  assert(visivel&&!dispensado&&!recuou&&aguardaBusca&&!fecharEm);
  assert(!preVideoTentou&&!preVideoRenovou&&!preTentou);
  for(int i=0;i<20;i++){frameAt(credits+7);assert(!pedT&&!tokenAtual&&!proximo);}
  posplay_aguardar_busca(0);frameAt(credits+7);prontoFonte();
  assert(tokenAtual&&tokenAtual!=preparedToken&&fecharEm&&!recuou);
  atomic_store(&relogio,fecharEm);frameAt(credits+13);assertAutomaticOnce();
  for(int i=0;i<20;i++){frameAt(duration);assert(!pedT);}
  puts("PASS actual posplay+controller: consumed automatic Next is revoked once; forward seek reschedules and requests once after settling");

  // Generic wait is the production path for an automatic credit skip. It
  // must keep already prepared native bytes and the router generation.
  resetJoined();frameAt(credits-60);prontoFonte();
  preparedToken=tokenAtual;queuedGeneration=proximopreparo_geracao();
  posplay_aguardar_busca(1);
  for(int i=0;i<20;i++)frameAt(credits+1);
  assert(tokenAtual==preparedToken&&proximopreparo_geracao()==queuedGeneration&&!cancelaBuffer);
  assert(!fecharEm&&!pedT&&preVideoTentou);
  posplay_aguardar_busca(0);frameAt(duration);assertAutomaticOnce();
  assert(tokenAtual==preparedToken&&proximopreparo_geracao()==queuedGeneration);
  puts("PASS actual posplay+controller: automatic skip-credit wait preserves prepared token and generation");

  // An explicit Back/Down remains dismissed when a later user seek starts.
  for(int which=0;which<2;which++) {
    resetJoined();frameAt(credits-60);prontoFonte();frameAt(credits);
    SDL_Event ev={0};ev.type=SDL_KEYDOWN;ev.key.keysym.sym=which?SDLK_DOWN:SDLK_ESCAPE;
    assert(posplay_evento(&ev));assert(dispensado&&!visivel&&!pedT);
    posplay_busca_usuario();posplay_aguardar_busca(0);
    for(int i=0;i<20;i++)frameAt(duration);
    assert(dispensado&&!visivel&&!pedT&&!proximo);
  }
  puts("PASS actual posplay+controller: explicit Back/Down dismissal survives a later seek");

  resetJoined();makeAutomatic();assertAutomaticOnce();
  posplay_busca_usuario();posplay_recuar();posplay_aguardar_busca(0);
  for(int i=0;i<20;i++)frameAt(duration);
  assert(recuou&&visivel&&!pedT&&!tokenAtual);
  SDL_Event ok={0};ok.type=SDL_KEYDOWN;ok.key.keysym.sym=SDLK_RETURN;
  assert(posplay_evento(&ok));assertAutomaticOnce();
  puts("PASS actual posplay+controller: true rewind after consumed auto stays at EOF; manual Next remains available");

  resetJoined();frameAt(credits-60);prontoFonte();
  queuedGeneration=proximopreparo_geracao();escopo.perfil++;
  frameAt(credits-59);
  assert(!proximo&&!tokenAtual&&proximopreparo_geracao()!=queuedGeneration);
  posplay_fechar();assert(!pedT&&!recuou&&!aguardaBusca&&!visivel);
  puts("PASS actual posplay+controller: profile changes and close release old Next ownership");
  posplay_fechar();proximopreparo_encerrar();return 0;
}
