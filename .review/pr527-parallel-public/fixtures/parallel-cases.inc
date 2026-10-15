static int runPublicManualCase(const char *test);
static void parallelFinish(unsigned mask){parallelReleaseAll();parallelPumpRows(mask);parallelWaitReturned();for(int i=0;i<4;i++)tick();CHECK(parallelMaxActive<=3);}
static void parallelWaitSource(int source){unsigned end=parallelMs()+3000;while(!parallelStartedAt(source)){tick();CHECK((int)(end-parallelMs())>0);}}
static void parallelSetup(void){
 char tmp[]="/tmp/nuvio362-parallel-XXXXXX";CHECK(mkdtemp(tmp));CHECK(!setenv("NUVIO_DADOS",tmp,1));dados_iniciar(tmp);setup();
 layout_modo_definir(1);layout_tela_definir(1080,2340);uiScale=drawScale=1;
 folder.nSources=6;globalView=VT_LAY_VITRINE;gravarEscolha(&folder,VT_LAY_VITRINE);
 for(int r=0;r<6;r++)snprintf(sources[r].base,sizeof sources[r].base,"https://source%d.invalid",r);
 parallelBlockFrom(0);parallelLegacyGate=1;readonlyOpen();parallelWaitStarted(7);
 CHECK(parallelMaxActive<=3);CHECK(vitLin[0].n==0&&vitLin[1].n==0&&vitLin[2].n==0);
 printf("PARALLEL_FIRST_THREE_STARTED_WITH_FIRST_BLOCKED calls=%d active=%d max=%d\n",parallelCalls,parallelActive,parallelMaxActive);
 ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);
}
static int runParallelCase(const char *test){
 parallelSetup();
 if(!strncmp(test,"parallel-manual-",16))return runPublicManualCase(test);
 if(!strcmp(test,"parallel-order-scroll-tap")){
  parallelReleaseOne(2);parallelPumpRows(4);CHECK(vitLin[0].n==0&&vitLin[1].n==0&&vitLin[2].n==6);CHECK(!strcmp(vitItem(2,1)->imdb,"tt202"));CHECK(strstr(vitItem(2,1)->poster,"source2/title2.jpg"));
  for(int i=0;i<90;i++)tick();drawActual();float x,y;visible("tt202",&x,&y);swipe(x,y,0);float beforeY=vitScroll,beforeX=vitSx[2];CHECK(beforeX>100);
  parallelReleaseOne(1);parallelPumpRows(6);parallelReleaseOne(0);parallelPumpRows(7);drawActual();
  CHECK(vitFR==2);CHECK(fabsf(vitScroll-beforeY)<.01f&&fabsf(vitSx[2]-beforeX)<.01f);
  for(int r=0;r<3;r++){char id[64];snprintf(id,sizeof id,"tt%03d",r*100+1);CHECK(!strcmp(vitItem(r,0)->imdb,id));CHECK(strstr(vitLin[r].nome,sources[r].title));}
  visible("tt202",&x,&y);actualTap(x,y);CHECK(vertudo_pediu_abrir()>=0&&addedN==1&&!strcmp(added.imdb,"tt202"));parallelFinish(63);puts("PASS parallel rows preserve source order artwork scroll and title taps");return 0;
 }
 if(!strcmp(test,"parallel-close-reopen")||!strcmp(test,"parallel-profile-aba")){
  int profileCase=!strcmp(test,"parallel-profile-aba");unsigned before=parallelMs();
  if(profileCase){perfis_invalidar_sessao();profile=2;perfis_invalidar_sessao();profile=1;tick();CHECK(!aberta);}else vertudo_fechar_seco();
  CHECK(parallelMs()-before<200);for(int r=0;r<6;r++)snprintf(sources[r].catId,sizeof sources[r].catId,"source%d",r+10);
  other.nSources=6;globalView=VT_LAY_VITRINE;vertudo_colecao(&other);for(int i=0;i<4;i++)tick();CHECK(parallelCalls==3&&parallelMaxActive==3);
  parallelReleaseOne(0);parallelReleaseOne(1);parallelReleaseOne(2);parallelWaitSource(10);parallelWaitSource(11);parallelWaitSource(12);CHECK(parallelMaxActive<=3);
  for(int r=0;r<3;r++)CHECK(vitLin[r].n==0);
  parallelReleaseOne(10);parallelReleaseOne(11);parallelReleaseOne(12);parallelPumpRows(7);
  for(int r=0;r<3;r++){char id[64];snprintf(id,sizeof id,"tt%03d",(r+10)*100+1);CHECK(!strcmp(vitItem(r,0)->imdb,id));}
  parallelFinish(63);puts(profileCase?"PASS parallel real profile ABA epoch rejects old results":"PASS parallel close reopen rejects old results without exceeding global cap");return 0;
 }
 if(!strcmp(test,"parallel-source-replaced")){
  snprintf(sources[0].catId,sizeof sources[0].catId,"source10");snprintf(sources[0].base,sizeof sources[0].base,"https://replacement.invalid");tick();
  parallelReleaseOne(1);parallelReleaseOne(2);parallelWaitSource(10);parallelReleaseOne(10);parallelPumpRows(1);CHECK(!strcmp(vitItem(0,0)->imdb,"tt1001"));
  parallelReleaseOne(0);for(int i=0;i<8;i++)tick();CHECK(!strcmp(vitItem(0,0)->imdb,"tt1001"));parallelFinish(63);puts("PASS replaced collection source rejects old owned request result");return 0;
 }
 if(!strcmp(test,"parallel-addon-disabled")){
  snprintf(fixtureDisabledBase,sizeof fixtureDisabledBase,"%s",sources[0].base);tick();parallelReleaseOne(0);parallelReleaseOne(1);parallelReleaseOne(2);parallelPumpRows(6);
  int launches=parallelStartedAt(0);
  for(int i=0;i<20;i++){clockNow+=1600;tick();CHECK(vitLin[0].estado==VIT_VAZIA&&vitLin[0].n==0);CHECK(parallelStartedAt(0)==launches);}
  CHECK(vitLin[1].n==6&&vitLin[2].n==6);fixtureDisabledBase[0]=0;parallelReleaseOne(3);clockNow+=1600;parallelPumpRows(7);CHECK(!strcmp(vitItem(0,0)->imdb,"tt001"));CHECK(parallelStartedAt(0)==launches+1);parallelFinish(63);puts("PASS disabled addon stops requests and reenable recovers exactly once");return 0;
 }
 if(!strcmp(test,"parallel-shrink-focused-row")){
  parallelReleaseOne(2);parallelPumpRows(4);for(int i=0;i<90;i++)tick();drawActual();float x,y;visible("tt202",&x,&y);fingerEvent(SDL_FINGERDOWN,x,y,1);
  folder.nSources=1;fixtureCollectionRevision++;tick();drawActual();fingerEvent(SDL_FINGERUP,x,y,1);CHECK(vitN==1&&vitFR==0&&vitFC==0);CHECK(addedN==0&&vertudo_pediu_abrir()<0);CHECK(!vitItem(2,0));
  parallelReleaseAll();parallelPumpRows(1);parallelWaitReturned();CHECK(!strcmp(vitItem(0,0)->imdb,"tt001"));CHECK(parallelMaxActive<=3);puts("PASS shrinking collection clamps focus and rejects removed row target");return 0;
 }
 CHECK(0);return 1;
}

#ifndef NV_COLLECTION_PARALLEL_BASELINE
#include "public-pagination-cases.inc"
#else
static int runPublicManualCase(const char *test){(void)test;assert(0);return 1;}
#endif
