/* Actual phone sheet draw functions with font/GL boundary doubles.
 * Captures clipped targets and runs the real direct scrolling callbacks. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#ifdef _WIN32
#include <time.h>
static struct tm *telefone_localtime_r(const time_t *t, struct tm *out) {
  struct tm *p = localtime(t); if (p) *out = *p; return p ? out : NULL;
}
#define localtime_r telefone_localtime_r
#endif
#if defined(TESTE_ILHA)
#include "../src/ilha.c"
#elif defined(TESTE_POSPLAY)
#include "../src/posplay.c"
#elif defined(TESTE_ARTE)
#include "../src/trocaarte.c"
#include "plrui.h"
#elif defined(TESTE_DIAG)
#include "../src/diagnostico.c"
#include "../src/vazao.c"
#include "../src/perfiltv.c"
#else
#error choose a secondary sheet
#endif
#include <assert.h>
float nv_layout_w = 1080, nv_layout_h = 2340;
float gfx_tex_aspect_atual, gfx_card_forcar_cover_atual;
int txt_pendentes, anim_politica_reduzida;
static float escala = 1, zoom = 1;
static GfxRect recorte, botoesTeste[32];
static int alvoA[32], alvoB[32];
#if defined(TESTE_ARTE)
static PonteiroFn alvoFocar[32], alvoAtivar[32];
#endif
static int cortando, nBotoesTeste, ultimoPoster, nTextos;
static float limiteTexto;
static int tintaFocoTeste, corTextoR, corTextoG, corTextoB;
static float telaW(void) { return nv_layout_w / escala; }
static float telaH(void) { return nv_layout_h / escala; }
static void limites(GfxRect r, int clipping) {
  assert(r.w >= 0 && r.h >= 0 && isfinite(r.x) && isfinite(r.y));
  if (clipping && cortando) {
    float dir = fminf(r.x + r.w, recorte.x + recorte.w), esq = fmaxf(r.x, recorte.x);
    if (dir <= esq || r.y + r.h <= recorte.y || r.y >= recorte.y + recorte.h) return;
    r.x = esq; r.w = dir - esq;
  }
  if (r.x < -.01f || r.x + r.w > telaW() + .01f) {
    fprintf(stderr, "outside %.1f: %.1f %.1f %.1f %.1f\n", telaW(), r.x, r.y, r.w, r.h); assert(0);
  }
}
float gfx_escala(void) { return escala; }
float gfx_escala_ui(void) { return zoom; }
float gfx_escala_entrar(void) { float antes = escala; escala = zoom; return antes; }
void gfx_escala_sair(float s) { escala = s; }
void gfx_recorte(float x, float y, float w, float h) { recorte = (GfxRect){x,y,w,h}; cortando = 1; limites(recorte, 0); }
void gfx_sem_recorte(void) { cortando = 0; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float a) { (void)raio; (void)cr; (void)cg; (void)cb; if (a > 0) limites(r, 1); }
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float f, float px, float py, float raio, float cr, float cg, float cb, float a) {
  (void)t; (void)f; (void)px; (void)py; if (m != GFX_SOMBRA) gfx_cor(r, raio, cr, cg, cb, a);
}
void gfx_icone(GfxRect r, const char *s, float cr, float cg, float cb, float a) { (void)s; gfx_cor(r, 0, cr, cg, cb, a); }
void gfx_anel(GfxRect r, float raio, float esp, float cr, float cg, float cb, float a) { (void)esp; gfx_cor(r, raio, cr, cg, cb, a); }
int txt_largura(TxtEstilo e, const char *s) { (void)e; return (int)strlen(s) * 12; }
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) { corTextoR=r;corTextoG=g;corTextoB=b;(void)a; return (TxtLinha){0,txt_largura(e,s),28}; }
TxtLinha txt_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float w) {
  assert(w > 0); TxtLinha t = txt_linha(e,s,r,g,b,a); if(t.w>w)t.w=(int)w; return t;
}
void txt_desenhar_alpha(TxtLinha t, float x, float y, float a) {
  if (a <= 0) return;
  limites((GfxRect){x,y,t.w,t.h},1); nTextos++;
  if (y + t.h > limiteTexto) limiteTexto = y + t.h;
}
void txt_desenhar(TxtLinha t, float x, float y) { txt_desenhar_alpha(t,x,y,1); }
float txt_bloco_corta(TxtEstilo e, const char *s, int r, int g, int b, float x, float y, float w, float lead, float a, int max) {
  assert(w > 0); int n = (int)ceilf(txt_largura(e,s)/w); if (max > 0 && n > max) n=max;
  if(a>0&&n)txt_desenhar_alpha((TxtLinha){0,(int)w,(int)(n*lead)},x,y,a);
  (void)r;(void)g;(void)b;return n*lead;
}
float txt_bloco(TxtEstilo e, const char *s, int r, int g, int b, float x, float y, float w, float lead, float a, int max) { return txt_bloco_corta(e,s,r,g,b,x,y,w,lead,a,max); }
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 5000; }
int ajustes_idioma(void) { return 0; }
int ajustes_idioma_ingles(void) { return 1; }
int ajustes_vidro(void) { return 0; }
int ajustes_tinta_foco(void) { return tintaFocoTeste; }
float ajustes_acento_tinta(float *r,float *g,float *b) { if(r)*r=.4f;if(g)*g=.7f;if(b)*b=1;return 0; }
int ajustes_relogio_12h(void) { return 0; }
float txt_tracking(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float spacing,float a) {
  TxtLinha t=txt_linha(e,s,r,g,b,255); (void)spacing;txt_desenhar_alpha(t,x,y,a);return t.w;
}
void ajustes_acento(float *r,float *g,float *b) { *r=.4f;*g=.7f;*b=1; }
GLuint tex_obter_larg(const char *s, float w) { (void)s;(void)w;return 0; }
GLuint tex_obter_larg_qualquer(const char *s, float w) { return tex_obter_larg(s,w); }
int tex_marca_escura(const char *s) { (void)s;return 0; }
int tex_falhou(const char *s) { (void)s;return 0; }
float tex_aspecto(const char *s) { (void)s;return 16.0f/9; }
void ponteiro_camada(void) { nBotoesTeste=0; }
#if defined(__GNUC__)
__attribute__((always_inline))
#endif
inline void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn focar,PonteiroFn ativar,int a,int b) {
  limites((GfxRect){x,y,w,h},0);
  if (w>0 && h>0) {
    assert(y>=0&&y+h<=telaH()+.01f);
    if(nBotoesTeste<32) {
      botoesTeste[nBotoesTeste]=(GfxRect){x,y,w,h};alvoA[nBotoesTeste]=a;alvoB[nBotoesTeste]=b;
#if defined(TESTE_ARTE)
      alvoFocar[nBotoesTeste]=focar;alvoAtivar[nBotoesTeste]=ativar;
#endif
      nBotoesTeste++;
    }
  }
  (void)a;(void)b;(void)focar;(void)ativar;
}
#if defined(__GNUC__)
__attribute__((always_inline))
#endif
inline void ponteiro_alvo_faixa(float x,float y,float w,float h,float y0,float y1,PonteiroFn f,PonteiroFn at,int a,int b) {
  float base=fminf(y+h,y1); y=fmaxf(y,y0); if(base>y)ponteiro_alvo(x,y,w,base-y,f,at,a,b);
}
void ponteiro_rolagem(PonteiroRolagemFn f) { (void)f; }
float plrui_botao_largura(const char *r,const char *i) { return txt_largura(TXT_G21B,r)+40+(i?32:0); }
float plrui_botao(float x,float y,const char *r,const char *i,float f,float a) { (void)f;float w=plrui_botao_largura(r,i);gfx_cor((GfxRect){x,y,w,60},0,1,1,1,a);return w; }
void plrui_botao_repouso(GfxRect r,float a) { gfx_cor(r,0,1,1,1,a); }
void plrui_pilula_foco(GfxRect r,float a) { gfx_cor(r,0,1,1,1,a); }
void plrui_material(GfxRect r,float raio,int modal,float a) { (void)modal; gfx_cor(r,raio,1,1,1,a); }
void plrui_trilho(GfxRect r,float f,float cr,float cg,float cb,float a) { (void)f; gfx_cor(r,0,cr,cg,cb,a); }
float plrui_kicker(const char *s,float x,float y,int r,int g,int b,float a) { TxtLinha t=txt_linha(TXT_MINI,s,r,g,b,255);if(x>=0)txt_desenhar_alpha(t,x,y,a);return t.w; }
float plrui_dicas(const char *const *k,const char *const *r,int n,float x,float y,int dir,float a) { (void)k;(void)r;(void)n;(void)x;(void)y;(void)dir;(void)a;return 0; }
void gfx_esqueleto(GfxRect r,float raio,float cr,float cg,float cb,float a) { gfx_cor(r,raio,cr,cg,cb,a); }
void botao_luz(GfxRect r,float f,float a) { (void)r;(void)f;(void)a; }
static void arrastar(ToqueRolagem *r,PonteiroRolagemFn fn,int eixo) {
  PonteiroRolagem e={PONT_ROL_INICIO,eixo,0,0,(r->regiao.x+10)*r->escala,(r->regiao.y+10)*r->escala};
  assert(fn(&e));e.fase=PONT_ROL_MOVER;e.delta=-100000;assert(fn(&e));
  e.fase=PONT_ROL_SOLTAR;assert(fn(&e)); e.fase=PONT_ROL_INERCIA; assert(!fn(&e));
  assert(*r->offset==r->maximo);e.fase=PONT_ROL_CANCELAR;assert(fn(&e));
}
#if defined(TESTE_ILHA)
const char *rec_genero_rotulo(int g) { (void)g;return "Drama"; }
void rec_avatar_estilo(GfxRect r,const char *u,const char *n,const char *id,float a,int estilo) { (void)u;(void)n;(void)id;(void)estilo;gfx_cor(r,0,1,1,1,a); }
static void rodar(void) {
  modalAberto=modalAviso=1;modalAtividade=0;memset(&modalM,0,sizeof modalM); modalM.nBotoes=3;modalM.salvos=1;
  memset(modalM.texto,'a',sizeof modalM.texto-1);
  memset(modalM.fala,'b',sizeof modalM.fala-1);strcpy(modalM.arte,"fixture-landscape");
  for(int i=0;i<3;i++)memset(modalM.lista[i],'c',sizeof modalM.lista[i]-1);
  strcpy(modalM.titulo,"A very long recommendation title with several words to wrap");
  for(int i=0;i<3;i++)strcpy(modalM.botao[i],"A translated action with long wording");
  modalAvisoH=2000;modalToqueReiniciar();
  GfxRect m=modalAlvo((GfxRect){10,0,300,64},1);
  assert(m.x>=40&&m.x+m.w<=NV_TELA_W-40&&m.y+m.h<=NV_TELA_H-40);
  m.h=fminf(m.h,640); /* the actual short landscape height, also exercised in portrait */
  layoutModalAviso(m,1,1);assert(nBotoesTeste==6); /* 2 barriers + 3 buttons + Salvos */
  float antes=toqueModalY;arrastar(&toqueModal,modalToqueRolar,1);assert(toqueModalY>antes);
  layoutModalAviso(m,1,1);assert(nBotoesTeste==6);
  /* A short new notice resets the old body's position through its real opener. */
  temCur=1;curAte=6000;memset(&cur,0,sizeof cur);cur.temModal=1;cur.modal.nBotoes=1;strcpy(cur.modal.botao[0],"OK");modalAberto=0;
  assert(abrirDoAviso());assert(toqueModalY==0&&!toqueModal.livre);
}
#elif defined(TESTE_POSPLAY)
static CatItem ci;
int cat_indice_vivo(int i,const char *id) { (void)id;return i; }
const CatItem *cat_item(int i) { (void)i;return &ci; }
int cat_n_episodios(int i) { (void)i;return 0; }
const CatEp *cat_episodio(int i,int e) { (void)i;(void)e;return NULL; }
int extras_n_relacionados(void) { return 8; }
const char *extras_relacionado_poster(int i) { ultimoPoster=i;return ""; }
const char *extras_relacionado_titulo(int i) { (void)i;return "An intentionally long film name that would extend past the right edge"; }
const char *extras_relacionado_ano(int i) { (void)i;return "2026"; }
static void rodar(void) {
  visivel=1;serie=0;idx=0;anim=1;foco=0;scrollPP=0;memset(&toquePP,0,sizeof toquePP);
  posplay_desenhar(5000,0);assert(ultimoPoster==7);
  int fo=foco,pedido=pedTitulo;if(toquePP.maximo>0)arrastar(&toquePP,toquePPRolar,0);assert(foco==fo&&pedTitulo==pedido);
  nBotoesTeste=0;ultimoPoster=-1;posplay_desenhar(5000,0);assert(ultimoPoster==7);
  assert(nBotoesTeste>0);float fim=0;for(int i=0;i<nBotoesTeste;i++)fim=fmaxf(fim,botoesTeste[i].x+botoesTeste[i].w);
  assert(fim<=NV_TELA_W);
  int ultimo=0;for(int i=0;i<nBotoesTeste;i++)if(alvoA[i]==7)ultimo=1;assert(ultimo);
  foco=7;toquerol_limpar(&toquePP);posplay_desenhar(5000,0);assert(ultimoPoster==7);if(toquePP.maximo>0)assert(scrollPP>0);
}
#elif defined(TESTE_DIAG)
static int introGravada;
int dados_gravar(const char *nome,const char *s) { assert(!strcmp(nome,"diagnostico-otimizacao-intro.cfg")&&!strcmp(s,"versao=1\n"));introGravada++;return 1; }
const char *addons_nome(int i) { (void)i;return "A long add-on display name used for the measured diagnostic card"; }
int ajustes_hero_fonte(void) { return 0; }
void ajustes_ui_fundo(void) {}
void ajustes_ui_ilha(GfxRect r,float raio,int modal) { plrui_material(r,raio,modal,1); }
void ajustes_ui_veu(void) {}
float ajustes_ui_kicker(const char *s,float x,float y,float a) { return plrui_kicker(s,x,y,243,242,239,a); }
void ajustes_ui_neutro(GfxRect r,float raio,float a) { gfx_cor(r,raio,1,1,1,a); }
float ajustes_ui_ef(const char *ico,const char *s,float x,float y,float w) { (void)ico;return txt_bloco(TXT_CAPTION,s,243,242,239,x,y,w,30,1,0); }
float ajustes_ui_dicas(const char *const *k,const char *const *r,int n,float x,float y,int draw) { (void)k;(void)r;(void)n;(void)x;(void)y;(void)draw;return 0; }
void tex_orcamento_info(int *mb,long *ram,int *fixo,int *slots) { if(mb)*mb=96;if(ram)*ram=0;if(fixo)*fixo=0;if(slots)*slots=100; }
int tex_fios_rede(void) { return 4; }
int tex_teto_heroi_perfil(void) { return 1920; }
long tex_orcamento_bytes(void) { return 96L<<20; }
void tex_estatisticas(int *itens,int *pend,long *bytes,int *quentes,long *bytesQuentes) { if(itens)*itens=10;if(pend)*pend=3;if(bytes)*bytes=32L<<20;if(quentes)*quentes=2;if(bytesQuentes)*bytesQuentes=4L<<20; }
static void rodar(void) {
  introGlobal=1;d.intro=0;sairTela=0;int gravadas=introGravada;
  dgApresentacao(1);assert(nBotoesTeste==3);
  if(dgIntroPhone.rolagem.maximo>0){arrastar(&dgIntroPhone.rolagem,dgIntroPhoneRolar,1);dgApresentacao(1);assert(nBotoesTeste==3);}
  dgIntroPhoneAcao(1,1);assert(!introGlobal&&introGravada==gravadas);
  introGlobal=1;dgApresentacao(1);dgIntroPhoneAcao(0,1);assert(!introGlobal&&introGravada==gravadas+1);
  d.intro=1;dgApresentacao(0);assert(nBotoesTeste==3);int estado=atomic_load(&d.estado),vEstado=atomic_load(&vz.estado);
  dgIntroPhoneAcao(0,0);assert(!d.intro&&!sairTela&&atomic_load(&d.estado)==estado&&atomic_load(&vz.estado)==vEstado);
  d.intro=1;dgIntroPhoneAcao(1,0);assert(!d.intro&&sairTela);
  introGlobal=d.intro=sairTela=0;vz.aberto=0;diagPaginaEstado=-1;atomic_store(&d.estado,0);dgTelefone(5000);
  if(toqueDiagPagina.maximo>0)arrastar(&toqueDiagPagina,dgPhoneRolar,1);nBotoesTeste=0;dgTelefone(5000);assert(nBotoesTeste>=3);
  atomic_store(&d.estado,1);dgTelefone(5000);assert(diagPaginaY==0);
  atomic_store(&d.estado,2);for(int i=1;i<PTV_N_FONTES;i++){d.fonte[i].ok=10;d.fonte[i].falhas=2;}d.medDepois.artesMs=4000;dgTelefone(5000);
  arrastar(&toqueDiagPagina,dgPhoneRolar,1);nBotoesTeste=0;dgTelefone(5000);assert(nBotoesTeste>=2);
  vz.aberto=1;atomic_store(&vz.estado,2);vz.resultado=VR_OK;vz.resumo.otimoKbps=25000;vz.resumo.maximoKbps=40000;dgTelefone(5000);
  assert(diagPaginaY==0);if(toqueDiagPagina.maximo>0)arrastar(&toqueDiagPagina,dgPhoneRolar,1);nBotoesTeste=0;dgTelefone(5000);assert(nBotoesTeste>=2);
  atomic_store(&vz.estado,1);dgTelefone(5000);assert(diagPaginaY==0);
  vz.aberto=0;atomic_store(&d.estado,4);strcpy(d.erro,"The worker could not start; no result was produced");dgTelefone(5000);
  assert(diagPaginaY==0);if(toqueDiagPagina.maximo>0)arrastar(&toqueDiagPagina,dgPhoneRolar,1);nBotoesTeste=0;dgTelefone(5000);assert(nBotoesTeste>=2);
}
#elif defined(TESTE_ARTE)
const char *ling_nome(const char *iso) { (void)iso;return "A translated language filter name"; }
static void arteCorpoOculto(int a,int b) { (void)a;(void)b;assert(!"hidden Detail target must be removed"); }
static void conferirAlvosArte(int interativos) {
  int barreiras=0,controles=0;
  for(int i=0;i<nBotoesTeste;i++) {
    GfxRect r=botoesTeste[i];
    if(!alvoFocar[i]&&!alvoAtivar[i]) {
      barreiras++;
      assert(r.x==0&&r.y==0&&r.w==NV_TELA_W&&r.h==NV_TELA_H);
    } else {
      assert(alvoFocar[i]==ponteiroFoco&&!alvoAtivar[i]);
      assert(alvoA[i]==-1||alvoA[i]==-2||alvoA[i]==aba);
      controles++;
      if(alvoA[i]>=0) {
        assert(r.y>=TA_Y0&&r.y+r.h<=TA_Y0+TA_LINHAS*TA_PASSO);
      }
      assert(r.y+r.h<=TA_Y0+TA_LINHAS*TA_PASSO);
    }
  }
  assert(barreiras==1);
  if(interativos)assert(controles>TA_COLS);
  else assert(!controles&&nBotoesTeste==1);
}
static void rodar(void) {
  escala=1;aberto=1;mola=1;aba=naAba=naFiltro=0;memset(filtro,0,sizeof filtro);memset(toque,0,sizeof toque);memset(toqueY,0,sizeof toqueY);memset(topo,0,sizeof topo);
  strcpy(item.titulo,"A long artwork chooser title");
  for(int a=0;a<2;a++) {nCand[a]=40;for(int i=0;i<40;i++){memset(&cand[a][i],0,sizeof cand[a][i]);snprintf(cand[a][i].rotulo,sizeof cand[a][i].rotulo,"Candidate %d",i);strcpy(cand[a][i].iso,i%2?"en":"es");}}
  trocaarte_desenhar("");assert(TA_COLS==(nv_layout_h>nv_layout_w?2:5));
  for(int a=0;a<2;a++) {
    aba=a;foco[a]=0;naAba=0;nBotoesTeste=0;trocaarte_desenhar("");
    int anterior=foco[a];arrastar(&toque[a],toqueRolar,1);assert(foco[a]==anterior&&!mudou);
    nBotoesTeste=0;trocaarte_desenhar("");assert(nBotoesTeste>TA_COLS);
    int ultimo=0;for(int i=0;i<nBotoesTeste;i++)if(alvoFocar[i]==ponteiroFoco&&alvoA[i]==a&&alvoB[i]==39)ultimo=1;assert(ultimo);
    /* The actual last grid row gets a clipped tap target at scroll end. */
    assert(toqueY[a]==toque[a].maximo);
    conferirAlvosArte(1);
  }
  /* While entering/closing, only the inert full-canvas barrier remains.
   * The real modal layer also discards a previously published body target. */
  const float transicao[]={.014f,.23f};
  for(int i=0;i<2;i++) {
    aberto=i==0;mola=transicao[i];
    ponteiro_alvo(32,32,70,60,arteCorpoOculto,NULL,7,11);
    trocaarte_desenhar("");conferirAlvosArte(0);
  }
}
#endif
int main(void) {
  float dimensoes[][2]={{1080,1920},{1080,2340},{2340,1080}}, escalas[]={1,1.2f,1.3f,1.5f};
  for(int d=0;d<3;d++)for(int s=0;s<4;s++) {
    nv_layout_w=dimensoes[d][0];nv_layout_h=dimensoes[d][1];zoom=escalas[s];escala=zoom;nBotoesTeste=nTextos=cortando=0;limiteTexto=0;rodar();
  }
  puts("Phone secondary sheets: PASS");return 0;
}
