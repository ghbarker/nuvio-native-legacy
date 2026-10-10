/* Draw the real Agenda list/calendar with controlled data and font/GL boundaries.
 * These checks cover visible target clipping, measured text and direct finger
 * deltas. The native snapshot fixture separately checks the rendered pixels. */
#ifndef AGENDA_TV_REVIEW
#define NV_TOUCH_PREVIEW 1
#endif
#define SDL_MAIN_HANDLED 1
#include "../src/agendaui.c"
#include <assert.h>

#ifdef NV_TOUCH_PREVIEW
float nv_layout_w = 1080, nv_layout_h = 2340;
#endif
float gfx_tex_aspect_atual, gfx_card_forcar_cover_atual;
int txt_pendentes, anim_politica_reduzida;
static float escalaTeste = 1, uiTeste = 1;
static int vidroTeste, nItens = 24, nAlvos, nTextos, nArtes;
static GfxRect clipTeste, alvosTeste[256], textosTeste[512], artesTeste[64];
static PonteiroFn focosTeste[256];
static int aTeste[256], bTeste[256], clipLigado;
static AgItem itens[AG_MAX];
static struct { char texto[512]; int s; } linhasTeste[4096];
static int nLinhasTeste, cortesRotulo;
typedef struct { GfxRect r; int tipo; float raio, param, cr, cg, cb, a; } Pintura;
static Pintura pinturasTeste[4096];
static int nPinturas, paresSeletor;
static void pintura(GfxRect r,int tipo,float raio,float param,float cr,float cg,float cb,float a) {
  if(a<=0)return;assert(nPinturas<4096);
  pinturasTeste[nPinturas++]=(Pintura){r,tipo,raio,param,cr,cg,cb,a};
}

