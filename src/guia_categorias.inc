// Hidden categories are presentation state, independent from loaded source data.
#define G_MAX_OCULTAS 30000
static uint64_t categoriasOcultas[G_MAX_OCULTAS];
static int nCategoriasOcultas,nCategoriasVisiveis,categoriasPerfil=-1;
static uint64_t categoriasVisiveis[G_MAX_OCULTAS];
#ifdef NV_GUIA_BACKUP_CATEGORIAS
static BackupCategorias *categoriasOrigem;
#endif
static uint64_t categoriasSessao;
static int categoriaComparar(const void *aa,const void *bb) {
  uint64_t a=*(const uint64_t*)aa,b=*(const uint64_t*)bb;return (a>b)-(a<b);
}
static void categoriasOrdenar(void) {
  qsort(categoriasOcultas,(size_t)nCategoriasOcultas,sizeof *categoriasOcultas,categoriaComparar);
  qsort(categoriasVisiveis,(size_t)nCategoriasVisiveis,sizeof *categoriasVisiveis,categoriaComparar);
}
static uint64_t categoriaChave(int cat) {
  if(cat<0 || cat>=nCats)return 0;
  const GFonteLtv *f=&ltvF[cats[cat].fonte];unsigned id=0;
  int tipo=f->tipo==LTV_XTREAM?IPTV_XTREAM:f->tipo==LTV_STALKER?IPTV_STALKER:f->tipo==LTV_M3U?IPTV_M3U:0;
  char origem[64];
  // Keep saved category choices from the approved multi-account builds.
  if(tipo && fonteConta(f,&id))snprintf(origem,sizeof origem,"%d:%u:",tipo,id);
  else if(!tipo)snprintf(origem,sizeof origem,"0:0:");
  else snprintf(origem,sizeof origem,"url:%s:",f->chave);
  const char *partes[]={origem,tipo?"":f->base,cats[cat].nome};
  uint64_t h=1469598103934665603ULL;
  for(int i=0;i<3;i++) { for(const unsigned char *p=(const unsigned char*)partes[i];*p;p++)h=(h^*p)*1099511628211ULL;h=(h^0)*1099511628211ULL; }
  return h?h:1;
}
static void categoriasGarantir(void) {
  int perfil=perfis_ativo();uint64_t sessao=nv_perfil_sessao_ler();
  if(categoriasPerfil==perfil && categoriasSessao==sessao)return;
  categoriasPerfil=perfil;categoriasSessao=sessao;nCategoriasOcultas=nCategoriasVisiveis=0;
#ifdef NV_GUIA_BACKUP_CATEGORIAS
  backupcategorias_liberar(categoriasOrigem);categoriasOrigem=NULL;
  char origemNome[64];arquivoPerfil("categorias-origem",origemNome,sizeof origemNome);
  char *origem=dados_ler(origemNome);if(origem){backupcategorias_ler(origem,&categoriasOrigem);free(origem);}
#endif
  char nome[64];arquivoPerfil("categorias",nome,sizeof nome);char *texto=dados_ler(nome);
  if(texto) {
    for(char *p=texto;*p;) {
      char *fim=strchr(p,'\n');if(fim)*fim++=0;else fim=p+strlen(p);
      unsigned long long chave;char sobra;
      if(strlen(p)==21 && !strncmp(p,"show ",5) && sscanf(p+5,"%16llx%c",&chave,&sobra)==1 && chave && nCategoriasVisiveis<G_MAX_OCULTAS)
        categoriasVisiveis[nCategoriasVisiveis++]=(uint64_t)chave;
      if(nCategoriasOcultas<G_MAX_OCULTAS && strlen(p)==16 && sscanf(p,"%16llx%c",&chave,&sobra)==1 && chave) {
        int i=0;for(;i<nCategoriasOcultas;i++)if(categoriasOcultas[i]==chave)break;
        if(i==nCategoriasOcultas)categoriasOcultas[nCategoriasOcultas++]=(uint64_t)chave;
      }
      p=fim;
    }
    free(texto);
  }
  categoriasOrdenar();
}
static int categoriaOculta(int cat) {
  if(cat<0 || cat>=nCats)return 0;
  uint64_t chave=categoriaChave(cat);
  if(bsearch(&chave,categoriasVisiveis,(size_t)nCategoriasVisiveis,sizeof *categoriasVisiveis,categoriaComparar))return 0;
  if(bsearch(&chave,categoriasOcultas,(size_t)nCategoriasOcultas,sizeof *categoriasOcultas,categoriaComparar))return 1;
#ifdef NV_GUIA_BACKUP_CATEGORIAS
  const GFonteLtv *f=&ltvF[cats[cat].fonte];unsigned id=0;
  int tipo=f->tipo==LTV_XTREAM?IPTV_XTREAM:f->tipo==LTV_STALKER?IPTV_STALKER:f->tipo==LTV_M3U?IPTV_M3U:0;
  if(!tipo || fonteConta(f,&id))return categoriasOrigem && backupcategorias_oculta(categoriasOrigem,tipo,id,f->base,cats[cat].nome)>0;
#endif
  return 0;
}
static int categoriaOcultaPorChave(uint64_t chave) {
  for(int i=0;i<nCats;i++)if(categoriaChave(i)==chave)return categoriaOculta(i);
  if(bsearch(&chave,categoriasVisiveis,(size_t)nCategoriasVisiveis,sizeof *categoriasVisiveis,categoriaComparar))return 0;
  return bsearch(&chave,categoriasOcultas,(size_t)nCategoriasOcultas,sizeof *categoriasOcultas,categoriaComparar)!=NULL;
}
int guia_categorias(GuiaCategoria *out,int max) {
  categoriasGarantir();int n=out && max<nCats?max:nCats;
  if(!out)return nCats;
  if(n<0)n=0;
  for(int i=0;i<n;i++) {
    GuiaCategoria *c=&out[i];memset(c,0,sizeof *c);c->chave=categoriaChave(i);c->sessao=categoriasSessao;
    snprintf(c->nome,sizeof c->nome,"%s",i18n(cats[i].nome));
    snprintf(c->conta,sizeof c->conta,"%s",ltvF[cats[i].fonte].nome);
    c->oculta=categoriaOculta(i);
  }
  return n;
}
