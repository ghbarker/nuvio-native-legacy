// Derived presentation only: channel IDs, cached records and EPG ownership stay
// unchanged. Rebuild from channel origin so old guide caches also gain labels.
#define G_MAX_GRUPOS (IPTV_MAX_CONTAS + G_MAX_FONTE + 1)
typedef struct { int tipo, ordem; unsigned id; char base[600], nome[96]; } GGrupo;
static GGrupo grupos[G_MAX_GRUPOS];
static int nGrupos, catGrupo[G_MAX_CAT];
static IptvConta grupoContas[IPTV_MAX_CONTAS];
static int grupoNContas;

static int grupoDoCanal(const GCanal *c) {
  unsigned id=0;int tipo=canalConta(c->id,&id);
  for(int i=0;i<nGrupos;i++)if(grupos[i].tipo==tipo && grupos[i].id==id &&
      (tipo || !strcmp(grupos[i].base,c->base)))return i;
  if(nGrupos>=G_MAX_GRUPOS)return 0;
  int i=nGrupos++;GGrupo *g=&grupos[i];memset(g,0,sizeof *g);
  g->tipo=tipo;g->id=id;g->ordem=IPTV_MAX_CONTAS+G_MAX_FONTE;
  if(tipo) {
    snprintf(g->nome,sizeof g->nome,"%s",tipo==IPTV_XTREAM?"Xtream":tipo==IPTV_M3U?"M3U":"Stalker");
    for(int j=0;j<grupoNContas;j++)if(grupoContas[j].tipo==tipo && grupoContas[j].id==id) {
      g->ordem=j;snprintf(g->nome,sizeof g->nome,"%s",grupoContas[j].nome);break;
    }
  } else {
    snprintf(g->base,sizeof g->base,"%s",c->base);
    for(int j=0;j<addons_n();j++)if(c->base[0] && !strcmp(addons_base(j),c->base)) {
      g->ordem=IPTV_MAX_CONTAS+j;snprintf(g->nome,sizeof g->nome,"%s",addons_nome(j));break;
    }
  }
  return i;
}
static int grupoOrdemConta;
static int grupoComparar(const void *aa,const void *bb) {
  int a=*(const int*)aa,b=*(const int*)bb;
  int ca=strcasecmp(cats[a],cats[b]);
  int ga=catGrupo[a],gb=catGrupo[b],origem=grupos[ga].ordem-grupos[gb].ordem;
  if(grupoOrdemConta && origem)return origem;
  if(ca)return ca;
  if(origem)return origem;
  return ga!=gb?(ga<gb?-1:1):(a>b)-(a<b);
}
static void gruposReunir(void) {
  static char nomes[G_MAX_CAT][64];
  int mapa[G_MAX_CAT],inverso[G_MAX_CAT],origens[G_MAX_CAT],antes=nCats;
  memcpy(nomes,cats,sizeof nomes);
  grupoNContas=iptv_contas(grupoContas,IPTV_MAX_CONTAS);nGrupos=nCats=0;
  for(int i=0;i<nCanais;i++) {
    GCanal *c=&canais[i];
    const char *nome=c->cat>=0 && c->cat<antes?nomes[c->cat]:"Outros";
    int grupo=grupoDoCanal(c),cat=0;
    for(;cat<nCats;cat++)if(catGrupo[cat]==grupo && !strcmp(cats[cat],nome))break;
    if(cat==nCats) { // At most one displayed category per retained channel.
      snprintf(cats[cat],sizeof cats[cat],"%s",nome);catGrupo[cat]=grupo;nCats++;
    }
    c->cat=cat;
  }
  grupoOrdemConta=ajustes_guia_ordem()==1;
  for(int i=0;i<nCats;i++)mapa[i]=i;
  qsort(mapa,(size_t)nCats,sizeof *mapa,grupoComparar);
  memcpy(nomes,cats,sizeof nomes);memcpy(origens,catGrupo,sizeof origens);
  for(int i=0;i<nCats;i++) {
    memcpy(cats[i],nomes[mapa[i]],sizeof cats[i]);catGrupo[i]=origens[mapa[i]];inverso[mapa[i]]=i;
  }
  for(int i=0;i<nCanais;i++)canais[i].cat=inverso[canais[i].cat];
}
static const char *categoriaRotulo(int cat) {
  static char texto[256];
  if(cat<0 || cat>=nCats)return "";
  const char *nome=catGrupo[cat]>=0 && catGrupo[cat]<nGrupos?grupos[catGrupo[cat]].nome:"";
  if(!nome[0])return i18n(cats[cat]);
  snprintf(texto,sizeof texto,"%s · %s",i18n(cats[cat]),nome);return texto;
}