static float larguraTela(void) { return NV_TELA_W; }
static float alturaTela(void) { return NV_TELA_H; }
static void perto(float a, float b) { if(fabsf(a-b)>=.04f)fprintf(stderr,"measured %.3f, expected %.3f\n",a,b);assert(fabsf(a-b) < .04f); }
static void conferir(GfxRect r, int recortar) {
  assert(isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h));
  assert(r.w >= 0 && r.h >= 0);
  if (recortar && clipLigado) {
    float fim = fminf(r.x+r.w, clipTeste.x+clipTeste.w), base = fminf(r.y+r.h, clipTeste.y+clipTeste.h);
    r.x = fmaxf(r.x, clipTeste.x); r.y = fmaxf(r.y, clipTeste.y);
    if (fim <= r.x || base <= r.y) return;
    r.w = fim-r.x; r.h = base-r.y;
  }
  assert(r.x >= -.04f && r.x+r.w <= larguraTela()+.04f);
  assert(r.y >= -.04f && r.y+r.h <= alturaTela()+.04f);
}
float gfx_escala(void) { return escalaTeste; }
float gfx_escala_ui(void) { return uiTeste; }
void gfx_escala_sair(float s) { escalaTeste = s; }
float gfx_escala_entrar(void) { float ant=escalaTeste; escalaTeste=uiTeste; return ant; }
void gfx_recorte(float x,float y,float w,float h) { clipTeste=(GfxRect){x,y,w,h}; conferir(clipTeste,0); clipLigado=1; }
void gfx_sem_recorte(void) { clipLigado=0; }
void gfx_cor(GfxRect r,float raio,float cr,float cg,float cb,float a) {
  pintura(r,1,raio,0,cr,cg,cb,a);if(a>0)conferir(r,1);
}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float px,float py,float raio,float cr,float cg,float cb,float a) {
  (void)f;(void)px;(void)py;
  if(m!=GFX_SOMBRA)gfx_cor(r,raio,cr,cg,cb,a);
  if(m==GFX_TEXTO && a>0 && t && nTextos<512)textosTeste[nTextos++]=r;
  if(m==GFX_CARD && a>0 && nArtes<64)artesTeste[nArtes++]=r;
}
void gfx_icone(GfxRect r,const char *s,float cr,float cg,float cb,float a) { (void)s;gfx_cor(r,0,cr,cg,cb,a); }
void gfx_anel(GfxRect r,float raio,float esp,float cr,float cg,float cb,float a) { pintura(r,4,raio,esp,cr,cg,cb,a);gfx_cor(r,raio,cr,cg,cb,a); }
void gfx_vidro_folha(GfxRect r,float raio,float a) { gfx_cor(r,raio,1,1,1,a); }
void gfx_vidro_painel(GfxRect r,float raio,float f,float a) { pintura(r,2,raio,f,0,0,0,a);gfx_cor(r,raio,1,1,1,a); }
void gfx_vidro_foco(GfxRect r,float raio,float f,float a) { pintura(r,3,raio,f,0,0,0,a);gfx_cor(r,raio,1,1,1,a); }
void gfx_sombra_sob(GfxRect r,float f,float px,float raio,float cr,float cg,float cb,float a,GfxRect p,float pr,float pa) {
  (void)r;(void)f;(void)px;(void)raio;(void)cr;(void)cg;(void)cb;(void)a;(void)p;(void)pr;(void)pa;
}
void gfx_luz_canto(GfxRect r,float raio,float x,float y,float tam,float cr,float cg,float cb,float a) {
  (void)r;(void)raio;(void)x;(void)y;(void)tam;(void)cr;(void)cg;(void)cb;(void)a;
}
GLuint gfx_desfocado(GLuint t,const char *s) { (void)s;return t; }
GLuint tex_obter_larg(const char *s,float w) { (void)w;return s&&s[0]?4000:0; }
float tex_aspecto(const char *s) { (void)s;return 16.f/9; }
static int fonteTeste(TxtEstilo e) {
  switch(e) {
    case TXT_TITULO1:return 76; case TXT_TITULO2:return 57;case TXT_TITULO3:return 48;
    case TXT_ILHA_TITULO:return 40;case TXT_G30B:case TXT_G30M:return 30;
    case TXT_G28R:case TXT_G28B:return 28;case TXT_G20M:return 20;
    case TXT_ILHA_NOME:case TXT_HERO_SEC:return 24;case TXT_ILHA_APOIO:return 16;
    case TXT_MINI:case TXT_ILHA_HORA:return 15;case TXT_CAPTION2:return 17;
    case TXT_AJ_SEG:case TXT_AJ_SUB:return 20;case TXT_AJ_CAPS13:return 14;
    case TXT_AJ_ESTADO:return 18;default:return 28;
  }
}
int txt_largura(TxtEstilo e,const char *s) {
  /* InterDisplay-Medium at 30 px: retain its exact-fit boundary rather than
   * hiding the native List truncation behind an arbitrary approximate width. */
  if(e==TXT_G30M && !strcmp(s,"Lista"))return 64;
  if(e==TXT_G30M && !strcmp(s,"Mês"))return 57;
  return (int)(strlen(s)*fonteTeste(e)*.52f);
}
size_t txt_token_tam(const char *s) { size_t n=0;while(s[n]&&s[n]!=' '&&s[n]!='\n')n++;return n; }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)r;(void)g;(void)b;(void)a;assert(nLinhasTeste<4095);
  int t=++nLinhasTeste;snprintf(linhasTeste[t].texto,sizeof linhasTeste[t].texto,"%s",s);
  linhasTeste[t].s=fonteTeste(e);return (TxtLinha){t,txt_largura(e,s),(int)(fonteTeste(e)*1.25f)};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) {
  assert(w>0);TxtLinha l=txt_linha(e,s,r,g,b,a);
  if(l.w>w) {
    if(e==TXT_G30M && (!strcmp(s,"Lista")||!strcmp(s,"Mês")))cortesRotulo++;
    l.w=(int)w;
  }
  return l;
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) { gfx_rect((GfxRect){x,y,l.w,l.h},l.tex,GFX_TEXTO,0,0,0,0,1,1,1,a); }
float txt_tracking(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float a,float esp) {
  (void)esp;TxtLinha t=txt_linha(e,s,r,g,b,255);txt_desenhar_alpha(t,x,y,a);return t.w;
}
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn at,int a,int b) {
  (void)at;GfxRect r={x,y,w,h};conferir(r,0);if(w<=0||h<=0)return;assert(nAlvos<256);
  alvosTeste[nAlvos]=r;focosTeste[nAlvos]=f;aTeste[nAlvos]=a;bTeste[nAlvos++]=b;
}
void ponteiro_alvo_faixa(float x,float y,float w,float h,float top,float base,PonteiroFn f,PonteiroFn at,int a,int b) {
  float fim=fminf(y+h,base);y=fmaxf(y,top);if(fim>y)ponteiro_alvo(x,y,w,fim-y,f,at,a,b);
}
void ponteiro_rolagem(PonteiroRolagemFn f) { (void)f; }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int m,const char *s) { (void)m;return s; }
int ajustes_idioma(void) { return 0; }
int ajustes_vidro(void) { return vidroTeste; }
float ajustes_rail_largura_fixa(void) { return 0; }
void ajustes_acento(float *r,float *g,float *b) { *r=.4f;*g=.7f;*b=1; }
int agenda_n(void) { return nItens; }
const AgItem *agenda_lista(int i) { return i>=0&&i<nItens?itens+i:NULL; }
const char *agenda_hoje(void) { return "2026-09-16"; }
int agenda_ano(const char *s) { return atoi(s); }
int agenda_mes(const char *s) { return strlen(s)>6?atoi(s+5):0; }
int agenda_dia(const char *s) { return strlen(s)>9?atoi(s+8):0; }
int agenda_dias(const char *s) { return s[0]?agenda_dia(s)-16:-1; }
int agenda_semana(const char *s) { return (agenda_dia(s)+1)%7; }
const char *agenda_semana_nome(int d) { static const char *v[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};return v[d%7]; }
const char *agenda_mes_nome(int m) { (void)m;return "September"; }
void agenda_falta(const char *s,char *d,size_t n) { (void)s;snprintf(d,n,"in 7 weeks"); }
int agenda_pode_lembrar(const char *s) { (void)s;return 1; }
int agenda_atualizando(void) { return 0; }
void agenda_quando(const char *s,char *d,size_t n) { snprintf(d,n,"%s",s); }
int noticias_n(const char *s) { (void)s;return 0; }
#ifdef NV_TOUCH_PREVIEW
static void zerarDesenho(void) {
  nAlvos=nTextos=nArtes=nLinhasTeste=cortesRotulo=nPinturas=0;clipLigado=0;
}
static int pinturasDoAlvo(GfxRect alvo,Pintura dst[8]) {
  int n=0;
  for(int i=0;i<nPinturas;i++) {
    GfxRect r=pinturasTeste[i].r;
    if(fabsf(r.x-alvo.x)<.03f&&fabsf(r.y-alvo.y)<.03f&&
       fabsf(r.w-alvo.w)<.03f&&fabsf(r.h-alvo.h)<.03f) {
      assert(n<8);dst[n++]=pinturasTeste[i];
    }
  }
  return n;
}
static void testarSeletor(const AgC1 *L) {
  for(int focado=0;focado<3;focado++) {
    Pintura desenho[2][2][8];int cont[2][2];GfxRect alvos[2][2];
    for(int vista=0;vista<2;vista++) {
      zerarDesenho();vistaMes=vista;
      int ativo=vista?AG_CAB_MES:AG_CAB_LISTA, inativo=vista?AG_CAB_LISTA:AG_CAB_MES;
      focoCabecalho=focado==1?ativo:focado==2?inativo:0;
      calAno=2026;calMes=9;
      if(vista)desenhaBarraCalendario(agX(),agFim());else cabecalhoIlha(L);
      assert(nAlvos==(vista?5:2) && cortesRotulo==0);
      for(int k=0;k<2;k++) {
        assert(focosTeste[k]==ponteiroCab&&aTeste[k]==k+1&&bTeste[k]==0);
        alvos[vista][k]=alvosTeste[k];
        perto(alvosTeste[k].w*escalaTeste,180*uiTeste);
        perto(alvosTeste[k].h*escalaTeste,112*uiTeste);
        cont[vista][k]=pinturasDoAlvo(alvosTeste[k],desenho[vista][k]);
        assert(cont[vista][k]>0 && desenho[vista][k][0].tipo==(vidroTeste?2:1));
        perto(desenho[vista][k][0].raio,.5f);
      }
      perto(alvosTeste[0].y,alvosTeste[1].y);
      perto((alvosTeste[1].x-alvosTeste[0].x-alvosTeste[0].w)*escalaTeste,12*uiTeste);
    }
    /* Compare actual paint primitives by active/inactive role, including the
     * glass strength, focus outline, radius, colors and opacity. */
    for(int k=0;k<2;k++) {
      int par=1-k;assert(cont[0][k]==cont[1][par]);
      perto(alvos[0][k].w,alvos[1][par].w);perto(alvos[0][k].h,alvos[1][par].h);
      for(int i=0;i<cont[0][k];i++) {
        Pintura a=desenho[0][k][i],b=desenho[1][par][i];assert(a.tipo==b.tipo);
        perto(a.raio,b.raio);perto(a.param,b.param);perto(a.cr,b.cr);
        perto(a.cg,b.cg);perto(a.cb,b.cb);perto(a.a,b.a);
      }
    }
    paresSeletor++;
  }
  vistaMes=focoCabecalho=0;zerarDesenho();
}
#endif
int main(void) {
  for(int i=0;i<AG_MAX;i++) {
    snprintf(itens[i].imdb,sizeof itens[i].imdb,"tt%08d",i);
    snprintf(itens[i].titulo,sizeof itens[i].titulo,"A very long followed television series title number %d",i);
    snprintf(itens[i].nomeEp,sizeof itens[i].nomeEp,"A very long premiere episode name number %d",i);
    strcpy(itens[i].fundo,"fixture-landscape");strcpy(itens[i].poster,"fixture-poster");
    strcpy(itens[i].rede,"A long network name");itens[i].temporada=23;itens[i].episodio=123;itens[i].lembrete=i%2;
    if(i<20)snprintf(itens[i].dataProx,sizeof itens[i].dataProx,"2026-09-%02d",16+i/6);
    else itens[i].situacao=AG_ENCERRADA;
  }
  int casos=0;
#ifdef NV_TOUCH_PREVIEW
  const float telas[][2]={{1080,2340},{2340,1080},{1080,2400},{2400,1080}};
  for(int t=0;t<4;t++)for(int z=0;z<2;z++)for(int v=0;v<2;v++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];uiTeste=z?1.5f:1;vidroTeste=v;
#else
  for(int z=0;z<2;z++)for(int v=0;v<2;v++) {
    uiTeste=z?1.5f:1;vidroTeste=v;
#endif
    escalaTeste=agEscala();ctxAberto=focoCabecalho=vistaMes=calPainel=calEvento=0;
    AgC1 L=c1();assert(L.lsH>0 && L.lsW>0);
#ifdef NV_TOUCH_PREVIEW
    assert(agColunaUnica());perto(agLinhaH()*escalaTeste,184*uiTeste);
    perto(agGrupoH()*escalaTeste,72*uiTeste);assert(L.artW==0);
    testarSeletor(&L);
#else
    assert(!agColunaUnica());perto(agLinhaH()*escalaTeste,116);perto(agGrupoH()*escalaTeste,51);
    assert(L.artW>0);perto(L.pnY*escalaTeste,112);
#endif
    for(int pos=0;pos<3;pos++) {
      nAlvos=nTextos=nArtes=nLinhasTeste=cortesRotulo=nPinturas=0;clipLigado=0;
#ifdef NV_TOUCH_PREVIEW
      toqueCalendario.offset=NULL;
#endif
      scrollY=pos==0?0:pos==1?fmaxf(0,alturaDoc()-L.lsH)*.463f:fmaxf(0,alturaDoc()-L.lsH);
      cabecalhoIlha(&L);desenhaLista(&L);assert(!clipLigado && nAlvos>2 && nTextos>3);
      assert(cortesRotulo==0);
      int primeiro=-1,ultimo=-1;
      for(int i=0;i<nAlvos;i++)if(focosTeste[i]==ponteiroLinha) {
        if(primeiro<0)primeiro=aTeste[i];ultimo=aTeste[i];
        assert(alvosTeste[i].y>=L.lsY-.04f && alvosTeste[i].y+alvosTeste[i].h<=L.lsY+L.lsH+.04f);
      }
      assert(primeiro>=0);if(pos==2)assert(ultimo==nItens-1);
#ifdef NV_TOUCH_PREVIEW
      GfxRect primeira=alvosTeste[0];perto(primeira.h*escalaTeste,112*uiTeste);
      float antes=scrollY;
      PonteiroRolagem e={PONT_ROL_INICIO,1,0,0,(toqueAgenda.regiao.x+4)*escalaTeste,(toqueAgenda.regiao.y+4)*escalaTeste};
      assert(toqueAgendaRolar(&e));e.fase=PONT_ROL_MOVER;e.delta=-31.25f;
      toqueAgendaRolar(&e);perto(scrollY,fminf(antes+31.25f/escalaTeste,toqueAgenda.maximo));
      e.fase=PONT_ROL_SOLTAR;assert(toqueAgendaRolar(&e));e.fase=PONT_ROL_FIM;assert(toqueAgendaRolar(&e));
      assert(toqueAgenda.livre);toquerol_limpar(&toqueAgenda);
      /* The renderer's actual title and two metadata rows stay inside a full card. */
      nTextos=nArtes=nLinhasTeste=nPinturas=0;gfx_recorte(L.pnX,L.lsY,L.pnW,L.lsH);
      linhaC1(&L,itens,L.lsY,1);assert(nTextos>=2 && nArtes==1);
      perto(artesTeste[0].w*escalaTeste,200*uiTeste);perto(artesTeste[0].h*escalaTeste,112*uiTeste);
      perto(textosTeste[0].h*escalaTeste,60*42.f/48*uiTeste);
      for(int i=0;i<nTextos;i++)assert(textosTeste[i].y>=L.lsY-.6f && textosTeste[i].y+textosTeste[i].h<=L.lsY+agLinhaH()+.6f);
      gfx_sem_recorte();
#endif
      casos++;
    }
    vistaMes=1;calAno=2026;calMes=9;calDia=16;calCelula=17;calPainel=calEvento=0;
#ifdef NV_TOUCH_PREVIEW
    toqueCalOffset=0;toquerol_limpar(&toqueCalendario);
#endif
    nAlvos=nTextos=nArtes=nLinhasTeste=cortesRotulo=nPinturas=0;desenhaBarraCalendario(agX(),agFim());assert(nAlvos==5);
    assert(cortesRotulo==0);
    for(int i=0;i<5;i++)for(int j=0;j<i;j++) {
      GfxRect a=alvosTeste[i],b=alvosTeste[j];
      assert(a.x+a.w<=b.x+.03f||b.x+b.w<=a.x+.03f||a.y+a.h<=b.y+.03f||b.y+b.h<=a.y+.03f);
    }
    for(int pos=0;pos<3;pos++) {
      nAlvos=nTextos=nArtes=nLinhasTeste=nPinturas=0;clipLigado=0;
#ifdef NV_TOUCH_PREVIEW
      toqueAgenda.offset=NULL;
      if(pos) { toqueCalendario.livre=1;toqueCalOffset=pos==1?toqueCalendario.maximo*.463f:toqueCalendario.maximo; }
#endif
      desenhaCalendarioMensal();assert(!clipLigado && nTextos>0);
#ifdef NV_TOUCH_PREVIEW
      assert(toqueCalendario.regiao.h>0);perto(agMesMedir().celH*escalaTeste,128*uiTeste);
      for(int i=0;i<nAlvos;i++)assert(alvosTeste[i].y>=toqueCalendario.regiao.y-.03f &&
          alvosTeste[i].y+alvosTeste[i].h<=toqueCalendario.regiao.y+toqueCalendario.regiao.h+.03f);
      if(pos==2) {
        int viu=-1;for(int i=0;i<nAlvos;i++)if(focosTeste[i]==ponteiroEvento)viu=aTeste[i];assert(viu==5);
      }
#endif
      casos++;
    }
  }
  printf("PASS: %d real Agenda list/calendar layouts, clipped actions, readable phone rows and direct finger deltas.\n",casos);
#ifdef NV_TOUCH_PREVIEW
  assert(paresSeletor==48);printf("PASS: %d actual phone selector paint/target pairs match List and Month.\n",paresSeletor);
#endif
  return 0;
}
