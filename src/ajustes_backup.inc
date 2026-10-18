// This is the scalar settings component, not a full installation backup.
// Separate settings stores and Drive transport are handled by their owners.
#include <limits.h>
#define AB_MAX_TEXTO 65536u
#define AB_N (AJ_N + 3)
static const char *const abExtras[] = {
  "idiomaAutoLocal", "idiomaFonteLocal", "texturasEscolhaLocal"
};
static const char *abNome(int op) { return op < AJ_N ? CHAVE[op] : abExtras[op-AJ_N]; }
static int abOp(const char *chave) {
  for(int i=0;i<AJ_N;i++)if(CHAVE[i] && !strcmp(CHAVE[i],chave))return i;
  for(int i=0;i<3;i++)if(!strcmp(abExtras[i],chave))return AJ_N+i;
  return -1;
}
static int abLocal(int op,int perfil,int v) {
  if(op<0 || op>=AB_N)return 0;
  if(op>=AJ_N)return !perfil;
  if(!CHAVE[op] || CHAVE[op][0]=='-' ||
     OPCOES[op].tipo==OP_ACAO || OPCOES[op].tipo==OP_LEITURA)return 0;
  // Fixed account themes remain Nuvio's. Native dynamic themes are the
  // existing deliberate exception in ajustes_mesclar_blob().
  if(op==AJ_TEMA)return perfil && v>=AJ_N_TEMAS && v<AJ_N_TEMAS_OPC;
  return somenteDesteAparelho(op) && !!perfilLocal(op)==perfil;
}
static int abValido(int op,int v) {
  if(op==AJ_N+1)return v>=IDA_TMDB && v<=IDA_PADRAO;
  if(op>=AJ_N)return v==0 || v==1;
  const Opcao *o=&OPCOES[op];
  return o->tipo==OP_NUMERO ? v>=o->min && v<=o->max : v>=0 && v<nValores(op);
}
// A bounded line view; keep original non-backup lines byte-for-byte on merge.
static const char *abLinha(const char *p,const char **fim,char chave[80],const char **valorIni) {
  *fim=strchr(p,'\n');if(!*fim)*fim=p+strlen(p);
  const char *q=p;while(q<*fim && (*q==' ' || *q=='\t' || *q=='\r'))q++;
  size_t n=0;
  while(q<*fim && (unsigned char)*q>32) {
    if(n<79)chave[n++]=*q;else { chave[0]=0;*valorIni=*fim;return **fim?*fim+1:*fim; }
    q++;
  }
  chave[n]=0;while(q<*fim && (*q==' ' || *q=='\t'))q++;
  *valorIni=q;return **fim?*fim+1:*fim;
}
static int abNumero(const char *p,const char *fim,int *out) {
  int negativo=0;unsigned long long n=0,limite;
  if(p<fim && (*p=='-' || *p=='+')) { negativo=*p=='-';p++; }
  if(p==fim || *p<'0' || *p>'9')return 0;
  limite=negativo?(unsigned long long)INT_MAX+1u:(unsigned long long)INT_MAX;
  while(p<fim && *p>='0' && *p<='9') {
    unsigned d=(unsigned)(*p++-'0');if(n>(limite-d)/10u)return 0;n=n*10u+d;
  }
  while(p<fim && (*p==' ' || *p=='\t' || *p=='\r'))p++;
  if(p!=fim)return 0;
  *out=negativo?(n==(unsigned long long)INT_MAX+1u?INT_MIN:-(int)n):(int)n;
  return 1;
}
static int abTexto(const char *s) { return s && strnlen(s,AB_MAX_TEXTO+1u)<=AB_MAX_TEXTO; }
static int abJuntar(char *out,size_t cap,size_t *n,const char *p,size_t len) {
  if(len>=cap-*n)return 0;
  memcpy(out+*n,p,len);*n+=len;out[*n]=0;return 1;
}
static int abEscrever(char *out,size_t cap,size_t *n,int op,int v) {
  char linha[128];int len=snprintf(linha,sizeof linha,"%s %d\n",abNome(op),v);
  return len>0 && (size_t)len<sizeof linha && abJuntar(out,cap,n,linha,(size_t)len);
}
int ajustes_backup_exportar(int perfil,const char *origem,char **saida) {
  if(saida)*saida=NULL;
  if(!saida || (perfil!=0 && perfil!=1) || !abTexto(origem))return 0;
  unsigned char viu[AB_N]={0};int valores[AB_N]={0};
  for(const char *p=origem;*p;) {
    char chave[80];const char *fim,*v;const char *proximo=abLinha(p,&fim,chave,&v);
    int op=abOp(chave),valorLido;
    if(op>=0) {
      // Values belonging to Nuvio or the other scope need not be numeric
      // for this component to preserve/ignore them.
      if(abLocal(op,perfil,AJ_N_TEMAS)) {
        if(!abNumero(v,fim,&valorLido))return 0;
        if(abLocal(op,perfil,valorLido)) {
          if(!abValido(op,valorLido))return 0;
          viu[op]=1;valores[op]=valorLido;
        }
      }
    }
    p=proximo;
  }
  char *out=calloc(AB_MAX_TEXTO+1u,1);if(!out)return 0;
  size_t n=(size_t)snprintf(out,AB_MAX_TEXTO+1u,"NUVIO-LOCAL-SETTINGS 1\nscope %s\n",perfil?"profile":"device");
  for(int i=0;i<AB_N;i++)if(viu[i] && !abEscrever(out,AB_MAX_TEXTO+1u,&n,i,valores[i])) { free(out);return 0; }
  *saida=out;return 1;
}
int ajustes_backup_mesclar(int perfil,const char *atual,const char *backup,char **saida) {
  if(saida)*saida=NULL;
  if(!saida || (perfil!=0 && perfil!=1) || !abTexto(atual) || !abTexto(backup))return 0;
  char cab[80];int k=snprintf(cab,sizeof cab,"NUVIO-LOCAL-SETTINGS 1\nscope %s\n",perfil?"profile":"device");
  if(strncmp(backup,cab,(size_t)k))return 0;
  unsigned char viu[AB_N]={0},escreveu[AB_N]={0};int valores[AB_N]={0};
  for(const char *p=backup+k;*p;) {
    char chave[80];const char *fim,*v;const char *proximo=abLinha(p,&fim,chave,&v);
    int op=abOp(chave),valorLido;
    if(op<0 || viu[op] || !abNumero(v,fim,&valorLido) ||
       !abLocal(op,perfil,valorLido) || !abValido(op,valorLido))return 0;
    viu[op]=1;valores[op]=valorLido;p=proximo;
  }
  // Complete validation precedes output construction. The caller later owns
  // atomic persistence, identity checks and module reloads.
  size_t cap=AB_MAX_TEXTO+1u,n=0;char *out=calloc(cap,1);if(!out)return 0;
  for(const char *p=atual;*p;) {
    char chave[80];const char *fim,*v;const char *proximo=abLinha(p,&fim,chave,&v);
    int op=abOp(chave),ok=1;
    if(op>=0 && viu[op]) {
      if(!escreveu[op])ok=abEscrever(out,cap,&n,op,valores[op]);
      escreveu[op]=1;
    } else ok=abJuntar(out,cap,&n,p,(size_t)(proximo-p));
    if(!ok) { free(out);return 0; }p=proximo;
  }
  for(int i=0;i<AB_N;i++)if(viu[i] && !escreveu[i]) {
    if(n && out[n-1]!='\n' && !abJuntar(out,cap,&n,"\n",1)) { free(out);return 0; }
    if(!abEscrever(out,cap,&n,i,valores[i])) { free(out);return 0; }
  }
  *saida=out;return 1;
}
