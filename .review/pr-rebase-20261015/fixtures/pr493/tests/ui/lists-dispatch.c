#include "common.h"
static int captureCreate(pthread_t*,const pthread_attr_t*,void*(*)(void*),void*);
#define pthread_create captureCreate
#define pthread_detach(t) ((void)(t),0)
#include "listas.c"
#undef pthread_create
#undef pthread_detach
static pthread_t dispatched;
static int captureCreate(pthread_t*t,const pthread_attr_t*a,void*(*fn)(void*),void*p){int r=pthread_create(t,a,fn,p);if(!r)dispatched=*t;return r;}
static char pinned[128];static int pinCount;
int col_extra_definir(const ColFolder*f,int n){pinCount=n;snprintf(pinned,sizeof pinned,"%s",n?f[0].title:"");return n;}
const char*nuvem_trakt_cliente(void){return "fixture-client";}
const char*nuvem_simkl_cliente(void){return "fixture-client";}
const char*nuvem_simkl_app(void){return "fixture";}
const char*simklauth_token(void){static char t[80];snprintf(t,sizeof t,"profile-%d",perfis_ativo());return t;}
void nuvem_url_escapar(const char*s,char*d,unsigned n){snprintf(d,n,"%s",s);}
int trakt_enfeitar_lote_sessao(CatItem*v,int n,uint64_t e){(void)v;assert(perfis_sessao_atual(e));return n;}
int trakt_cabecalhos(const char**cab,char*aut,size_t na,char*key,size_t nk){snprintf(aut,na,"Authorization: Bearer profile-%d",perfis_ativo());snprintf(key,nk,"trakt-api-key: fixture");cab[0]=aut;cab[1]="trakt-api-version: 2";cab[2]=key;cab[3]=NULL;return 1;}
void desc_vertudo_fonte(const ColSource*s){(void)s;assert(0);}
void desc_vertudo_filtro(const char*a,const char*b,const char*c,const char*d){(void)a;(void)b;(void)c;(void)d;assert(0);}
int col_n(void){return 0;}
const ColFolder*col_folder(int i){(void)i;return NULL;}
const char*addons_base_por_id(const char*i){(void)i;return "";}
static pthread_mutex_t netLock=PTHREAD_MUTEX_INITIALIZER;static pthread_cond_t netCond=PTHREAD_COND_INITIALIZER;
static int waiting,releaseOld,block=1,expectedProfile=1;
char*rede_baixar_com(const char*u,int secs,const char*const*headers){(void)secs;
 char expected[80];snprintf(expected,sizeof expected,"profile-%d",expectedProfile);assert(strstr(headers[0],expected));
 pthread_mutex_lock(&netLock);if(block){waiting=1;pthread_cond_broadcast(&netCond);while(!releaseOld)pthread_cond_wait(&netCond,&netLock);}pthread_mutex_unlock(&netLock);
 if(strstr(u,"api.simkl"))return strdup("{\"movies\":[{\"movie\":{\"title\":\"Fixture movie\",\"ids\":{\"imdb\":\"tt888\"}}}]}");
 return strdup("[{\"name\":\"Fixture private list\",\"ids\":{\"trakt\":777}}]");}
static void awaitHttp(void){pthread_mutex_lock(&netLock);while(!waiting)pthread_cond_wait(&netCond,&netLock);pthread_mutex_unlock(&netLock);}
static void releaseHttp(void){pthread_mutex_lock(&netLock);releaseOld=1;pthread_cond_broadcast(&netCond);pthread_mutex_unlock(&netLock);pthread_join(dispatched,NULL);}
int main(void){setup();alarm(15);
 dados_gravar("listas-p1.txt","trakt\t11\tMOVIE\t1\tA pinned list\tAlice\n");dados_gravar("listas-p2.txt","trakt\t22\tMOVIE\t1\tB pinned list\tBob\n");
 lst_abrir_sessao();assert(pinCount==1&&!strcmp(pinned,"A pinned list"));disparar(PED_TRAKT_MINHAS);awaitHttp();
 selectProfile(2);lst_abrir_sessao();assert(pinCount==1&&!strcmp(pinned,"B pinned list"));fioVivo=1;releaseHttp();assert(!nTabela&&fioVivo);
 pthread_mutex_lock(&netLock);block=0;expectedProfile=2;pthread_mutex_unlock(&netLock);disparar(PED_TRAKT_MINHAS);pthread_join(dispatched,NULL);assert(nTabela==1&&!fioVivo);
 selectProfile(1);lst_abrir_sessao();pthread_mutex_lock(&netLock);block=1;waiting=releaseOld=0;expectedProfile=1;pthread_mutex_unlock(&netLock);
 LstLista l={.fonte=LST_SIMKL};snprintf(l.estado,sizeof l.estado,"watching");snprintf(l.midia,sizeof l.midia,"MOVIE");lst_abrir(&l,"MOVIE");awaitHttp();
 selectProfile(2);lst_abrir_sessao();selectProfile(1);lst_abrir_sessao();itFioVivo=1;releaseHttp();assert(!nItens&&itFioVivo);
 pthread_mutex_lock(&netLock);block=0;pthread_mutex_unlock(&netLock);lst_abrir(&l,"MOVIE");pthread_join(dispatched,NULL);assert(nItens==1&&!itFioVivo&&!strcmp(itens[0].imdb,"tt888"));
 assert(!deleted);puts("PASS actual list dispatch: captured A credentials, stale Trakt A→B and Simkl A→B→A rejected; current Trakt/Simkl accepted; pins/files retained");}
