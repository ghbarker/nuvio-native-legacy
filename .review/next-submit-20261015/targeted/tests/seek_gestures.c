// Real player gesture/commit code and real posplay live in separate TUs.
// Only transport, metadata, rendering and background-resolver boundaries double.
#include "player_boundaries.inc"
#include "fontecache.h"

static double pointerX, enginePosition, lastSeek;
static int isTouch=1, seekCalls, introRewinds, schedules, jobs;
static int engineEnded;
static FontecacheEscopo scope={"fixture-account",1,1,1,1};

float ponteiro_x(void) { return (float)pointerX; }
int ponteiro_toque(void) { return isTouch; }
float gfx_escala_ui(void) { return 1; }
void video_pausar(int pause) { (void)pause; }
void video_buscar(double target) { lastSeek=target;seekCalls++; }
double video_pos(void) { return enginePosition; }
void intro_auto_recuar(double from,double to) { assert(to<from);introRewinds++; }
void seekr_ocioso(void) {}
double intro_creditos_janela_filme(double d) { (void)d;return 900; }
double intro_fim_estimado_filme(double d) { (void)d;return 180; }
int ajustes_fonte_preparar(void) { return 1; }
int ajustes_fonte_manual(void) { return 0; }
int ajustes_proximo_auto(void) { return 1; }
int ajustes_proximo_contagem(void) { return 5; }
void addons_capturar_escopo(FontecacheEscopo *out) { *out=scope; }
void fontecache_cancelar_proximo(void) { jobs=0; }
void fontecache_vod_apagar(const char *a,const char *t,const char *o,const FontecacheEscopo *e) {
  (void)a;(void)t;(void)o;(void)e;
}
int fontecache_precarregar_proximo(const char *id,const FontecacheEscopo *e) {
  (void)id;(void)e;schedules++;jobs=1;return 1;
}
int fontecache_vod_pegar(const char *a,const char *t,const char *o,const FontecacheEscopo *e,Stream **out,int *n,Uint32 *age) {
  (void)a;(void)t;(void)o;(void)e;*out=NULL;*n=0;*age=0;return FC_NADA;
}
int extras_n_relacionados(void) { return 0; }
const char *extras_relacionado_imdb(int i) { (void)i;return ""; }
void desc_pedir_titulo(const char *id) { (void)id; }
int cat_indice_por_imdb(const char *id) { (void)id;return 0; }
int cat_indice_vivo(int idx,const char *id) { (void)id;return idx; }
int desc_episodios_carregando(int idx) { (void)idx;return 0; }
int intro_ativo(double pos,double *fim,int *tipo) { (void)pos;(void)fim;(void)tipo;return 0; }
void cred_aprender(const char *id,double dur,double pos,int source) { (void)id;(void)dur;(void)pos;(void)source; }

