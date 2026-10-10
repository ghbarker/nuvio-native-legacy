/* The real TV Guide drawers and their clipped touch targets, with inert providers. */
#define TESTE_GUIA 1
#define main guia_existing_main
#include "utilidades_toque.c"
#undef main
#include "../src/guiaaddons.c"
#ifdef _WIN32
#define timegm _mkgmtime
#endif
#include "../src/js.c"

static char basesReview[16][64];
static int gravacoesReview;
static int globaisReview;
int addons_n(void) { return 16; }
const char *addons_base(int i) { assert(i>=0&&i<16); return basesReview[i]; }
const char *addons_nome(int i) { (void)i; return "Um addon com um nome longo e canais de noticias e documentarios"; }
int addons_ativo(int i) { return i%2; }
int addons_alternar(int i) { globaisReview++;return !(i%2); }
int ajustes_idioma(void) { return 0; }
int perfis_ativo(void) { return 1; }
char *dados_ler(const char *s) { (void)s; return NULL; }
int dados_gravar(const char *s,const char *t) { assert(strstr(s,"guia-addons-p1.txt"));(void)t;gravacoesReview++;return 1; }
int addons_catalogos_canal(int i,AddCatCanal *v,int n) { (void)i;(void)v;(void)n;return 0; }
int addons_base_desligada(const char *s) { (void)s;return 0; }
int stalker_configurado(void) { return 0; }
int stalker_canais(StalkerCanal *v,int n) { (void)v;(void)n;assert(!"provider called");return 0; }
int xtream_configurado(void) { return 0; }
int xtream_canais(XtreamCanal *v,int n) { (void)v;(void)n;assert(!"provider called");return 0; }
int xtream_ultima_falha(void) { return 0; }
int xtream_ultimo_http(void) { return 0; }
int xtream_ultimo_total(void) { return 0; }
char *rede_baixar(const char *s,int segundos) { (void)s;(void)segundos;assert(!"network called");return NULL; }
void fontecache_engatilhar(const char *a,const char *ba,const char *b,const char *bb) { (void)a;(void)ba;(void)b;(void)bb; }
void gfx_vidro_folha(GfxRect r,float rad,float a) { gfx_cor(r,rad,1,1,1,a); }
void plrui_linha_foco(GfxRect r,float rad,float a) { gfx_cor(r,rad,1,1,1,a); }

float txt_bloco_corta(TxtEstilo estilo,const char *s,int r,int g,int b,
                      float x,float y,float w,float leading,float a,int maxLinhas) {
  assert(w>0&&leading>=fonteGuiaTeste(estilo)*1.2f);
  int linhas=(int)ceilf(letrasGuiaTeste(s)*fonteGuiaTeste(estilo)*.55f/w);
  if(linhas<1)linhas=1;
  if(maxLinhas>0&&linhas>maxLinhas)linhas=maxLinhas;
  if(a>0) {
    TxtLinha t=txt_linha_corta(estilo,s,r,g,b,255,w);
    t.h=(int)(linhas*leading);
    txt_desenhar_alpha(t,x,y,a);
  }
  return linhas*leading;
}

#define ponteiro_alvo review_unused_alvo
#define ponteiro_alvo_faixa review_unused_faixa
#define ponteiro_camada review_unused_camada
#define ponteiro_rolagem review_unused_rolagem
#include "../src/ponteiro.c"
#undef ponteiro_alvo
#undef ponteiro_alvo_faixa
#undef ponteiro_camada
#undef ponteiro_rolagem

