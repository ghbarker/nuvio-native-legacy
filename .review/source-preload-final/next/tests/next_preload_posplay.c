#include "posplay.c"
#include <assert.h>

static CatItem item;
static CatEp episodes[2];
static FontecacheEscopo scope={"A",1,1,1};
static int videoSetting=1,sourceSetting,manualSetting,rate=100,currentT=1,currentE=1;
static int schedules,refreshes,oldSchedules,job,canceled;
static double position,duration=2000,credits=1900;
static unsigned clockNow=1000;

Uint32 SDL_GetTicks(void) {return clockNow;}
double video_creditos(void) {return credits;}
double intro_creditos_seg(void) {return credits;}
double intro_creditos_janela_filme(double dur) {(void)dur;return 900;}
double intro_fim_estimado_filme(double dur) {(void)dur;return 180;}
double vel_tempo_real(double seconds,int percent) {return seconds*100.0/percent;}
int player_velocidade_efetiva(void) {return rate;}
void player_episodio_atual(int *t,int *e) {*t=currentT;*e=currentE;}
void player_aprender_creditos(void) {}
int player_janela_proximo(double beforehand) {return position+beforehand*rate/100.0>=credits;}
const CatItem *cat_item(int idx) {(void)idx;return &item;}
int cat_n_episodios(int idx) {(void)idx;return 2;}
const CatEp *cat_episodio(int idx,int ep) {(void)idx;return ep>=0&&ep<2?&episodes[ep]:NULL;}
int cat_id_stream(int idx,int t,int e,char *id,unsigned n) {(void)idx;snprintf(id,n,"tt42:%d:%d",t,e);return 1;}
int cat_indice_por_imdb(const char *id) {(void)id;return 0;}
int cat_indice_vivo(int idx,const char *id) {(void)id;return idx;}
int ajustes_fonte_precarregar(void) {return videoSetting;}
int ajustes_fonte_preparar(void) {return sourceSetting;}
int ajustes_fonte_manual(void) {return manualSetting;}
int ajustes_proximo_auto(void) {return 1;}
int ajustes_proximo_contagem(void) {return 5;}
void addons_capturar_escopo(FontecacheEscopo *out) {*out=scope;}
void fontecache_cancelar_proximo(void) {}
void fontecache_vod_apagar(const char *a,const char *t,const char *o,const FontecacheEscopo *e) {(void)a;(void)t;(void)o;(void)e;}
int fontecache_precarregar_proximo(const char *id,const FontecacheEscopo *e) {(void)id;(void)e;oldSchedules++;return 1;}
int fontecache_vod_pegar(const char *a,const char *t,const char *o,const FontecacheEscopo *e,Stream **out,int *n,Uint32 *age) {
  (void)a;(void)t;(void)o;(void)e;*out=NULL;*n=0;*age=0;return FC_NADA;
}
void proximopreparo_passo(const char *current) {(void)current;}
void proximopreparo_cancelar(void) {if(job)canceled++;job=0;}
int proximopreparo_agendar(int idx,int t,int e,const char *current,const char *next,const FontecacheEscopo *scope) {
  assert(idx==0&&t==1&&e==2&&!strcmp(current,"tt42:1:1")&&!strcmp(next,"tt42:1:2"));(void)scope;
  schedules++;job=1;return 1;
}
int proximopreparo_renovar(const char *current,const char *next) {(void)current;(void)next;refreshes++;return 1;}
int extras_n_relacionados(void) {return 0;}
const char *extras_relacionado_imdb(int i) {(void)i;return "";}
void desc_pedir_titulo(const char *id) {(void)id;}

