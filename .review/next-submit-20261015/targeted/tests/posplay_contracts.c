#include "posplay.c"
#include <assert.h>

static CatItem item;
static CatEp episodes[2];
static FontecacheEscopo scope={"A",1,1,1,1};
static int videoSetting,sourceSetting,manualSetting,autoSetting=1,rate=100,currentT=1,currentE=1;
static int schedules,refreshes,oldSchedules,job,canceled;
static int metadataCanceled,refreshChecks,refreshStatus=1;
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
int ajustes_proximo_auto(void) {return autoSetting;}
int ajustes_proximo_contagem(void) {return 5;}
void addons_capturar_escopo(FontecacheEscopo *out) {*out=scope;}
void fontecache_cancelar_proximo(void) {metadataCanceled++;}
void fontecache_vod_apagar(const char *a,const char *t,const char *o,const FontecacheEscopo *e) {(void)a;(void)t;(void)o;(void)e;}
int fontecache_precarregar_proximo(const char *id,const FontecacheEscopo *e) {(void)id;(void)e;oldSchedules++;return 1;}
int fontecache_vod_pegar(const char *a,const char *t,const char *o,const FontecacheEscopo *e,Stream **out,int *n,Uint32 *age) {
  (void)a;(void)t;(void)o;(void)e;*out=NULL;*n=0;*age=0;return FC_NADA;
}
int extras_n_relacionados(void) {return 0;}
const char *extras_relacionado_imdb(int i) {(void)i;return "";}
void desc_pedir_titulo(const char *id) {(void)id;}

static void frame(double at,int isSeries) {position=at;clockNow+=1000;posplay_atualizar(1,clockNow,at,duration,isSeries,0,player_janela_proximo(0));}
static void reset(void) {
  posplay_fechar();schedules=refreshes=oldSchedules=canceled=job=metadataCanceled=refreshChecks=0;refreshStatus=1;
  memset(&item,0,sizeof item);strcpy(item.imdb,"tt42");strcpy(item.tipo,"series");
  memset(episodes,0,sizeof episodes);
  for(int i=0;i<2;i++) {episodes[i].temporada=1;episodes[i].episodio=i+1;strcpy(episodes[i].data,"2020-01-01");}
  scope=(FontecacheEscopo){"A",1,1,1,1};
  videoSetting=autoSetting=1;sourceSetting=manualSetting=0;rate=100;currentT=currentE=1;
  for(int i=0;i<10;i++)frame(0,1);
}
static void event(int key) {SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;assert(posplay_evento(&e));}

static int takeNext(void) { int t=0,e=0;if(!posplay_pediu_episodio(&t,&e))return 0;assert(t==1&&e==2);return 1; }
static void automatic(void) {
 reset();sourceSetting=1;frame(credits,1);assert(oldSchedules==1);
 posplay_marcador_real(1);clockNow+=4000;frame(credits+1,1);assert(fecharEm);
 clockNow=fecharEm;posplay_atualizar(.016,clockNow,credits+6,duration,1,0,1);
 assert(pedAutomatico&&!visivel);
}
int main(int argc,char **argv) {
 const int expectWindowBug=argc==2&&!strcmp(argv[1],"--expect-window-bug");
 duration=3600;credits=3500;reset();sourceSetting=0;
 posplay_marcador_real(1);
 posplay_atualizar(.016f,1000,3597,3600,1,0,1);assert(posplay_visivel());
 posplay_recuar();posplay_aguardar_busca(1);
 posplay_atualizar(.016f,6000,2000,3600,1,0,0);
 posplay_atualizar(.016f,7000,3600,3600,1,0,1);
 posplay_aguardar_busca(0);posplay_atualizar(.016f,8000,3600,3600,1,0,1);
 assert(posplay_visivel()&&!takeNext());
 posplay_atualizar(.016f,9000,2000,3600,1,0,0);unsigned outsideDeadline=fecharEm;
 posplay_atualizar(.016f,10000,3597,3600,1,0,1);unsigned returnedDeadline=fecharEm;
 posplay_atualizar(.016f,13100,3600,3600,1,0,1);int requested=takeNext();
 printf("actual posplay reentry: outside_deadline=%u return_deadline=%u next_at_13100=%d\n",outsideDeadline,returnedDeadline,requested);
 if(expectWindowBug) { assert(outsideDeadline==14000&&returnedDeadline==14000&&!requested);posplay_fechar();return 43; }
 assert(!outsideDeadline&&returnedDeadline==13000&&requested&&!takeNext());
 puts("PASS actual posplay: final-window exit does not start countdown; reentry rearms and requests exactly once");
 duration=2000;credits=1900;
 automatic();assert(takeNext()&&!takeNext());
 posplay_busca_usuario();assert(visivel&&!dispensado&&!recuou&&aguardaBusca&&!fecharEm&&!preTentou);
 int scheduled=oldSchedules;
 for(int i=0;i<20;i++) { frame(credits+7,1);assert(!takeNext()&&oldSchedules==scheduled); }
 posplay_aguardar_busca(0);frame(credits+7,1);assert(oldSchedules==scheduled+1&&fecharEm);
 clockNow=fecharEm;frame(credits+13,1);assert(takeNext()&&!takeNext());
 puts("PASS actual posplay: consumed automatic request can be recovered by forward seek only after acknowledgement");
 automatic();assert(takeNext());posplay_busca_usuario();posplay_recuar();posplay_aguardar_busca(0);
 for(int i=0;i<20;i++) {frame(duration,1);assert(recuou&&!takeNext());}
 event(SDLK_RETURN);assert(takeNext()&&!takeNext());
 puts("PASS actual posplay: true rewind after consumed automatic request suppresses autoplay but preserves manual Next");
 for(int key=0;key<2;key++) {
  reset();sourceSetting=1;frame(credits,1);event(key?SDLK_DOWN:SDLK_ESCAPE);
  assert(dispensado&&!visivel);posplay_busca_usuario();posplay_aguardar_busca(0);
  for(int i=0;i<20;i++){frame(duration,1);assert(dispensado&&!visivel&&!takeNext());}
 }
 puts("PASS actual posplay: manual Back/Down dismissal stays sticky across later seek");
 reset();sourceSetting=1;frame(credits,1);scheduled=oldSchedules;int canceledAtStart=metadataCanceled;
 posplay_aguardar_busca(1);
 for(int i=0;i<20;i++){frame(credits+1,1);assert(!takeNext()&&!fecharEm);}
 assert(oldSchedules==scheduled&&metadataCanceled==canceledAtStart&&preTentou);
 posplay_aguardar_busca(0);frame(duration,1);assert(takeNext());
 puts("PASS actual posplay: generic automatic seek wait preserves already prepared source metadata");
 reset();sourceSetting=1;frame(credits,1);event(SDLK_RETURN);posplay_busca_usuario();
 assert(takeNext()&&!takeNext());
 posplay_fechar();assert(!pedT&&!recuou&&!aguardaBusca&&!visivel);
 puts("PASS actual posplay: explicit manual Next survives seek cancellation; closing clears old title state");
 return 0;
}
