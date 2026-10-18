#include "drivelocal.h"
#include "js.h"
#include "jsw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DRIVE_API "https://www.googleapis.com/drive/v3/files"
#define DRIVE_UPLOAD "https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart&fields=id"
#define DRIVE_MIME "application/vnd.nuvio.local-settings"
static int identificador(const char *s,size_t max) {
  if(!s || !*s)return 0;
  size_t n=0;
  for(;s[n];n++) {
    unsigned char c=(unsigned char)s[n];
    if(n>=max || !((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-'))return 0;
  }
  return 1;
}
static int valido(const DriveLocal *c) {
  if(!c || !c->token || !*c->token || !identificador(c->dono,80) || c->perfil<1)return 0;
  for(size_t i=0;c->token[i];i++)if(i>=4096 || (unsigned char)c->token[i]<=32 || (unsigned char)c->token[i]>=127)return 0;
  return 1;
}
static char *escapar(const char *s,size_t limite) {
  if(!s || strnlen(s,limite+1)>limite)return NULL;
  size_t n=strlen(s);char *out=malloc(n*3+1);if(!out)return NULL;
  static const char hex[]="0123456789ABCDEF";size_t k=0;
  for(size_t i=0;i<n;i++) {
    unsigned char c=(unsigned char)s[i];
    if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='-' || c=='_' || c=='.' || c=='~')out[k++]=(char)c;
    else {out[k++]='%';out[k++]=hex[c>>4];out[k++]=hex[c&15];}
  }
  out[k]=0;return out;
}
static int executar(const DriveLocal *c,const char *metodo,const char *url,const char *tipo,
                    const char *corpo,size_t n,size_t max,char **saida,size_t *tamanho) {
  *saida=NULL;*tamanho=0;
  if(c->job && rede_job_estado(c->job)!=REDE_OK)return DRIVE_LOCAL_CANCELADO;
  char autorizacao[4120];snprintf(autorizacao,sizeof autorizacao,"Authorization: Bearer %s",c->token);
  const char *cab[]={autorizacao,tipo,NULL};
  RedePedido p={.metodo=metodo,.url=url,.cabecalhos=cab,.corpo=corpo,.n_corpo=n,
    .prazo_ms=30000,.max_bytes=max,.max_cabecalhos=16384,.seguir=0,.job=c->job};
  RedeResposta r={0};int recebeu=c->pedir?c->pedir(c->ctx,&p,&r):rede_pedir(&p,&r);
  int resultado=DRIVE_LOCAL_REDE;
  if(r.erro==REDE_CANCELADO || r.erro==REDE_GERACAO || (c->job && rede_job_estado(c->job)!=REDE_OK))resultado=DRIVE_LOCAL_CANCELADO;
  else if(recebeu && r.erro==REDE_OK) {
    if(r.status==401)resultado=DRIVE_LOCAL_AUTORIZAR;
    else if(r.status==403) {
      char motivo[80];
      resultado=DRIVE_LOCAL_NEGADO;
      if(r.corpo && r.n_corpo<=max && !memchr(r.corpo,0,r.n_corpo) &&
         js_texto(r.corpo,r.corpo+r.n_corpo,"reason",motivo,sizeof motivo) &&
         (!strcmp(motivo,"rateLimitExceeded") || !strcmp(motivo,"userRateLimitExceeded")))resultado=DRIVE_LOCAL_REPETIR;
    }
    else if(r.status==404)resultado=DRIVE_LOCAL_AUSENTE;
    else if(r.status==429 || r.status>=500)resultado=DRIVE_LOCAL_REPETIR;
    else if(r.status>=200 && r.status<300) {
      resultado=DRIVE_LOCAL_RESPOSTA;
      if(r.corpo && r.n_corpo && r.n_corpo<=max && !memchr(r.corpo,0,r.n_corpo)) {
        char *out=malloc(r.n_corpo+1);
        if(out){memcpy(out,r.corpo,r.n_corpo);out[r.n_corpo]=0;*saida=out;*tamanho=r.n_corpo;resultado=DRIVE_LOCAL_OK;}
      }
    } else resultado=DRIVE_LOCAL_RESPOSTA;
  }
  // The network layer never logs caller headers. No token/response logging here.
  memset(autorizacao,0,sizeof autorizacao);rede_resposta_limpar(&r);return resultado;
}
static const char *pular(const char *p){while(*p && (unsigned char)*p<=32)p++;return p;}
static int objeto(const char *s) {
  s=pular(s);if(*s!='{')return 0;
  const char *fim=js_fim(s);return fim>s && fim[-1]=='}' && !*pular(fim);
}
static int arquivo(const DriveLocal *c,const char *s,const char *fim,DriveBackup *a) {
  char propriedades[512],dono[81],perfil[16],atual[16],versao[8],mime[80],tamanho[24],data[64];
  if(!js_bruto(s,fim,"appProperties",propriedades,sizeof propriedades) || !objeto(propriedades) ||
     !js_texto_raiz(propriedades,"nuvioBackupVersion",versao,sizeof versao) || strcmp(versao,"1") ||
     !js_texto_raiz(propriedades,"nuvioOwner",dono,sizeof dono) || strcmp(dono,c->dono) ||
     !js_texto_raiz(propriedades,"nuvioProfile",perfil,sizeof perfil))return 0;
  snprintf(atual,sizeof atual,"%d",c->perfil);if(strcmp(perfil,atual))return 0;
  memset(a,0,sizeof *a);
  if(!js_texto_raiz_em(s,fim,"id",a->id,sizeof a->id) || !identificador(a->id,sizeof a->id-1) ||
     !js_texto_raiz_em(s,fim,"name",a->nome,sizeof a->nome) ||
     !js_texto_raiz_em(s,fim,"mimeType",mime,sizeof mime) || strcmp(mime,DRIVE_MIME) ||
     !js_texto_raiz_em(s,fim,"size",tamanho,sizeof tamanho) ||
     !js_texto_raiz_em(s,fim,"createdTime",data,sizeof data))return -1;
  size_t bytes=0;
  if(!*tamanho)return -1;
  for(const char *p=tamanho;*p;p++) {
    if(*p<'0' || *p>'9' || bytes>(BACKUP_LOCAL_MAX-(unsigned)(*p-'0'))/10u)return -1;
    bytes=bytes*10u+(unsigned)(*p-'0');
  }
  a->criadoMs=js_ms_iso(data);if(!bytes || a->criadoMs<=0)return -1;
  a->bytes=bytes;return 1;
}
int drivelocal_pasta(const DriveLocal *c,int criar,char id[200]) {
  if(id)*id=0;
  if(!id || !valido(c))return DRIVE_LOCAL_INVALIDO;
  const char consulta[]="trashed=false and mimeType='application/vnd.google-apps.folder' and 'me' in owners and appProperties has { key='nuvioBackupFolder' and value='1' }";
  char *q=escapar(consulta,sizeof consulta-1);if(!q)return DRIVE_LOCAL_INVALIDO;
  char url[1024];int tamanho=snprintf(url,sizeof url,DRIVE_API
    "?spaces=drive&pageSize=1&orderBy=createdTime&q=%s&fields=files(id,mimeType,appProperties)",q);free(q);
  if(tamanho<0 || (size_t)tamanho>=sizeof url)return DRIVE_LOCAL_INVALIDO;
  char *s=NULL;size_t n=0;int resultado=executar(c,"GET",url,NULL,NULL,0,16384,&s,&n);
  if(resultado!=DRIVE_LOCAL_OK)return resultado;
  char lista[16384],mime[80],props[512],marca[8],arquivoId[200];
  resultado=DRIVE_LOCAL_RESPOSTA;
  if(!objeto(s) || !js_bruto(s,NULL,"files",lista,sizeof lista) || *lista!='[')goto fim;
  const char *p=pular(lista+1);
  if(*p!=']') {
    const char *f=js_fim(p);
    if(*p!='{' || f<=p || f[-1]!='}' || *pular(f)!=']' || *pular(pular(f)+1) ||
       !js_texto_raiz_em(p,f,"id",arquivoId,sizeof arquivoId) || !identificador(arquivoId,199) ||
       !js_texto_raiz_em(p,f,"mimeType",mime,sizeof mime) || strcmp(mime,"application/vnd.google-apps.folder") ||
       !js_bruto(p,f,"appProperties",props,sizeof props) || !objeto(props) ||
       !js_texto_raiz(props,"nuvioBackupFolder",marca,sizeof marca) || strcmp(marca,"1"))goto fim;
    strcpy(id,arquivoId);resultado=DRIVE_LOCAL_OK;goto fim;
  }
  if(*pular(p+1))goto fim;
  resultado=DRIVE_LOCAL_AUSENTE;
  if(criar) {
    free(s);s=NULL;
    const char corpo[]="{\"name\":\"Nuvio Backups\",\"mimeType\":\"application/vnd.google-apps.folder\",\"appProperties\":{\"nuvioBackupFolder\":\"1\"}}";
    resultado=executar(c,"POST",DRIVE_API "?fields=id","Content-Type: application/json; charset=UTF-8",
                       corpo,sizeof corpo-1,16384,&s,&n);
    if(resultado==DRIVE_LOCAL_OK) {
      if(!objeto(s) || !js_texto_raiz(s,"id",arquivoId,sizeof arquivoId) || !identificador(arquivoId,199))resultado=DRIVE_LOCAL_RESPOSTA;
      else strcpy(id,arquivoId);
    }
  }
fim:
  free(s);return resultado;
}
int drivelocal_listar(const DriveLocal *c,const char *pagina,DrivePagina *saida) {
  if(saida)memset(saida,0,sizeof *saida);
  if(!saida || !valido(c))return DRIVE_LOCAL_INVALIDO;
  char consulta[512];snprintf(consulta,sizeof consulta,
    "trashed=false and appProperties has { key='nuvioBackupVersion' and value='1' } and appProperties has { key='nuvioOwner' and value='%s' } and appProperties has { key='nuvioProfile' and value='%d' }",c->dono,c->perfil);
  char *q=escapar(consulta,511),*pg=escapar(pagina?pagina:"",4095);if(!q || !pg){free(q);free(pg);return DRIVE_LOCAL_INVALIDO;}
  size_t cap=strlen(q)+strlen(pg)+512;char *url=malloc(cap);if(!url){free(q);free(pg);return DRIVE_LOCAL_INVALIDO;}
  snprintf(url,cap,DRIVE_API "?spaces=drive&pageSize=50&orderBy=createdTime%%20desc&q=%s&fields=nextPageToken,files(id,name,mimeType,size,createdTime,appProperties)%s%s",q,*pg?"&pageToken=":"",pg);
  free(q);free(pg);char *corpo=NULL;size_t n=0;int resultado=executar(c,"GET",url,NULL,NULL,0,128u*1024u,&corpo,&n);free(url);
  if(resultado!=DRIVE_LOCAL_OK)return resultado;
  DrivePagina *out=calloc(1,sizeof *out);char *lista=malloc(n+1);
  resultado=DRIVE_LOCAL_RESPOSTA;
  if(!out || !lista || !objeto(corpo) || !js_bruto(corpo,NULL,"files",lista,n+1) || *lista!='[')goto fim;
  if(js_tem(corpo,NULL,"nextPageToken") && !js_texto_raiz(corpo,"nextPageToken",out->proxima,sizeof out->proxima))goto fim;
  const char *p=pular(lista+1);int vistos=0;
  while(*p!=']') {
    if(*p!='{' || vistos++>=DRIVE_BACKUPS_PAGINA)goto fim;
    const char *f=js_fim(p);if(f<=p || f[-1]!='}')goto fim;
    DriveBackup a;int estado=arquivo(c,p,f,&a);if(estado<0)goto fim;
    if(estado)out->arquivo[out->n++]=a;
    p=pular(f);if(*p==']')break;
    if(*p++!=',')goto fim;
    p=pular(p);if(*p==']')goto fim;
  }
  if(*pular(p+1))goto fim;
  *saida=*out;resultado=DRIVE_LOCAL_OK;
fim:
  free(lista);free(out);free(corpo);return resultado;
}
int drivelocal_enviar(const DriveLocal *c,const char *snapshot,size_t n,char id[200]) {
  if(id)*id=0;
  if(!id || !valido(c))return DRIVE_LOCAL_INVALIDO;
  BackupLocal *b=NULL;if(!backuplocal_ler(snapshot,n,c->dono,c->perfil,&b))return DRIVE_LOCAL_INVALIDO;
  // The envelope reader accepted an exact byte span; add a terminator for
  // boundary collision detection without assuming the caller supplied one.
  char *texto=malloc(n+1);if(!texto){backuplocal_liberar(b);return DRIVE_LOCAL_INVALIDO;}
  memcpy(texto,snapshot,n);texto[n]=0;
  char pasta[200];int resultado=drivelocal_pasta(c,1,pasta);
  if(resultado!=DRIVE_LOCAL_OK){free(texto);backuplocal_liberar(b);return resultado;}
  char nome[128],perfil[16],fronteira[80];snprintf(nome,sizeof nome,"Nuvio-settings-p%d-%lld.txt",c->perfil,backuplocal_criado(b));
  snprintf(perfil,sizeof perfil,"%d",c->perfil);backuplocal_liberar(b);
  unsigned k=0;
  do{snprintf(fronteira,sizeof fronteira,"nuvio-local-settings-%u",k++);}while(strstr(texto,fronteira) && k<1000);
  if(strstr(texto,fronteira)){free(texto);return DRIVE_LOCAL_INVALIDO;}
  Jsw w;jsw_iniciar(&w);jsw_obj_ini(&w);jsw_cs(&w,"name",nome);jsw_cs(&w,"mimeType",DRIVE_MIME);
  jsw_chave(&w,"parents");jsw_arr_ini(&w);jsw_str(&w,pasta);jsw_arr_fim(&w);
  jsw_chave(&w,"appProperties");jsw_obj_ini(&w);jsw_cs(&w,"nuvioBackupVersion","1");
  jsw_cs(&w,"nuvioOwner",c->dono);jsw_cs(&w,"nuvioProfile",perfil);jsw_obj_fim(&w);jsw_obj_fim(&w);
  const char *meta=jsw_texto_final(&w);char *corpo=NULL;
  size_t cap=meta?strlen(meta)+n+512:0;
  if(meta)corpo=malloc(cap);
  if(!corpo){jsw_livre(&w);free(texto);return DRIVE_LOCAL_INVALIDO;}
  int inicio=snprintf(corpo,cap,"--%s\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n%s\r\n--%s\r\nContent-Type: " DRIVE_MIME "\r\n\r\n",fronteira,meta,fronteira);
  if(inicio<0 || (size_t)inicio+n>=cap){free(corpo);jsw_livre(&w);free(texto);return DRIVE_LOCAL_INVALIDO;}
  jsw_livre(&w);memcpy(corpo+inicio,texto,n);free(texto);
  size_t pos=(size_t)inicio+n;
  int fim=snprintf(corpo+pos,cap-pos,"\r\n--%s--\r\n",fronteira);
  if(fim<0 || (size_t)fim>=cap-pos){free(corpo);return DRIVE_LOCAL_INVALIDO;}
  char tipo[128];snprintf(tipo,sizeof tipo,"Content-Type: multipart/related; boundary=%s",fronteira);
  char *resposta=NULL;size_t nr=0;
  resultado=executar(c,"POST",DRIVE_UPLOAD,tipo,corpo,(size_t)inicio+n+(size_t)fim,16384,&resposta,&nr);free(corpo);
  if(resultado==DRIVE_LOCAL_OK) {
    char arquivo[200];
    if(!objeto(resposta) || !js_texto_raiz(resposta,"id",arquivo,sizeof arquivo) || !identificador(arquivo,sizeof arquivo-1))resultado=DRIVE_LOCAL_RESPOSTA;
    else strcpy(id,arquivo);
  }
  free(resposta);return resultado;
}
int drivelocal_baixar(const DriveLocal *c,const char *id,BackupLocal **saida) {
  if(saida)*saida=NULL;
  if(!saida || !valido(c) || !identificador(id,199))return DRIVE_LOCAL_INVALIDO;
  char url[512];snprintf(url,sizeof url,DRIVE_API "/%s?alt=media",id);
  char *corpo=NULL;size_t n=0;int resultado=executar(c,"GET",url,NULL,NULL,0,BACKUP_LOCAL_MAX,&corpo,&n);
  if(resultado==DRIVE_LOCAL_OK && !backuplocal_ler(corpo,n,c->dono,c->perfil,saida))resultado=DRIVE_LOCAL_RESPOSTA;
  free(corpo);return resultado;
}
