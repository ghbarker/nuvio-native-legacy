#include <stdatomic.h>
#include <errno.h>
#include "home-boundaries.inc"
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condT = PTHREAD_COND_INITIALIZER;
static int cwEntered, socialEntered, cwRelease, socialRelease, cwEmpty;
static int cwDone, socialDone;
static Uint32 tickBase;
Uint32 SDL_GetTicks(void) {
  struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
  return (Uint32)((uint64_t)ts.tv_sec*1000+ts.tv_nsec/1000000);
}
char *rede_baixar(const char *u,int t) { return original_rede_baixar(u,t); }
int trakt_continuar(CatItem *s,int max) {
  pthread_mutex_lock(&gate);cwEntered=1;pthread_cond_broadcast(&condT);
  while(!cwRelease)pthread_cond_wait(&condT,&gate);
  pthread_mutex_unlock(&gate);
  int n=cwEmpty?0:original_trakt_continuar(s,max);
  pthread_mutex_lock(&gate);cwDone=1;pthread_cond_broadcast(&condT);pthread_mutex_unlock(&gate);
  return n;
}
int trakt_social(CatItem *s,int max) {
  pthread_mutex_lock(&gate);socialEntered=1;pthread_cond_broadcast(&condT);
  while(!socialRelease)pthread_cond_wait(&condT,&gate);
  pthread_mutex_unlock(&gate);
  int n=original_trakt_social(s,max);
  pthread_mutex_lock(&gate);socialDone=1;pthread_cond_broadcast(&condT);pthread_mutex_unlock(&gate);
  return n;
}
/* Faithful catalogo.c CW splice boundary. Production discovery is included
   unchanged. Real catalogo ownership tests remain separate mandatory gates. */
