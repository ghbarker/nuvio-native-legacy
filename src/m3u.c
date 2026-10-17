#include "m3u.h"
#include "guiacorte.h"
#include "dados.h"
#include "perfis.h"
#include "perfisessao.h"
#include "rede.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cargaTrava = PTHREAD_MUTEX_INITIALIZER;
static char lista[1100], epg[1100], epgDetectada[1100];
static M3uCanal *canais;
static int nCanais, total, perfil = -1, lido, renovarCache;
static uint64_t sessaoLida, revisao;
static time_t carregada;
static volatile int cancelada;
static uint64_t hash(const char *s) {
  uint64_t h=UINT64_C(14695981039346656037);
  for(;*s;s++){h^=(unsigned char)*s;h*=UINT64_C(1099511628211);}return h;
}
static void nomeConfig(char *s,unsigned n,int p) { snprintf(s,n,"m3u-p%d.txt",p); }
static void limparEpg(int p) {
  static const char *ext[]={"bin","ts","id"};char nome[96];
  for(int i=0;i<3;i++){snprintf(nome,sizeof nome,"epg-extra-m3u-p%d.%s",p,ext[i]);dados_apagar(nome);}
}
static void invalidar(void) {
  cancelada=1;
  free(canais); canais=NULL; nCanais=total=0; carregada=0; epgDetectada[0]=0; renovarCache=0; revisao++;
}
static void carregar(void) {
  int p=perfis_ativo(); uint64_t sessao=nv_perfil_sessao_ler();
  if(lido&&perfil==p&&sessaoLida==sessao)return;
  lido=1;perfil=p;sessaoLida=sessao;invalidar();lista[0]=epg[0]=0;
  char nome[64];nomeConfig(nome,sizeof nome,p);
  char *body=dados_ler(nome);if(!body)return;
  for(char *line=body;*line;){char *next=strchr(line,'\n');if(next)*next++=0;else next=line+strlen(line);
    if(!strncmp(line,"lista\t",6)&&strlen(line+6)<sizeof lista)snprintf(lista,sizeof lista,"%s",line+6);
    if(!strncmp(line,"epg\t",4)&&strlen(line+4)<sizeof epg)snprintf(epg,sizeof epg,"%s",line+4);
    line=next;
  }free(body);
}
static int urlValida(const char *s) {
  if(!s)return 0;
  if(!*s)return 1;
  if(strlen(s)>=sizeof lista||strpbrk(s,"\r\n\t "))return 0;
  return !strncmp(s,"http://",7)||!strncmp(s,"https://",8);
}
static int definir(const char *url,int grade) {
  if(!urlValida(url))return 0;
  pthread_mutex_lock(&trava);carregar();
  char body[2300],nome[64];nomeConfig(nome,sizeof nome,perfil);
  snprintf(body,sizeof body,"lista\t%s\nepg\t%s\n",grade?lista:url,grade?url:epg);
  int atual=nv_perfil_sessao_travar(sessaoLida);
  int ok=atual && dados_gravar(nome,body);
  if(ok){
    limparEpg(perfil);
    if (!grade && lista[0] && strcmp(lista,url)) {
      char cache[96];snprintf(cache,sizeof cache,"m3u-cache-p%d-%016" PRIx64 ".txt",perfil,hash(lista));
      dados_apagar(cache);
    }
    snprintf(grade?epg:lista,sizeof lista,"%s",url);invalidar();
  }
  if(atual)nv_perfil_sessao_destravar();
  pthread_mutex_unlock(&trava);return ok;
}
int m3u_definir_lista(const char *s){return definir(s,0);}
int m3u_definir_epg(const char *s){return definir(s,1);}
void m3u_esquecer(void) {
  pthread_mutex_lock(&trava);carregar();
  if (nv_perfil_sessao_travar(sessaoLida)) {
    char nome[96];limparEpg(perfil);
    snprintf(nome,sizeof nome,"m3u-cache-p%d-%016" PRIx64 ".txt",perfil,hash(lista));
    dados_apagar(nome);nomeConfig(nome,sizeof nome,perfil);dados_apagar(nome);
    lista[0]=epg[0]=0;invalidar();
    nv_perfil_sessao_destravar();
  }
  pthread_mutex_unlock(&trava);
}
void m3u_esquecer_todos(void) {
  pthread_mutex_lock(&trava);
  invalidar();lista[0]=epg[0]=0;lido=0;
  // Matches the account cleanup's supported local profile slots.
  for (int p=0;p<=32;p++) {
    limparEpg(p);
    char nome[96];nomeConfig(nome,sizeof nome,p);
    char *body=dados_ler(nome);
    if(body) {
      char *u=!strncmp(body,"lista\t",6)?body+6:NULL;
      if(u) { char *fim=strchr(u,'\n');if(fim)*fim=0;
        snprintf(nome,sizeof nome,"m3u-cache-p%d-%016" PRIx64 ".txt",p,hash(u));dados_apagar(nome);
      }
      free(body);
    }
    nomeConfig(nome,sizeof nome,p);dados_apagar(nome);
  }
  pthread_mutex_unlock(&trava);
}
int m3u_configurado(void){pthread_mutex_lock(&trava);carregar();int r=lista[0]!=0;pthread_mutex_unlock(&trava);return r;}
const char *m3u_lista(void){static char s[1100];pthread_mutex_lock(&trava);carregar();snprintf(s,sizeof s,"%s",lista);pthread_mutex_unlock(&trava);return s;}
const char *m3u_epg(void){static char s[1100];pthread_mutex_lock(&trava);carregar();snprintf(s,sizeof s,"%s",epg);pthread_mutex_unlock(&trava);return s;}
int m3u_url_epg(char *s,unsigned n){pthread_mutex_lock(&trava);carregar();const char *u=epg[0]?epg:epgDetectada;int k=snprintf(s,n,"%s",u);pthread_mutex_unlock(&trava);return k>0&&(unsigned)k<n;}
int m3u_e_id(const char *id){if(!id||strncmp(id,"nvm3u:",6)||strlen(id)!=22)return 0;for(const char *p=id+6;*p;p++)if(!isxdigit((unsigned char)*p))return 0;return 1;}