// Full real player_atualizar runs; unrelated transports/UI remain inert.
double video_duracao(void) { return 3000; }
int ponteiro_ativo(void) { return 0; }
int pausao_visivel(void) { return 0; }
int reacao_visivel(void) { return 0; }
int video_pausa_confirmada(void) { return !tocando; }
int video_terminou(void) { return engineEnded; }
int video_conflito_recurso(void) { return 0; }
int video_reconectando(void) { return 0; }
void video_dv_segurar(int v) { (void)v; }
int dvtela_visivel(void) { return 0; }
const char *cacheboost_cache_aviso(void) { return ""; }
int dvtela_atualizar(const DvtelaSinais *s,float dt,Uint32 now) { (void)s;(void)dt;(void)now;return 0; }
const DvtelaEstado *dvtela_estado(void) { static DvtelaEstado state;return &state; }
int video_audio_nao_suportado(void) { return 0; }
int aovivo_zap_pronto(AoVivoZap *z,Uint32 now,int *offset) { (void)z;(void)now;(void)offset;return 0; }
int video_seek_desistiu(void) { return 0; }
void mkvass_estatisticas(long *p,long *b,int *c,int *t) { *p=*b=*c=*t=0; }
int ajustes_seekr_ligado(void) { return 0; }
unsigned cat_revisao(void) { return 1; }
unsigned cat_geracao_episodios(void) { return 1; }
void desc_episodios_pendente(void) {}
void reacao_player_atualizar(float dt,Uint32 now,const CatItem *ci,int series,double pos,double dur,double cr,int next,int season) {
  (void)dt;(void)now;(void)ci;(void)series;(void)pos;(void)dur;(void)cr;(void)next;(void)season;
}
Uint32 ponteiro_last_motion(void) { return 0; }
int video_iniciando(void) { return 0; }
int episodios_aberto(void) { return 0; }
int stream_folha_aberta(void) { return 0; }
int faixas_aberta(void) { return 0; }
void pausao_atualizar(float dt,Uint32 now,int can,int idx,const char *id,const char *ep) {
  (void)dt;(void)now;(void)can;(void)idx;(void)id;(void)ep;
}
int video_tocando(void) { return tocando; }
int video_superficie_estavel(void) { return 1; }
unsigned video_bufferando_ms(void) { return 0; }
double video_buffer_fim(void) { return 0; }
void servidores_reproducao_tick(const char *u,double p,double d,int playing) { (void)u;(void)p;(void)d;(void)playing; }
int video_velocidade_atual(void) { return 100; }
void video_velocidade_recusada(void) {}
int video_retomada_inicial_estado(void) { return 1; }
void recomenda_atividade_passo(const CatItem *ci,int playing) { (void)ci;(void)playing; }
int episodios_menu_aberto_qualquer(void) { return 0; }
int ajustes_pular_creditos(void) { return 0; }
int ajustes_pular_resumo(void) { return 0; }
int ajustes_pular_abertura(void) { return 0; }
int intro_auto_ativo(double p,double *end,int *type) { (void)p;(void)end;(void)type;return 0; }
int intro_auto_pular(int type,double end,double dur,int opening,int recap,int credits) { (void)type;(void)end;(void)dur;(void)opening;(void)recap;(void)credits;return 0; }
int intro_auto_ja(int type,double end) { (void)type;(void)end;return 0; }
void intro_auto_marcar(int type,double end) { (void)type;(void)end; }
void mkvass_passo(double p) { (void)p; }
void mkvass_folga(double seconds) { (void)seconds; }
int ajustes_legenda_sync_audio(void) { return 0; }
void legsync_audio_habilitar(int on) { (void)on; }
unsigned cat_versao_episodios(void) { return 1; }
void atividade_player_passo(const CatItem *ci,int t,int e,double p,double d,int playing,int done,float dt) {
  (void)ci;(void)t;(void)e;(void)p;(void)d;(void)playing;(void)done;(void)dt;
}
int trakt_scrobble(int event,const char *id,double p,double d,double watched,int done) {
  (void)event;(void)id;(void)p;(void)d;(void)watched;(void)done;return 0;
}
void legsync_passo(const char *u,double p,double slack,int sensitive,unsigned now) {
  (void)u;(void)p;(void)slack;(void)sensitive;(void)now;
}

// Read-only observers of the actual posplay state, supplied by its TU.
int probe_rewound(void);
int probe_waiting(void);
Uint32 probe_countdown(void);