static void frame(double at,int isSeries) {position=at;clockNow+=1000;posplay_atualizar(1,clockNow,at,duration,isSeries,0,player_janela_proximo(0));}
static void reset(void) {
  posplay_fechar();schedules=refreshes=oldSchedules=canceled=job=0;
  memset(&item,0,sizeof item);strcpy(item.imdb,"tt42");strcpy(item.tipo,"series");
  memset(episodes,0,sizeof episodes);
  for(int i=0;i<2;i++) {episodes[i].temporada=1;episodes[i].episodio=i+1;strcpy(episodes[i].data,"2020-01-01");}
  scope=(FontecacheEscopo){"A",1,1,1};
  videoSetting=1;sourceSetting=manualSetting=0;rate=100;currentT=currentE=1;
  for(int i=0;i<10;i++)frame(0,1);
}
static void event(int key) {SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;assert(posplay_evento(&e));}
int main(void) {
  const int rates[]={50,100,200};
  for(int i=0;i<3;i++) {
    reset();rate=rates[i];double threshold=credits-60.0*rate/100;
    frame(threshold-0.01,1);assert(!schedules&&!oldSchedules);
    frame(threshold,1);assert(schedules==1&&!oldSchedules);
    assert(!posplay_visivel()); // the preload window does not display Next early
    for(int j=0;j<100;j++)frame(threshold+1,1);assert(schedules==1&&!refreshes);
    frame(duration-15.0*rate/100,1);assert(refreshes==1);
    for(int j=0;j<20;j++)frame(duration-3,1);assert(refreshes==1&&schedules==1);
    printf("PASS video60: %.1fx rate, before boundary idle, boundary prepares once, late refresh once\n",rates[i]/100.0);

    reset();rate=rates[i];videoSetting=0;sourceSetting=1;
    threshold=credits-10.0*rate/100;
    frame(credits-60.0*rate/100,1);assert(!schedules&&!oldSchedules);
    frame(threshold-0.01,1);assert(!schedules&&!oldSchedules);
    frame(threshold,1);assert(!schedules&&oldSchedules==1&&!refreshes);
    for(int j=0;j<100;j++)frame(threshold+1,1);assert(oldSchedules==1&&!schedules);
    frame(duration-15.0*rate/100,1);assert(oldSchedules==2&&!schedules);
    for(int j=0;j<20;j++)frame(duration-3,1);assert(oldSchedules==2&&!schedules&&!refreshes);
    printf("PASS metadata10: %.1fx rate, remains idle at video boundary, metadata prepares/renews once\n",rates[i]/100.0);
  }
  // Both toggles enabled still use the independent video window.
  reset();sourceSetting=1;frame(credits-60.01,1);assert(!schedules&&!oldSchedules);
  frame(credits-60,1);assert(schedules==1&&!oldSchedules);
  // Neither path may start while the duration is still settling.
  for(int legacy=0;legacy<2;legacy++) {
    reset();videoSetting=!legacy;sourceSetting=legacy;posplay_fechar();
    double threshold=credits-(legacy?10.0:60.0);
    frame(threshold,1);assert(!schedules&&!oldSchedules);
    for(int j=0;j<7;j++){frame(threshold,1);assert(!schedules&&!oldSchedules);}
    frame(threshold,1);assert(legacy?oldSchedules==1&&!schedules:schedules==1&&!oldSchedules);
  }
  reset();videoSetting=0;frame(credits,1);assert(!schedules&&!oldSchedules);
  reset();videoSetting=0;sourceSetting=1;frame(credits,1);assert(!schedules&&oldSchedules==1);
  reset();manualSetting=1;frame(credits,1);assert(!schedules);
  reset();episodes[1].data[0]=0;frame(credits,1);assert(!schedules);
  reset();strcpy(episodes[1].data,"2099-01-01");frame(credits,1);assert(!schedules);
  reset();frame(credits,0);assert(!schedules);
  reset();frame(credits,1);assert(job&&posplay_visivel());event(SDLK_ESCAPE);assert(!job&&canceled==1);
  frame(credits+1,1);assert(schedules==1&&!posplay_visivel());
  reset();frame(credits,1);assert(job);event(SDLK_DOWN);assert(!job&&canceled==1);
  reset();frame(credits,1);assert(job);posplay_fechar();assert(!job&&canceled==1);
  reset();frame(credits,1);assert(job);event(SDLK_RETURN);assert(job);
  int t=0,e=0;assert(posplay_pediu_episodio(&t,&e)&&t==1&&e==2);assert(!posplay_pediu_episodio(&t,&e));
  reset();frame(credits,1);posplay_recuar();assert(!job&&canceled==1);
  frame(credits+1,1);assert(schedules==1);event(SDLK_RETURN);assert(posplay_pediu_episodio(&t,&e));
  reset();frame(credits,1);posplay_recuar();frame(credits-30,1);frame(credits-10,1);assert(schedules==2);
  reset();frame(credits,1);posplay_aguardar_busca(1);frame(credits+1,1);assert(schedules==1&&job);
  event(SDLK_RETURN);assert(posplay_pediu_episodio(&t,&e));
  reset();frame(credits,1);scope.perfil++;frame(credits+1,1);assert(!job&&canceled==1);
  reset();frame(credits,1);currentE=2;frame(credits+1,1);assert(!job&&canceled==1);
  puts("Posplay: independent video60s + legacy metadata10s at 0.5x/1x/2x, stable/released gates, one refresh, Back/Down/Stop, manual Next, rewind and seek-ack semantics PASS");
}
