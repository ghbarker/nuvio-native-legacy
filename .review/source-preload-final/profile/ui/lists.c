#include "common.h"
#include "listas.c"
static char pinned[128];static int pinCount;
int col_extra_definir(const ColFolder*f,int n){pinCount=n;snprintf(pinned,sizeof pinned,"%s",n?f[0].title:"");return n;}
const char*nuvem_trakt_cliente(void){return "fixture-client";}
int trakt_cabecalhos(const char**cab,char*aut,size_t na,char*key,size_t nk){snprintf(aut,na,"Authorization: Bearer profile-%d",perfis_ativo());snprintf(key,nk,"trakt-api-key: fixture");cab[0]=aut;cab[1]="trakt-api-version: 2";cab[2]=key;cab[3]=NULL;return 1;}
static pthread_mutex_t netLock=PTHREAD_MUTEX_INITIALIZER;static pthread_cond_t netCond=PTHREAD_COND_INITIALIZER;
static int waiting,releaseOld,done;
char*rede_baixar_com(const char*u,int secs,const char*const*headers){(void)u;(void)secs;assert(strstr(headers[0],"profile-1"));pthread_mutex_lock(&netLock);waiting=1;pthread_cond_broadcast(&netCond);while(!releaseOld)pthread_cond_wait(&netCond,&netLock);pthread_mutex_unlock(&netLock);return strdup("[{\"name\":\"A private list\",\"ids\":{\"trakt\":777}}]");}
static void*finishRequest(void*p){fioListas(p);pthread_mutex_lock(&netLock);done=1;pthread_cond_broadcast(&netCond);pthread_mutex_unlock(&netLock);return NULL;}
int main(void){setup();alarm(10);
 dados_gravar("listas-p1.txt","trakt\t11\tMOVIE\t1\tA pinned list\tAlice\n");dados_gravar("listas-p2.txt","trakt\t22\tMOVIE\t1\tB pinned list\tBob\n");
 lst_abrir_sessao();assert(pinCount==1&&!strcmp(pinned,"A pinned list"));
 ListaTarefa*t=calloc(1,sizeof*t);t->sessao=perfis_sessao_geracao();t->geracao=++geracao;t->qual=PED_TRAKT_MINHAS;snprintf(t->aut,sizeof t->aut,"Authorization: Bearer profile-1");snprintf(t->chave,sizeof t->chave,"key");fioVivo=1;
 pthread_t thread;pthread_create(&thread,NULL,finishRequest,t);pthread_mutex_lock(&netLock);while(!waiting)pthread_cond_wait(&netCond,&netLock);pthread_mutex_unlock(&netLock);
 selectProfile(2);lst_abrir_sessao();assert(pinCount==1&&!strcmp(pinned,"B pinned list"));
 // The current profile's loading state belongs to its own request.
 fioVivo=1;pthread_mutex_lock(&netLock);releaseOld=1;pthread_cond_broadcast(&netCond);pthread_mutex_unlock(&netLock);pthread_join(thread,NULL);
 assert(!nTabela&&fioVivo==1);selectProfile(1);lst_abrir_sessao();assert(pinCount==1&&!strcmp(pinned,"A pinned list"));assert(!deleted);
 puts("PASS Lists: pinned folders reload A/B/A, late A result rejected without clearing B loading, files retained");}
