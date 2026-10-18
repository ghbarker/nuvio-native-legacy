// Profile-local presentation preferences; cached channels remain untouched.
#define G_MAX_OCULTAS 4096
static uint64_t categoriasOcultas[G_MAX_OCULTAS];
static int nCategoriasOcultas, categoriasPerfil = -1;
static uint64_t categoriasSessao;
static int catsVisiveis[G_MAX_CAT], nCatsVisiveis;
static uint64_t categoriaChave(int cat) {
  const GGrupo *g=&grupos[catGrupo[cat]];
  char origem[48];snprintf(origem,sizeof origem,"%d:%u:",g->tipo,g->id);
  const char *partes[]={origem,g->tipo?"":g->base,cats[cat]};
  uint64_t h=1469598103934665603ULL;
  for(int i=0;i<3;i++) { for(const unsigned char *p=(const unsigned char*)partes[i];*p;p++)h=(h^*p)*1099511628211ULL;h=(h^0)*1099511628211ULL; }
  return h?h:1;
}
static void categoriasGarantir(void) {
  int perfil=perfis_ativo();uint64_t sessao=nv_perfil_sessao_ler();
  if(categoriasPerfil==perfil && categoriasSessao==sessao)return;
  categoriasPerfil=perfil;categoriasSessao=sessao;nCategoriasOcultas=0;
  char nome[64];arquivoPerfil("categorias",nome,sizeof nome);char *texto=dados_ler(nome);
  if(texto) {
    for(char *p=texto;*p && nCategoriasOcultas<G_MAX_OCULTAS;) {
      char *fim=strchr(p,'\n');if(fim)*fim++=0;else fim=p+strlen(p);
      unsigned long long chave;char sobra;
      if(strlen(p)==16 && sscanf(p,"%16llx%c",&chave,&sobra)==1 && chave) {
        int i=0;for(;i<nCategoriasOcultas;i++)if(categoriasOcultas[i]==chave)break;
        if(i==nCategoriasOcultas)categoriasOcultas[nCategoriasOcultas++]=(uint64_t)chave;
      }
      p=fim;
    }
    free(texto);
  }
}
static int categoriaOculta(uint64_t chave) {
  for(int i=0;i<nCategoriasOcultas;i++)if(categoriasOcultas[i]==chave)return 1;
  return 0;
}
static void categoriasReunir(void) {
  categoriasGarantir();nCatsVisiveis=0;
  for(int i=0;i<nCats;i++)if(!categoriaOculta(categoriaChave(i)))catsVisiveis[nCatsVisiveis++]=i;
}
int guia_categorias(GuiaCategoria *out,int max) {
  categoriasGarantir();int n=out && max<nCats?max:nCats;
  if(!out)return nCats;
  if(n<0)n=0;
  for(int i=0;i<n;i++) {
    GuiaCategoria *c=&out[i];memset(c,0,sizeof *c);c->chave=categoriaChave(i);c->sessao=categoriasSessao;
    snprintf(c->nome,sizeof c->nome,"%s",i18n(cats[i]));
    snprintf(c->conta,sizeof c->conta,"%s",grupos[catGrupo[i]].nome);
    c->oculta=categoriaOculta(c->chave);
  }
  return n;
}
