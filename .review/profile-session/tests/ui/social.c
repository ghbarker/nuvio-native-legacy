#define NV_REC_URL "https://fixture.invalid"
#include "common.h"
#include "recomenda.c"
static pthread_mutex_t netLock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond=PTHREAD_COND_INITIALIZER;
static int waiting,releaseOld,blockRequest=1;
int ajustes_social(void){return 1;}
const char*i18n(const char*x){return x;}
int sessao_identidade_local_copiar(char*u,size_t nu,char*t,size_t nt){snprintf(u,nu,"fixture-account-a");snprintf(t,nt,"fixture-token");return 1;}
char *rede_baixar_st(const char*u,int t,const char*const*c,int*st){(void)u;(void)t;(void)c;
 pthread_mutex_lock(&netLock);int block=blockRequest;
 if(block){waiting=1;pthread_cond_broadcast(&netCond);while(!releaseOld)pthread_cond_wait(&netCond,&netLock);}
 pthread_mutex_unlock(&netLock);*st=200;
 return strdup(block?"{\"contatos\":[{\"id\":\"profile-a\",\"nome\":\"A private friend\"}]}":"{\"contatos\":[{\"id\":\"profile-b\",\"nome\":\"B private friend\"}]}");}
static void*oldRequest(void*p){recSessaoDefinir(*(uint64_t*)p);const char*headers[]={"old-A-auth",NULL};lerContatos(headers);assert(!strcmp(REC_ARQ,"recomendacoes.txt"));return NULL;}
int main(void){setup();alarm(10);
 dados_gravar("recomendacoes-aparecer.txt","2\n");dados_gravar("recomendacoes-aparecer-p2.txt","1\n");
 dados_gravar("recomendacoes-perfil.txt","1\t1\t1\t2\t0\t1\tAlice\tA private bio\n");
 recomenda_abrir_sessao();assert(aparecer==REC_APARECER_SIM&&perfil.publicado&&!strcmp(perfil.apelido,"alice"));
 recresp_responder(99,1,"a saved reply");RecResp rr;assert(recresp_ler(99,&rr));
 uint64_t old=perfis_sessao_geracao();pthread_t thread;assert(!pthread_create(&thread,NULL,oldRequest,&old));
 pthread_mutex_lock(&netLock);while(!waiting)pthread_cond_wait(&netCond,&netLock);pthread_mutex_unlock(&netLock);
 selectProfile(2);recomenda_abrir_sessao();assert(aparecer==REC_APARECER_NAO&&!perfil.publicado&&!nContatos&&!nFeed);
 assert(!recresp_ler(99,&rr));
 pthread_mutex_lock(&netLock);releaseOld=1;blockRequest=0;pthread_cond_broadcast(&netCond);pthread_mutex_unlock(&netLock);pthread_join(thread,NULL);
 assert(!nContatos);recSessaoDefinir(perfis_sessao_geracao());const char*headers[]={"new-B-auth",NULL};lerContatos(headers);assert(nContatos==1&&!strcmp(contatos[0].id,"profile-b"));
 recSessaoDefinir(0);selectProfile(1);recomenda_abrir_sessao();assert(aparecer==REC_APARECER_SIM&&perfil.publicado&&!strcmp(perfil.apelido,"alice")&&recresp_ler(99,&rr));
 // A later confirmation of A is still a different session from the old A request.
 pthread_mutex_lock(&netLock);waiting=releaseOld=0;blockRequest=1;pthread_mutex_unlock(&netLock);
 old=perfis_sessao_geracao();pthread_create(&thread,NULL,oldRequest,&old);
 pthread_mutex_lock(&netLock);while(!waiting)pthread_cond_wait(&netCond,&netLock);pthread_mutex_unlock(&netLock);
 selectProfile(2);recomenda_abrir_sessao();selectProfile(1);recomenda_abrir_sessao();
 pthread_mutex_lock(&netLock);releaseOld=1;blockRequest=0;pthread_cond_broadcast(&netCond);pthread_mutex_unlock(&netLock);pthread_join(thread,NULL);
 assert(!nContatos&&recresp_ler(99,&rr));
 assert(!deleted);puts("PASS Social: old A HTTP completion rejected, current B accepted, A consent/profile/replies restored without deletes");}
