/* New upstream continuation contract alongside the existing parallel cases. */
static void manualWaitStarted(void){
 unsigned end=parallelMs()+3000;
 while(!manualCalls){tick();CHECK((int)(end-parallelMs())>0);}
 CHECK(manualPending&&manualInputPage==7&&manualSource==0);
 CHECK(vitEmVoo<=3);
}
static void manualPrepare(void){
 fixtureManualEnabled=fixtureWatchedPrefix=hideWatched=1;
 for(int i=0;i<itemCount;i++)snprintf(items[i].tipo,sizeof items[i].tipo,"movie");
 parallelReleaseOne(0);parallelPumpRows(1);
 CHECK(vitLin[0].brutoN==6&&vitLin[0].n==0&&vitLin[0].mais&&vitLin[0].pagina==7);
 CHECK(temMaisVistos());
 for(int i=0;i<6;i++){char id[32];snprintf(id,sizeof id,"tt%03d",i+1);CHECK(!strcmp(vitBuf[i].imdb,id));}
 /* Later initial rows can finish while this row offers manual continuation. */
 parallelReleaseOne(1);parallelPumpRows(3);CHECK(vitLin[1].n==6);
 vitFR=0;vitFC=0;vitPedirMais();tick();
 /* If all slots are occupied, freeing one initial request dispatches More. */
 parallelReleaseOne(3);manualWaitStarted();
 parallelReleaseOne(2);parallelPumpRows(4);
 CHECK(manualPending&&vitLin[2].n==6&&!strcmp(vitItem(2,0)->imdb,"tt201"));
}
static int runPublicManualCase(const char *test){
 manualPrepare();
 if(!strcmp(test,"parallel-manual-prefix")){
  vitFR=1;vitFC=2;vitMexeu=1;vitScroll=77;vitSx[1]=99;vitToqueY.livre=1;vitToqueX[1].livre=1;vitLembrarFoco();
  manualComplete(8,0);
  for(int i=0;i<8;i++)tick();vitProjetar();
  CHECK(vitLin[0].pagina==8&&vitLin[0].mais&&vitLin[0].brutoN==6&&vitLin[0].extraN==6&&vitLin[0].n==6);
  CHECK(!strcmp(vitItem(0,0)->imdb,"tt801"));CHECK(!strcmp(vitBuf[0].imdb,"tt001"));
  fprintf(stderr,"MANUAL_PREFIX_FOCUS r=%d c=%d Y=%g X=%g freeY=%d freeX=%d\n",vitFR,vitFC,vitScroll,vitSx[1],vitToqueY.livre,vitToqueX[1].livre);CHECK(vitFR==1&&vitFC==2&&fabsf(vitScroll-77)<.01f&&fabsf(vitSx[1]-99)<.01f);
  hideWatched=0;vitProjetar();CHECK(vitLin[0].n==6&&!strcmp(vitItem(0,0)->imdb,"tt001"));
  hideWatched=1;vitProjetar();CHECK(vitLin[0].n==6&&!strcmp(vitItem(0,0)->imdb,"tt801"));
  CHECK(manualCalls==1);parallelFinish(63);
  puts("PASS public watched empty More resumes exact cursor and preserves prefix siblings focus scroll");return 0;
 }
 if(!strcmp(test,"parallel-manual-source-change")){
  snprintf(sources[0].catId,sizeof sources[0].catId,"source10");
  snprintf(sources[0].base,sizeof sources[0].base,"https://replacement.invalid");tick();
  parallelReleaseAll();parallelWaitSource(10);parallelPumpRows(1);
  CHECK(!strcmp(vitItem(0,0)->imdb,"tt1001"));manualComplete(8,0);
  for(int i=0;i<8;i++)tick();CHECK(!strcmp(vitItem(0,0)->imdb,"tt1001"));
  parallelFinish(63);puts("PASS public source replacement rejects old manual continuation");return 0;
 }
 if(!strcmp(test,"parallel-manual-close")){
  unsigned before=parallelMs();vertudo_fechar_seco();CHECK(parallelMs()-before<200);
  for(int r=0;r<6;r++)snprintf(sources[r].catId,sizeof sources[r].catId,"source%d",r+10);
  other.nSources=6;vertudo_colecao(&other);parallelReleaseAll();
  for(int i=0;i<8;i++)tick();CHECK(vitEmVoo<=3);manualComplete(8,0);
  parallelPumpRows(63);parallelWaitReturned();
  for(int r=0;r<6;r++){char id[32];snprintf(id,sizeof id,"tt%03d",(r+10)*100+1);CHECK(!strcmp(vitItem(r,0)->imdb,id));}
  CHECK(vitEmVoo<=3);puts("PASS public close reopen rejects old manual continuation without waiting on network");return 0;
 }
 if(!strcmp(test,"parallel-manual-layout")){
  fprintf(stderr,"LAYOUT_BEFORE choice=%d actual=%d\n",layEscolha,lay);globalView=VT_LAY_GRADE;alternarExibicao();fprintf(stderr,"LAYOUT_AFTER choice=%d actual=%d\n",layEscolha,lay);CHECK(lay!=VT_LAY_VITRINE);
  manualComplete(8,0);parallelReleaseAll();parallelWaitReturned();
  for(int i=0;i<8;i++)tick();CHECK(lay!=VT_LAY_VITRINE&&vitEmVoo<=3);
  CHECK(vitLin[0].n==0);puts("PASS public layout change cannot publish old manual continuation into Showcase");return 0;
 }
 CHECK(0);return 1;
}