static void bounds(void) {
  assert(nAlvosGuia>0);
  for(int i=0;i<nAlvosGuia;i++) {
    const PonteiroAlvo *p=&alvosGuia[i];
    assert(p->x>=0&&p->y>=0&&p->w>0&&p->h>0);
    assert(p->x+p->w<=NV_TELA_W+.01f&&p->y+p->h<=NV_TELA_H+.01f);
  }
  for(int i=0;i<nDesenhosGuia;i++) {
    GfxRect r=desenhosGuia[i].r;
    /* The body is scissored; header/footer X budgets must independently fit. */
    assert(r.x>=0&&r.x+r.w<=NV_TELA_W+.01f);
  }
}
static void drag(PonteiroRolagemFn fn,float x,float y,float *off) {
  int beforeFoco=paFoco, beforeCat=catFoco, beforeLin=focoLin;
  float before=*off;
  PonteiroRolagem e={PONT_ROL_INICIO,1,0,0,x,y};assert(fn(&e));
  e.fase=PONT_ROL_MOVER;e.delta=-13.75f;assert(fn(&e));
  perto(*off,before+13.75f);assert(paFoco==beforeFoco&&catFoco==beforeCat&&focoLin==beforeLin);
  e.fase=PONT_ROL_SOLTAR;fn(&e);e.fase=PONT_ROL_INERCIA;e.delta=-1e6f;fn(&e);
  assert(!fn(&e));e.fase=PONT_ROL_FIM;fn(&e);
}
static void vazioBounds(void) {
  for(int i=0;i<nAlvosGuia;i++) {
    const PonteiroAlvo *p=&alvosGuia[i];
    assert(p->x>=G_AREA_X&&p->y>=recorteGuia.y&&p->w>0&&p->h>0);
    assert(p->x+p->w<=G_AREA_DIR+.01f&&p->y+p->h<=recorteGuia.y+recorteGuia.h+.01f);
  }
  for(int i=0;i<nDesenhosGuia;i++) {
    GfxRect r=desenhosGuia[i].r;
    assert(r.x>=G_AREA_X&&r.x+r.w<=G_AREA_DIR+.01f);
  }
}
static void vazioTelefoneReview(void) {
  testMobile=1;
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  const char *curto="O guia se enche por dois caminhos: um addon de canais instalado na conta, ou um portal IPTV cadastrado em Ajustes > Conta.";
  char longo[2400];
  memset(longo,'a',sizeof longo-1);longo[sizeof longo-1]=0;
  for(int i=7;i<(int)sizeof longo-1;i+=8)longo[i]=' ';
  int quadros=0,gestos=0,acoes=0;
  for(int d=0;d<4;d++)for(int escala=0;escala<2;escala++)for(int rotulo=0;rotulo<2;rotulo++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];
    escalaGuiaTeste=escala?1.5f:1;rotulosLongosGuiaTeste=rotulo;
    aberta=1;overlay=catAberto=buscaEstado=painel=0;entrada=1;
    nCanais=nCats=nFontes=0;fontesOk=1;estado=G_FALHOU;falhas=xtFalha=0;
    rolVazio=0;memset(&toqueVazio,0,sizeof toqueVazio);
    guiaTesteLimpar();gTopoMedir();desenharTopo(1);
    int topoN=nAlvosGuia;
    float baseTopo=G_TOPO_Y+G_TOPO_H;
    for(int i=0;i<topoN;i++)baseTopo=fmaxf(baseTopo,alvosGuia[i].y+alvosGuia[i].h);
    /* The empty screen starts under every actual header control, including
       a translated Preview chip that wraps onto its own row. */
    for(int l=0;l<2;l++)for(int configurar=0;configurar<2;configurar++) {
      guiaTesteLimpar();
      desenharVazioTelefone(l?longo:curto,configurar,1);quadros++;
      assert(toqueVazio.regiao.y>=baseTopo+39.99f);
      assert(recorteGuia.y==toqueVazio.regiao.y&&recorteGuia.h==toqueVazio.regiao.h);
      assert(recorteGuia.y+recorteGuia.h<=NV_TELA_H-79.99f);
      vazioBounds();
      for(int i=0;i<nDesenhosGuia;i++)assert(desenhosGuia[i].r.y>=recorteGuia.y-.01f);
      int visivel=configurar&&formasGuia[0].y<recorteGuia.y+recorteGuia.h&&
                  formasGuia[0].y+formasGuia[0].h>recorteGuia.y;
      assert(nAlvosGuia==visivel);
      if(visivel) {
        PonteiroAlvo setup=alvosGuia[0];
        assert(setup.focar==ponteiroConfigGuia&&!setup.ativar);
        assert(setup.y>=baseTopo+40&&setup.y+setup.h<=NV_TELA_H-80);
        focoTopo=0;topoCol=-1;setup.focar(setup.a,setup.b);acoes++;
        assert(focoTopo&&topoCol==G_TOPO_ADDONS&&!globaisReview);
        /* Stale setup targets cannot focus Add-ons after channels load,
           during a failed provider, while loading, or beneath a drawer. */
        for(int caso=0;caso<5;caso++) {
          nCanais=caso==0;falhas=caso==1;xtFalha=caso==2;
          estado=caso==3?G_BAIXANDO:G_FALHOU;painel=caso==4;
          focoTopo=0;topoCol=-1;setup.focar(setup.a,setup.b);
          assert(!focoTopo&&topoCol==-1&&!globaisReview);
        }
        nCanais=falhas=xtFalha=painel=0;estado=G_FALHOU;
      }
      if(toqueVazio.maximo>0) {
        float before=rolVazio;
        float passo=fminf(13.75f,toqueVazio.maximo*.37f);
        int foc=focoTopo,col=topoCol;
        PonteiroRolagem e={PONT_ROL_INICIO,1,0,0,(G_AREA_X+30)*escalaGuiaTeste,
                          (recorteGuia.y+20)*escalaGuiaTeste};
        assert(toqueVazioRolar(&e));
        e.fase=PONT_ROL_MOVER;e.delta=-passo*escalaGuiaTeste;
        assert(toqueVazioRolar(&e));perto(rolVazio,before+passo);gestos++;
        assert(focoTopo==foc&&topoCol==col&&!pediuCanal&&!globaisReview);
        e.fase=PONT_ROL_SOLTAR;assert(toqueVazioRolar(&e));
        guiaTesteLimpar();desenharVazioTelefone(l?longo:curto,configurar,1);
        perto(rolVazio,before+passo);vazioBounds();quadros++;
        e.fase=PONT_ROL_INERCIA;e.delta=-1e6f;toqueVazioRolar(&e);
        perto(rolVazio,toqueVazio.maximo);assert(!toqueVazioRolar(&e));
        guiaTesteLimpar();desenharVazioTelefone(l?longo:curto,configurar,1);vazioBounds();quadros++;
        if(configurar) assert(nAlvosGuia==1);
        GfxRect dernier=desenhosGuia[nDesenhosGuia-1].r;
        assert(dernier.y+dernier.h<=NV_TELA_H-80+.01f);
        e.fase=PONT_ROL_FIM;assert(toqueVazioRolar(&e));
        painel=1;assert(!toqueVazioRolar(&e));painel=0;
      }
      rolVazio=0;toquerol_limpar(&toqueVazio);
    }
  }
  testMobile=0;nv_layout_w=1920;nv_layout_h=1080;rotulosLongosGuiaTeste=0;escalaGuiaTeste=1;
  guiaTesteLimpar();desenharDuasPortas(G_AREA_X,196,1);
  assert(!telefoneui_ativo()&&!guiaVazioTelefone());
  assert(nCoresGuia>0);perto(formasGuia[0].x,G_AREA_X);perto(formasGuia[0].y,196);
  perto(formasGuia[0].w,470);perto(formasGuia[0].h,152);
  printf("guia empty phone: %d body frames, %d fractional swipes, %d existing Add-ons selections, translated header separation/status/stale guards/TV diagram OK\n",quadros,gestos,acoes);
}
int main(void) {
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  for(int i=0;i<16;i++)snprintf(basesReview[i],sizeof basesReview[i],"https://example.invalid/addon%d",i);
  int pages=0,actions=0;
  for(int d=0;d<4;d++) {
    testMobile=1;nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];guiaTesteCanais();
    nCats=16;for(int i=0;i<16;i++){catIni[i]=i;catN[i]=1;snprintf(cats[i],sizeof cats[i],"Categoria bastante comprida%d",i);}
    catAberto=1;catAnim=1;catFoco=catRol=0;
    guiaTesteLimpar();desenharPainelCategorias(1);bounds();pages++;
    if(toqueCat.maximo>0)drag(toqueCategoriaRolar,100,G_CAT_TOPO+50,&catRol);
    guiaTesteLimpar();desenharPainelCategorias(1);bounds();pages++;
    PonteiroAlvo last=alvosGuia[nAlvosGuia-1];assert(last.focar==ponteiroCategoria);
    last.focar(last.a,last.b);assert(catFoco==last.a);
    catAberto=0;painel=1;paN=16;nRec=4;paRol=paFoco=0;
    for(int i=0;i<16;i++)paIdx[i]=i;
    for(int i=0;i<4;i++){
      snprintf(rec[i].nome,sizeof rec[i].nome,"Addon recomendado de nome comprido numero%d",i);
      snprintf(rec[i].url,sizeof rec[i].url,"https://example.invalid/recommended%d/manifest.json",i);
      strcpy(rec[i].desc,"A descricao de um addon recomendado bastante longa para ocupar as duas linhas reservadas.");
    }
    guiaTesteLimpar();desenharPainelAddons(1);bounds();pages++;
    drag(toqueAddonRolar,G_PA_X+100,G_PA_LISTA_Y+50,&paRol);
    guiaTesteLimpar();desenharPainelAddons(1);bounds();pages++;
    last=alvosGuia[nAlvosGuia-1];assert(last.focar==ponteiroAddon&&last.a==painelN()-1);
    last.focar(last.a,last.b);assert(paFoco==last.a);
    /* Every installed row exposes its own local-visibility target. A click on
     * that button must win over the row's global-toggle Return target. */
    fioVivo=1;ocultosPerfil=1;memset(&ocultos,0,sizeof ocultos);
    for(int i=0;i<paN;i++) {
      paRol=fminf(toqueAddon.maximo,paItemY(i));toqueAddon.livre=1;
      guiaTesteLimpar();desenharPainelAddons(1);bounds();
      int found=-1;
      for(int k=0;k<nAlvosGuia;k++)if(alvosGuia[k].ativar==ponteiroAddonGuia&&alvosGuia[k].a==i&&alvosGuia[k].b==i)found=k;
      assert(found>=0);
      PonteiroAlvo local=alvosGuia[found];float off=paRol;int f=paFoco;
      int hit=ponteiro_achar(alvosGuia,nAlvosGuia,local.x+local.w*.5f,local.y+local.h*.5f);
      assert(hit==found&&local.focar==NULL);
      int g=gravacoesReview;local.ativar(local.a,local.b);actions++;
      assert(guiaaddons_oculto(&ocultos,basesReview[i])&&gravacoesReview==g+1&&!globaisReview);
      assert(paFoco==f&&paRol==off&&toqueAddon.livre);
      local.ativar(local.a,local.b);actions++;
      assert(!guiaaddons_oculto(&ocultos,basesReview[i])&&gravacoesReview==g+2&&!globaisReview&&recarregarPend);
      paIdx[i]=(i+1)%16;g=gravacoesReview;local.ativar(local.a,local.b);assert(gravacoesReview==g);
      paIdx[i]=i;painel=0;local.ativar(local.a,local.b);assert(gravacoesReview==g);painel=1;
      assert(paFoco==f&&paRol==off&&toqueAddon.livre&&!globaisReview);
    }
  }
  testMobile=0;nv_layout_w=1920;nv_layout_h=1080;guiaTesteLimpar();desenharPainelAddons(1);
  assert(G_PA_ROW==92);
  for(int k=0;k<nAlvosGuia;k++)assert(alvosGuia[k].ativar!=ponteiroAddonGuia);
  int g=gravacoesReview;ponteiroAddonGuia(0,0);assert(gravacoesReview==g);
  printf("guia_drawers_review: %d category/addon pages, %d local actions, scroll/clip/hit priority/stale guards/global state/tablet OK\n",pages,actions);
  vazioTelefoneReview();
}
