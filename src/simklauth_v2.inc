// AUTH V2 device flow has no PKCE/redirect: the device code is its secret.
// https://api.simkl.org/api-reference/oauth2-device
static char *oauthPost(const char *path,const char *fields,int *st) {
  char endpoint[160],cid[600],body[2000];
  nuvem_url_escapar(nuvem_simkl_cliente(),cid,sizeof cid);
  snprintf(endpoint,sizeof endpoint,"%s%s",SMK_BASE,path);
  int n=snprintf(body,sizeof body,"client_id=%s&%s",cid,fields);
  if(n<0 || (size_t)n>=sizeof body) { if(st)*st=0;return NULL; }
  const char *cab[]={"Content-Type: application/x-www-form-urlencoded",
    "Accept: application/json","User-Agent: Nuvio-Legacy/2",NULL};
  char *r=rede_postar_st(endpoint,20,cab,body,st);
  memset(body,0,sizeof body);return r;
}

static int tokensV2(const char *r,char *rt,size_t cap,long long *exp) {
  char tipo[24]="",scope[160]="",refresh[300]="";
  const char *fim=r?r+strlen(r):NULL;
  if(!r || !js_texto(r,fim,"refresh_token",refresh,sizeof refresh) || !refresh[0]) return 0;
  js_texto(r,fim,"token_type",tipo,sizeof tipo);js_texto(r,fim,"scope",scope,sizeof scope);
  const char *w=strstr(scope,"media:write");
  if(strcasecmp(tipo,"Bearer") || !w || (w!=scope && w[-1]!=' ') || (w[11] && w[11]!=' ')) return 0;
  double segundos=js_num(r,fim,"expires_in",0);
  if(!(segundos>=60 && segundos<=2592000)) return 0;
  snprintf(rt,cap,"%s",refresh);*exp=(long long)time(NULL)+(long long)segundos;
  memset(refresh,0,sizeof refresh);return 1;
}

extern void simkl_credencial_renovada(const char *,const char *);

int simklauth_atualizar(void) {
  // One refresh owner per process. Other workers reuse its new access token.
  pthread_mutex_lock(&refreshTrava);
  char rt[300],usuario[80],antigo[300];unsigned ger;int dono;
  pthread_mutex_lock(&trava);
  if(!authV2 || !refreshToken[0] || (expiresAt>(long long)time(NULL)+300)) {
    int ok=token[0]!=0;pthread_mutex_unlock(&trava);pthread_mutex_unlock(&refreshTrava);return ok;
  }
  if(refreshAfter>(long long)time(NULL)) {
    int ok=token[0] && expiresAt>(long long)time(NULL);
    pthread_mutex_unlock(&trava);pthread_mutex_unlock(&refreshTrava);return ok;
  }
  snprintf(rt,sizeof rt,"%s",refreshToken);snprintf(antigo,sizeof antigo,"%s",token);
  snprintf(usuario,sizeof usuario,"%s",usuarioEstado);ger=geracao;dono=perfil;
  pthread_mutex_unlock(&trava);
  char esc[900],body[1100];nuvem_url_escapar(rt,esc,sizeof esc);
  snprintf(body,sizeof body,"grant_type=refresh_token&refresh_token=%s",esc);
  int st=0;char *r=oauthPost("/oauth2/token",body,&st);
  memset(body,0,sizeof body);memset(esc,0,sizeof esc);
  char tk[300]="",novoRt[300]="",e[64]="";long long exp=0;
  int ok=r && st>=200 && st<300 && js_texto(r,r+strlen(r),"access_token",tk,sizeof tk) && tk[0] && tokensV2(r,novoRt,sizeof novoRt,&exp);
  if(r)js_texto(r,r+strlen(r),"error",e,sizeof e);
  pthread_mutex_lock(&trava);
  if(ger==geracao && perfil==dono && mesmaConta(usuario) && !strcmp(token,antigo) && !strcmp(refreshToken,rt)) {
    if(ok) {
      credGeracao++;snprintf(token,sizeof token,"%s",tk);
      snprintf(refreshToken,sizeof refreshToken,"%s",novoRt);expiresAt=exp;refreshAfter=0;
      gravar();printf("[simkl] AUTH V2 acesso renovado, perfil %d\n",dono);
    } else if(st==400 && !strcmp(e,"invalid_grant")) {
      // The grant expired or was revoked. A transport failure is not logout.
      geracao++;zerarEstado();dados_apagar(arq(dono).s);estado=SMK_ERRO;
      snprintf(erro,sizeof erro,"%s",i18n("Não foi possível agora. Tente novamente."));
    } else {
      refreshAfter=(long long)time(NULL)+60;
      printf("[simkl] AUTH V2 renovacao HTTP %d; nova tentativa em 60s\n",st);
    }
    ok=token[0] && (!authV2 || expiresAt>(long long)time(NULL));
  } else ok=0;
  pthread_mutex_unlock(&trava);
  if(ok && tk[0]) simkl_credencial_renovada(antigo,tk);
  free(r);memset(rt,0,sizeof rt);memset(tk,0,sizeof tk);memset(novoRt,0,sizeof novoRt);memset(antigo,0,sizeof antigo);
  pthread_mutex_unlock(&refreshTrava);return ok;
}

static void *fioRefresh(void *unused) {
  (void)unused;simklauth_atualizar();fioPronto=1;return NULL;
}
