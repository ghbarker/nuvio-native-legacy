// Pure snapshot codec. No file writes, active-config changes or credentials.
// Ownership/profile checks and transactional persistence belong to restore.
#include <stdarg.h>
#define FB_MAX_TEXTO 16384u
static int fbConfigValida(const FonteFalhaConfig *c) {
  if(!c || (c->provedores!=0 && c->provedores!=1) ||
     (c->addons!=0 && c->addons!=1) || !ordemVale(c->ordem) ||
     c->nAddons<0 || c->nAddons>FONTEFALHA_MAX_ADDONS || !fontefiltro_config_validar(&c->filtro))return 0;
  for(int i=0;i<4;i++)if(!limiteVale(c->limites[i]))return 0;
  for(int i=0;i<c->nAddons;i++) {
    if(!memchr(c->addon[i].chave,0,sizeof c->addon[i].chave) ||
       !chaveVale(c->addon[i].chave) || !limiteVale(c->addon[i].limite) ||
       (c->addon[i].aoVivo!=0 && c->addon[i].aoVivo!=1))return 0;
    for(int j=0;j<i;j++)if(!strcmp(c->addon[i].chave,c->addon[j].chave))return 0;
  }
  return 1;
}
static int fbAdicionar(char *out,size_t *n,const char *formato,...) {
  va_list ap;va_start(ap,formato);
  int k=vsnprintf(out+*n,FB_MAX_TEXTO+1u-*n,formato,ap);va_end(ap);
  if(k<0 || (size_t)k>FB_MAX_TEXTO-*n)return 0;
  *n+=(size_t)k;return 1;
}
int fontefalha_backup_exportar(const FonteFalhaConfig *c,char **saida) {
  if(saida)*saida=NULL;
  if(!saida || !fbConfigValida(c))return 0;
  char grupos[sizeof c->filtro.grupos];
  if(!fontefiltro_grupos_normalizar(c->filtro.grupos,grupos,sizeof grupos))return 0;
  char *out=calloc(FB_MAX_TEXTO+1u,1);if(!out)return 0;
  size_t n=0;int ok=fbAdicionar(out,&n,"NUVIO-SOURCE-SETTINGS 2\nprovedores %d\naddons %d\nordem %d %d %d %d\nlimites %d %d %d %d\ncontagem %d\n",
    c->provedores,c->addons,c->ordem[0],c->ordem[1],c->ordem[2],c->ordem[3],
    c->limites[0],c->limites[1],c->limites[2],c->limites[3],c->nAddons);
  for(int i=0;i<c->nAddons && ok;i++)ok=fbAdicionar(out,&n,"addon %s %d\naddon_live %s %d\n",c->addon[i].chave,c->addon[i].limite,c->addon[i].chave,c->addon[i].aoVivo);
  for(int i=0;i<FONTEFILTRO_N && ok;i++)ok=fbAdicionar(out,&n,"%s %016" PRIx64 "\n",filtroChaves[i],c->filtro.excluir[i]);
  if(ok)ok=fbAdicionar(out,&n,"filtro_maxgb %u\nfiltro_cache %u\nfiltro_grupos ",c->filtro.maxGB,c->filtro.cache);
  if(ok && !grupos[0])ok=fbAdicionar(out,&n,"-");
  for(const unsigned char *p=(const unsigned char *)grupos;ok && *p;p++)ok=fbAdicionar(out,&n,"%02x",(unsigned)*p);
  if(ok)ok=fbAdicionar(out,&n,"\n");
  if(!ok){free(out);return 0;}*saida=out;return 1;
}
typedef struct { const char *p,*fim; } FbLeitor;
static int fbLinha(FbLeitor *r,char *out,size_t n) {
  const char *fim=memchr(r->p,'\n',(size_t)(r->fim-r->p));
  if(!fim || (size_t)(fim-r->p)>=n)return 0;
  size_t k=(size_t)(fim-r->p);memcpy(out,r->p,k);out[k]=0;r->p=fim+1;return 1;
}
static char *fbPalavra(char **p) {
  while(**p==' ')(*p)++;
  if(!**p)return NULL;
  char *ini=*p;while(**p && **p!=' ')(*p)++;
  if(**p)*(*p)++=0;
  return ini;
}
static int fbInteiro(const char *p,int min,int max,int *out) {
  if(!p || !*p)return 0;
  // No signs, decimal fractions, trailing tokens or overflowing conversions.
  unsigned n=0;
  for(;*p;p++) {
    if(*p<'0' || *p>'9')return 0;
    unsigned d=(unsigned)(*p-'0');if(d>(unsigned)max || n>((unsigned)max-d)/10u)return 0;
    n=n*10u+d;
  }
  if(n<(unsigned)min)return 0;
  *out=(int)n;return 1;
}
static char *fbValor(FbLeitor *r,const char *chave,char *linha,size_t n) {
  if(!fbLinha(r,linha,n))return NULL;
  size_t k=strlen(chave);
  if(strncmp(linha,chave,k) || linha[k]!=' ')return NULL;
  return linha+k+1;
}
static int fbNumeros(FbLeitor *r,const char *chave,int *out,int n,int min,int max) {
  char linha[128],*p=fbValor(r,chave,linha,sizeof linha);if(!p)return 0;
  for(int i=0;i<n;i++)if(!fbInteiro(fbPalavra(&p),min,max,&out[i]))return 0;
  return fbPalavra(&p)==NULL;
}
int fontefalha_backup_ler(const char *texto,size_t n,FonteFalhaConfig *saida) {
  if(!texto || !saida || !n || n>FB_MAX_TEXTO || memchr(texto,0,n))return 0;
  FbLeitor r={texto,texto+n};FonteFalhaConfig c={0};char linha[1100];
  if(!fbLinha(&r,linha,sizeof linha) || strcmp(linha,"NUVIO-SOURCE-SETTINGS 2") ||
     !fbNumeros(&r,"provedores",&c.provedores,1,0,1) || !fbNumeros(&r,"addons",&c.addons,1,0,1) ||
     !fbNumeros(&r,"ordem",c.ordem,4,0,3) || !fbNumeros(&r,"limites",c.limites,4,1,FONTEFALHA_MAX_LINKS) ||
     !fbNumeros(&r,"contagem",&c.nAddons,1,0,FONTEFALHA_MAX_ADDONS))return 0;
  for(int i=0;i<c.nAddons;i++) {
    char *p=fbValor(&r,"addon",linha,sizeof linha),*k;if(!p || !(k=fbPalavra(&p)) || !chaveVale(k))return 0;
    snprintf(c.addon[i].chave,sizeof c.addon[i].chave,"%s",k);
    if(!fbInteiro(fbPalavra(&p),1,FONTEFALHA_MAX_LINKS,&c.addon[i].limite) || fbPalavra(&p))return 0;
    p=fbValor(&r,"addon_live",linha,sizeof linha);if(!p || !(k=fbPalavra(&p)) || strcmp(k,c.addon[i].chave) ||
      !fbInteiro(fbPalavra(&p),0,1,&c.addon[i].aoVivo) || fbPalavra(&p))return 0;
  }
  for(int i=0;i<FONTEFILTRO_N;i++) {
    char *p=fbValor(&r,filtroChaves[i],linha,sizeof linha);
    if(!p || !filtroMascara(p,&c.filtro.excluir[i]))return 0;
  }
  char *p=fbValor(&r,"filtro_maxgb",linha,sizeof linha);
  if(!p || !filtroMaximo(p,&c.filtro.maxGB))return 0;
  p=fbValor(&r,"filtro_cache",linha,sizeof linha);if(!p || !filtroCache(p,&c.filtro.cache))return 0;
  p=fbValor(&r,"filtro_grupos",linha,sizeof linha);
  if(!p || !filtroGruposLer(p,c.filtro.grupos,sizeof c.filtro.grupos) || r.p!=r.fim || !fbConfigValida(&c))return 0;
  *saida=c;return 1;
}