// Fetch/parse on the guide/resolver worker, never in a frame or Settings getter.
static int atualizar(uint64_t sessao,int renovar) {
  char url[1100],nome[96],descoberta[1100];uint64_t rev;int p;
  pthread_mutex_lock(&cargaTrava);
  pthread_mutex_lock(&trava);carregar();
  if(!nv_perfil_sessao_atual(sessao)||!lista[0]){pthread_mutex_unlock(&trava);pthread_mutex_unlock(&cargaTrava);return 0;}
  renovar = renovar || renovarCache;
  if(!renovar&&canais&&time(NULL)-carregada<3600){int n=nCanais;pthread_mutex_unlock(&trava);pthread_mutex_unlock(&cargaTrava);return n;}
  snprintf(url,sizeof url,"%s",lista);p=perfil;rev=revisao;cancelada=0;
  snprintf(nome,sizeof nome,"m3u-cache-p%d-%016" PRIx64 ".txt",p,hash(url));
  pthread_mutex_unlock(&trava);
  char *body=NULL;
  if(!renovar){
    char caminho[1024];struct stat st;
    if(dados_caminho(caminho,sizeof caminho,nome)&&!stat(caminho,&st)&&time(NULL)-st.st_mtime<3600)body=dados_ler(nome);
  }
  int daRede=0;
  if(!body){
    RedeControle ctl={0};RedeMedida med={0};long bytes=0;
    ctl.max_bytes=16*1024*1024;ctl.cancelado=&cancelada;
    body=rede_baixar_bin_medido_controle(url,45,NULL,&ctl,&bytes,&med);
    if(!body||med.status<200||med.status>=300||med.limitado||bytes<=0){free(body);body=NULL;}
    else if(memchr(body,0,(size_t)bytes)){free(body);body=NULL;}
    else {char *b=realloc(body,(size_t)bytes+1);if(b){body=b;body[bytes]=0;daRede=1;}else{free(body);body=NULL;}}
  }
  if(!body)body=dados_ler(nome); // Network failure keeps a previous playlist usable.
  M3uCanal *novo=body?calloc(GUIACORTE_MAX_CANAL,sizeof *novo):NULL;
  char *cache=body&&daRede?strdup(body):NULL;
  int totalNovo=0,n=novo?m3u_processar(body,url,novo,GUIACORTE_MAX_CANAL,descoberta,sizeof descoberta,&totalNovo):0;
  free(body);
  pthread_mutex_lock(&trava);
  if(n>0&&rev==revisao&&p==perfil&&nv_perfil_sessao_travar(sessao)){
    free(canais);canais=novo;novo=NULL;nCanais=n;total=totalNovo;carregada=daRede?time(NULL):(renovar?0:time(NULL));renovarCache=0;
    snprintf(epgDetectada,sizeof epgDetectada,"%s",descoberta);
    if(cache)dados_gravar_leve(nome,cache);
    nv_perfil_sessao_destravar();
    printf("[m3u] %d channel(s) loaded, %d declared\n",n,totalNovo);fflush(stdout);
  } else if(!nv_perfil_sessao_atual(sessao)||rev!=revisao)n=0;
  else n=nCanais; // A failed refresh keeps the last working list.
  pthread_mutex_unlock(&trava);free(novo);free(cache);pthread_mutex_unlock(&cargaTrava);return n;
}
int m3u_canais_sessao(M3uCanal *out,int max,uint64_t sessao) {
  if(!out||max<1||!atualizar(sessao,0))return 0;
  pthread_mutex_lock(&trava);int n=nv_perfil_sessao_atual(sessao)?nCanais:0;if(n>max)n=max;
  if(n)memcpy(out,canais,(size_t)n*sizeof *out);
  pthread_mutex_unlock(&trava);return n;
}
int m3u_ultimo_total(void){pthread_mutex_lock(&trava);int n=total;pthread_mutex_unlock(&trava);return n;}
int m3u_resolver_sessao(const char *id,char *out,unsigned n,uint64_t sessao,int renovar) {
  if(n)out[0]=0;
  if(!m3u_e_id(id)||!nv_perfil_sessao_atual(sessao))return 0;
  int pronto=0;
  pthread_mutex_lock(&trava);carregar();
  if(!renovar && nv_perfil_sessao_atual(sessao))for(int i=0;i<nCanais;i++)if(!strcmp(id,canais[i].id)) {
    int k=snprintf(out,n,"%s",canais[i].url);pronto=k>0&&(unsigned)k<n;break;
  }
  pthread_mutex_unlock(&trava);
  if(pronto)return 1;
  if(!atualizar(sessao,renovar))return 0;
  pthread_mutex_lock(&trava);int ok=0;
  if(nv_perfil_sessao_atual(sessao))for(int i=0;i<nCanais;i++)if(!strcmp(id,canais[i].id)){
    int k=snprintf(out,n,"%s",canais[i].url);ok=k>0&&(unsigned)k<n;break;
  }pthread_mutex_unlock(&trava);return ok;
}
int m3u_cabecalhos(const char *id,char *out,unsigned n) {
  if(n)out[0]=0;
  pthread_mutex_lock(&trava);carregar();int ok=0;
  for(int i=0;i<nCanais;i++)if(!strcmp(id,canais[i].id)){int k=snprintf(out,n,"%s",canais[i].cabecalhos);ok=k>=0&&(unsigned)k<n;break;}
  pthread_mutex_unlock(&trava);return ok;
}
void m3u_recarregar(void){pthread_mutex_lock(&trava);carregar();carregada=0;renovarCache=1;pthread_mutex_unlock(&trava);}
