/* Public 2.0.5 singleton pagination transport: UI remains actual vertudo.c.
 * Explicit main-thread latch models a pending page; no UI scheduler is mocked. */
#define PUBLIC_MANUAL_PAGINATION 1
static int fixtureManualEnabled,fixtureWatchedPrefix;
static int manualCalls,manualPending,manualCount,manualPage,manualEnd,manualInputPage,manualSource;
static int manualActive(void){return fixtureManualEnabled&&manualCalls>0;}
static void manualBegin(int row,int pagina){
 assert(pagina>=0);manualSource=row;sourceIndex=row;manualInputPage=pagina;
 manualCalls++;manualPending=1;manualCount=0;manualEnd=0;manualPage=pagina;
}
static int manualItem(int i,CatItem *out){
 if(manualPending||i<0||i>=manualCount)return 0;
 *out=items[i%itemCount];snprintf(out->imdb,sizeof out->imdb,"tt%03d",801+i);
 snprintf(out->tipo,sizeof out->tipo,"movie");
 snprintf(out->poster,sizeof out->poster,"https://art.invalid/manual/title%d.jpg",i+1);
 return 1;
}
static void manualComplete(int page,int end){manualCount=6;manualPage=page;manualEnd=end;manualPending=0;}