static void pointerAt(double seconds) {
  pointerX=seconds/duracaoSeg*barraPtrW;
  clockFixture+=30;
  ponteiroBuscar(0,0);
}
static void resetGesture(double origin) {
  posplay_fechar();resetFixture();
  strcpy(fixtureTitle.tipo,"series");
  memset(fixtureEpisodes,0,sizeof fixtureEpisodes);
  for(int i=0;i<2;i++) {
    fixtureEpisodes[i].temporada=1;fixtureEpisodes[i].episodio=i+1;
    strcpy(fixtureEpisodes[i].data,"2020-01-01");
  }
  aberto=comVideo=tocando=duracaoReal=1;epT=epE=1;idx=0;
  snprintf(fonteSessao,sizeof fonteSessao,"%s",fixtureStream.url);trailerSessao=0;
  duracaoSeg=3000;posSeg=enginePosition=origin;barraPtrX=0;barraPtrW=3000;
  scrubbing=retomandoSalto=scrubDir=0;engineEnded=0;
  fixtureEngineActive=fixtureEngineReady=retomadaAplicada=1;
  velmed_zerar(&velMed);
  creditosauto_zerar(&creditosAuto);
  isTouch=1;seekCalls=introRewinds=schedules=jobs=0;
  clockFixture=1000;
  for(int i=0;i<10;i++)posplay_atualizar(1,clockFixture+=1000,origin,duracaoSeg,1,0,0);
  schedules=0;
}
static void frameNative(double at,unsigned elapsed) {
  enginePosition=at;clockFixture+=elapsed;
  player_atualizar(.016f,clockFixture);
}
static int takeNext(void) {
  int t=0,e=0;
  if(!posplay_pediu_episodio(&t,&e))return 0;
  assert(t==1&&e==2);return 1;
}
int main(int argc,char **argv) {
  if(argc==2&&!strcmp(argv[1],"--jitter-only")) {
    resetGesture(3.46);pointerAt(2963.84);pointerAt(2958.46);terminarSalto();
    if(probe_rewound()||creditosAuto.buscaRecuo) {
      puts("REPRODUCED: forward gesture 3.46 -> 2963.84 -> 2958.46 incorrectly cancelled automatic Next as rewind");
      return 42;
    }
    puts("PASS actual player: forward gesture with release jitter preserves automatic Next eligibility");
    posplay_fechar();return 0;
  }
  resetGesture(3.46);
  pointerAt(2963.84);
  player_atualizar(.016f,clockFixture);
  assert(probe_waiting()&&!probe_countdown()&&!schedules&&!jobs);
  // Repeated pointer previews may cross the credits but never start source
  // preparation or an automatic request while the finger is still down.
  for(int i=0;i<40;i++) {
    pointerAt(2958.46+(i%2));player_atualizar(.016f,clockFixture);
    assert(probe_waiting()&&!probe_countdown()&&!schedules&&!jobs&&!takeNext());
  }
  terminarSalto();assert(creditosAuto.buscaPendente&&!creditosAuto.buscaRecuo);
  frameNative(3.46,100);assert(probe_waiting()&&!schedules&&!takeNext());
  frameNative(lastSeek+.5,100);assert(!probe_waiting()&&!probe_rewound()&&schedules==1&&jobs);
  engineEnded=1;frameNative(3000,1000);assert(takeNext()&&!takeNext());
  for(int i=0;i<20;i++){frameNative(3000,1000);assert(!takeNext());}
  puts("PASS actual player+posplay: drag/seek acknowledgement blocks early metadata preparation; forward jitter permits exactly one automatic Next");
  resetGesture(3.46);
  pointerAt(2963.84);pointerAt(2958.46);
  assert(scrubbing&&!seekCalls);
  assert(!probe_rewound()&&!introRewinds);
  terminarSalto();
  assert(!scrubbing&&seekCalls==1&&fabs(lastSeek-2958.46)<0.01);
  assert(!probe_rewound()&&!introRewinds&&!creditosAuto.buscaRecuo);
  puts("PASS actual player: forward touch drag with release jitter does not become rewind");

  resetGesture(2990);
  pointerAt(2980);pointerAt(2985);terminarSalto();
  assert(seekCalls==1&&introRewinds==1&&probe_rewound());
  assert(creditosAuto.buscaPendente&&creditosAuto.buscaRecuo);
  assert(fabs(creditosAuto.inicioBusca-2990)<0.01&&fabs(creditosAuto.fimBusca-2985)<0.01);
  assert(creditosauto_aguardar(&creditosAuto,3000,0,1,clockFixture));
  assert(!creditosauto_aguardar(&creditosAuto,2985.5,1,0,clockFixture+100));
  puts("PASS actual player: genuine backward drag retains rewind and rejects stale EOF");

  resetGesture(2990);pointerAt(2980);pointerAt(2985);terminarSalto();
  engineEnded=1;frameNative(3000,100);
  assert(probe_waiting()&&probe_rewound()&&!schedules&&!takeNext());
  frameNative(3000,CREDITOSAUTO_LIMITE_MS+1);
  assert(!probe_waiting()&&probe_rewound()&&!takeNext());
  for(int i=0;i<20;i++){frameNative(3000,1000);assert(!takeNext()&&!schedules);}
  SDL_Event ok={0};ok.type=SDL_KEYDOWN;ok.key.keysym.sym=SDLK_RETURN;
  assert(posplay_evento(&ok)&&takeNext()&&!takeNext());
  puts("PASS actual player+posplay: backward seek stays in episode at stale EOF, explicit Next remains usable");

  resetGesture(2900);saltar(1,0);clockFixture+=200;saltar(-1,0);
  assert(posSeg==2900);terminarSalto();
  assert(introRewinds==1&&probe_rewound()&&seekCalls==1);
  resetGesture(2900);saltar(-1,0);clockFixture+=200;saltar(1,0);clockFixture+=200;saltar(1,0);
  terminarSalto();assert(posSeg==2910&&introRewinds==1&&probe_rewound()&&seekCalls==1);
  resetGesture(2900);saltar(1,0);clockFixture+=200;saltar(-1,0);clockFixture+=200;saltar(-1,0);
  terminarSalto();assert(posSeg==2890&&introRewinds==2&&probe_rewound()&&seekCalls==1);
  resetGesture(2900);saltar(1,0);terminarSalto();
  assert(posSeg==2910&&!introRewinds&&!probe_rewound()&&seekCalls==1);
  resetGesture(1450);comVideo=tocando=0;duracaoSeg=1500;
  scrubbing=1;scrubDir=-1;scrubTocava=0;
  scrubSalto=(SaltoEst){.inicio=clockFixture-2000,.ultimaTecla=clockFixture,.rep=1};
  scrubUltimo=clockFixture;
  player_atualizar(.05f,clockFixture);
  assert(posSeg<1450&&creditosAuto.buscaPendente&&creditosAuto.buscaRecuo);
  assert(creditosAuto.inicioBusca==1450&&creditosAuto.fimBusca==posSeg&&probe_rewound());
  double backwardTarget=creditosAuto.fimBusca;
  scrubDir=1;scrubSalto.ultimaTecla=clockFixture+16;scrubUltimo=clockFixture+16;
  player_atualizar(.05f,clockFixture+16);
  assert(creditosAuto.fimBusca==backwardTarget);
  puts("PASS actual player: keyboard Left and continuous rewind preserve the original immediate cancellation policy");

  resetGesture(2900);isTouch=0;pointerAt(2950);
  assert(!scrubbing&&seekCalls==1&&!introRewinds&&!probe_rewound());
  enginePosition=2950;
  pointerAt(2920);assert(seekCalls==2&&introRewinds==1&&probe_rewound());
  puts("PASS actual player: mouse remains an immediate single seek in each direction");
  resetGesture(3.46);pointerAt(2958);terminarSalto();
  frameNative(3.46,100);assert(probe_waiting());
  frameNative(3.46,CREDITOSAUTO_LIMITE_MS+1);assert(!probe_waiting());
  assert(!probe_rewound()&&!scrubbing);
  pointerAt(2958);terminarSalto();
  assert(creditosAuto.buscaPendente);fixtureEngineActive=0;
  player_abrir(0,NULL);
  assert(!scrubbing&&!creditosAuto.buscaPendente&&!probe_rewound()&&!probe_waiting()&&!takeNext());
  puts("PASS actual player: failed seek expires; opening another title clears gesture/seek ownership");
  posplay_fechar();return 0;
}