void cat_trocar_continuar(const CatItem *items,int count) {
  pthread_mutex_lock(&pubTravaT);
  int row=-1,start=0,old=0;
  for(int k=0;k<nPubFils;k++)if(!strcmp(pubFils[k].chave,"continue_watching")){row=k;start=pubFils[k].ini;old=pubFils[k].n;break;}
  assert(count>=0&&count+nPub-old<4096);
  memmove(pub+start+count,pub+start+old,(size_t)(nPub-start-old)*sizeof(CatItem));
  if(count)memcpy(pub+start,items,(size_t)count*sizeof(CatItem));
  nPub+=count-old;
  for(int k=0;k<nPubFils;k++){
    if(k==row)pubFils[k].n=count;
    else if(pubFils[k].ini>=start+old)pubFils[k].ini+=count-old;
  }
  if(row>=0&&!count){memmove(pubFils+row,pubFils+row+1,(size_t)(nPubFils-row-1)*sizeof(CatFileira));nPubFils--;}
  if(row<0&&count){assert(nPubFils<CAT_FIL_MAX);memmove(pubFils+1,pubFils,(size_t)nPubFils*sizeof(CatFileira));memset(pubFils,0,sizeof(CatFileira));snprintf(pubFils[0].chave,sizeof pubFils[0].chave,"continue_watching");pubFils[0].n=count;nPubFils++;}
  pthread_mutex_unlock(&pubTravaT);
}
static int catalogRows(void){int n=0;pthread_mutex_lock(&pubTravaT);for(int k=0;k<nPubFils;k++)if(pubFils[k].catId[0]&&pubFils[k].n)n++;pthread_mutex_unlock(&pubTravaT);return n;}
static int cwRows(void){CatItem c[4];return cat_copiar_fileira("continue_watching",c,4,NULL);}
static void waitEntered(void){struct timespec until;clock_gettime(CLOCK_REALTIME,&until);until.tv_sec+=3;pthread_mutex_lock(&gate);while(!cwEntered||!socialEntered)assert(pthread_cond_timedwait(&condT,&gate,&until)!=ETIMEDOUT);pthread_mutex_unlock(&gate);}
static void waitCatalog(void){Uint32 end=SDL_GetTicks()+3000;while(!catalogRows()&&SDL_GetTicks()<end)usleep(1000);assert(catalogRows()>0);}
static void release(int cw,int social){pthread_mutex_lock(&gate);cwRelease|=cw;socialRelease|=social;pthread_cond_broadcast(&condT);pthread_mutex_unlock(&gate);}
static void waitCW(int count){Uint32 end=SDL_GetTicks()+3000;while(cwRows()!=count&&SDL_GetTicks()<end)usleep(1000);assert(cwRows()==count);}
static void verifyRows(int wantCW,int wantSocial){
 CatItem c[4]; assert(cwRows()==wantCW);
 pthread_mutex_lock(&pubTravaT);
 if(wantCW){assert(!strcmp(pubFils[0].chave,"continue_watching"));assert(!strcmp(pub[pubFils[0].ini].imdb,"tt9001"));}
 int cats=0,soc=0;
 for(int r=0;r<nPubFils;r++){CatFileira *f=&pubFils[r];assert(f->ini>=0&&f->ini+f->n<=nPub);
  if(f->catId[0]){cats++;for(int k=0;k<f->n;k++){CatItem *it=&pub[f->ini+k];assert(!strncmp(it->titulo,f->catId,strlen(f->catId)));assert(!strcmp(it->poster,"p.jpg"));}}
  if(!strcmp(f->chave,"social_activity")){soc++;assert(!strcmp(pub[f->ini].imdb,"tt9101"));assert(!strcmp(pub[f->ini].socialNome,"Kevin"));assert(r==(wantCW?1:0));}}
 assert(cats==2);assert(soc==wantSocial);pthread_mutex_unlock(&pubTravaT);
}
int main(int argc,char **argv){assert(argc==3);int fixed=atoi(argv[1]);const char *mode=argv[2];
 bumpNoPrimeiroCatalogo=0;tickBase=SDL_GetTicks();montagemSessao=nv_perfil_sessao_ler();
 if(!strcmp(mode,"cached")||!strcmp(mode,"clear")){CatItem old={0};CatFileira f={0};snprintf(old.imdb,sizeof old.imdb,"tt-cached");snprintf(f.chave,sizeof f.chave,"continue_watching");f.n=1;cat_definir_tudo(&old,1,&f,1);cwEmpty=!strcmp(mode,"clear");}
 pthread_t worker;buscando=1;assert(!pthread_create(&worker,NULL,montar,NULL));waitEntered();
 if(fixed){waitCatalog();assert(!cwRelease&&!socialRelease);printf("CATALOG_AVAILABLE_WHILE_CW_AND_SOCIAL_BLOCKED rows=%d requests=%d\n",catalogRows(),pedidosCatalogo);assert(pedidosCatalogo==2);}
 else{assert(pedidosCatalogo==0);assert(catalogRows()==0);puts("BASELINE_REPRODUCED_CW_BLOCKS_CATALOG");}
 if(!strcmp(mode,"stale")&&fixed){
  perfis_invalidar_sessao();descobertaInicializada=1;aberturaFontesProntas=0;montagemGeracao++;aberturaPendente=1;aberturaSessao=nv_perfil_sessao_ler();
  assert(!pthread_join(worker,NULL));
  CatItem marker={0};CatFileira row={0};snprintf(marker.imdb,sizeof marker.imdb,"tt-new-profile");snprintf(row.chave,sizeof row.chave,"new-profile");row.n=1;cat_definir_tudo(&marker,1,&row,1);
  release(1,1);Uint32 end=SDL_GetTicks()+3000;while((!cwDone||!socialDone||cwVivo)&&SDL_GetTicks()<end)usleep(1000);
  assert(cwDone&&socialDone&&!cwVivo);assert(cat_n()==1&&!strcmp(cat_item(0)->imdb,"tt-new-profile"));puts("STALE_PROFILE_CW_AND_SOCIAL_REJECTED");return 0;
 }
 release(1,0);
 if(fixed){waitCW(cwEmpty?0:2);verifyRows(cwEmpty?0:2,0);}
 release(0,1);assert(!pthread_join(worker,NULL));
 if(fixed){waitCW(cwEmpty?0:2);verifyRows(cwEmpty?0:2,1);desc_remontar_fileiras();verifyRows(cwEmpty?0:2,1);}else verifyRows(2,1);
 Uint32 end=SDL_GetTicks()+3000;while(cwVivo&&SDL_GetTicks()<end)usleep(1000);assert(!cwVivo);
 printf("LATCH PASS mode=%s fixed=%d elapsed=%u\n",mode,fixed,(unsigned)(SDL_GetTicks()-tickBase));return 0;
}
void homeestado_contexto(HomeContexto *c){original_homeestado_contexto(c);}
int homeestado_mudancas(const HomeContexto *a,const HomeContexto *b){return original_homeestado_mudancas(a,b);}
int ajustes_ocultar_filmes_vistos(void){return 0;}
int cat_visto(const CatItem *it){(void)it;return 0;}
int cat_copiar_por_id(const char *id,const char *tipo,CatItem *out){(void)id;(void)tipo;(void)out;return 0;}
int cat_gravar_cache_se_sessao(const char *dir,const char *user,int profile,uint64_t session){(void)dir;(void)user;(void)profile;return nv_perfil_sessao_atual(session);}
int addons_identidade_recebida(const char *base,unsigned version,uint64_t session,const char *body){(void)base;(void)version;(void)body;return nv_perfil_sessao_atual(session);}
int addons_manifesto_recebido(const char *base,unsigned version,uint64_t session,const char *body){(void)base;(void)version;(void)body;return nv_perfil_sessao_atual(session);}

/* Verbatim production helper; only storage/mutex/notification bindings differ. */
static int fileiraContinuar(void){for(int r=0;r<nPubFils;r++)if(!strcmp(pubFils[r].chave,"continue_watching"))return r;return -1;}
#define fils pubFils
#define nFils nPubFils
#define pubTrava pubTravaT
#define catTravar() (pthread_mutex_lock(&pubTravaT), (n = nPub))
#define revisaoSobe() ((void)0)
#define mudou() ((void)0)
#define cat_republicar_fileiras actual_catalog_republish
#include "catalog-republish-actual.inc"
#undef cat_republicar_fileiras
#undef n
#undef fils
#undef nFils
#undef pubTrava
#undef catTravar
#undef revisaoSobe
#undef mudou

void cat_republicar_fileiras(const CatFileira *f,int count){n=nPub;actual_catalog_republish(f,count);}
