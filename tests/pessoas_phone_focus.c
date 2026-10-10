/* Actual People updater plus drawing, with offline empty provider data. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include "../src/pessoas.c"
#include <assert.h>
#include <math.h>
float nv_layout_w = 1080, nv_layout_h = 2340;
static float scale = 1, ui = 1;
static GfxRect clip;
static int clipped, nTargets, writes, requests, longText;
static PonteiroRolagemFn scrollFn;
static struct { GfxRect r; PonteiroFn focus, action; int a,b; } targets[32];
static float screenW(void) { return nv_layout_w / scale; }
static float screenH(void) { return nv_layout_h / scale; }
static void bounded(GfxRect r, int clipping) {
  assert(isfinite(r.x) && isfinite(r.y) && r.w >= 0 && r.h >= 0);
  if (clipping && clipped) {
    float right = fminf(r.x + r.w, clip.x + clip.w), left = fmaxf(r.x, clip.x);
    if (right <= left || r.y + r.h <= clip.y || r.y >= clip.y + clip.h) return;
    r.x = left; r.w = right - left;
  }
  if (r.x < -.01f || r.x + r.w > screenW() + .01f) {
    fprintf(stderr, "outside width %.1f: x %.1f w %.1f y %.1f\n", screenW(),r.x,r.w,r.y); assert(0);
  }
}
const char *i18n(const char *s) { return longText && (strlen(s)>40 || !strcmp(s,"Tentar de novo") || !strcmp(s,"Abrir Meu perfil")) ? "A deliberately long translated label and description to verify that header measurement and list focus use the same displayed viewport and leave the existing action reachable within the phone dialog" : s; }
float gfx_escala(void) { return scale; }
float gfx_escala_ui(void) { return ui; }
float gfx_escala_entrar(void) { float old = scale; scale = ui; return old; }
void gfx_escala_sair(float old) { scale = old; }
void gfx_recorte(float x,float y,float w,float h) { clip=(GfxRect){x,y,w,h};clipped=1;assert(w>0&&h>0);bounded(clip,0); }
void gfx_sem_recorte(void) { clipped=0; }
void gfx_cor(GfxRect r,float radius,float cr,float cg,float cb,float a) { (void)radius;(void)cr;(void)cg;(void)cb;if(a>0)bounded(r,1); }
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float px,float py,float radius,float cr,float cg,float cb,float a) { (void)t;(void)m;(void)f;(void)px;(void)py;gfx_cor(r,radius,cr,cg,cb,a); }
void gfx_icone(GfxRect r,const char *s,float cr,float cg,float cb,float a) { (void)s;gfx_cor(r,0,cr,cg,cb,a); }
int txt_largura(TxtEstilo e,const char *s) { return (int)strlen(i18n(s))*(e==TXT_LOG_COD?104:e==TXT_ILHA_PERGUNTA?20:12); }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) { (void)r;(void)g;(void)b;(void)a;return (TxtLinha){.w=txt_largura(e,s),.h=e==TXT_LOG_COD?100:28}; }
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) { assert(w>0);TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=(int)w;return t; }
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) { if(a>0)bounded((GfxRect){x,y,l.w,l.h},1); }
void txt_desenhar(TxtLinha l,float x,float y) { txt_desenhar_alpha(l,x,y,1); }
float txt_bloco_corta(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float lead,float a,int max) {
  assert(w>0);int n=(int)ceilf(txt_largura(e,s)/w);if(max>0&&n>max)n=max;
  (void)r;(void)g;(void)b;if(a>0)txt_desenhar_alpha((TxtLinha){.w=(int)w,.h=(int)(n*lead)},x,y,a);return n*lead;
}
float txt_bloco(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float lead,float a,int max) { return txt_bloco_corta(e,s,r,g,b,x,y,w,lead,a,max); }
void ponteiro_camada(void) { nTargets=0; }
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int i,int b) {
  GfxRect r={x,y,w,h};bounded(r,0);assert(y>=0&&y+h<=screenH()+.01f);assert(nTargets<32);
  targets[nTargets].r=r;targets[nTargets].focus=f;targets[nTargets].action=a;targets[nTargets].a=i;targets[nTargets++].b=b;
}
void ponteiro_rolagem(PonteiroRolagemFn f) { scrollFn=f; }
Uint32 SDL_GetTicks(void) { return 5000; }
int ajustes_tinta_foco(void) { return 0; }
void ajustes_acento(float *r,float *g,float *b) { *r=.4f;*g=.7f;*b=1; }
int ajustes_vidro(void) { return 0; }
int ajustes_idioma(void) { return 0; }
int ajustes_envio_auto(void) { return 0; }
int ajustes_animacoes_reduzidas(void) { return 1; }
char *dados_ler(const char *s) { (void)s;return NULL; }
int dados_gravar(const char *s,const char *t) { (void)s;(void)t;writes++;return 1; }

int teclado_aberto(void) { return 0; }
void teclado_atualizar(float dt,Uint32 agora) { (void)dt;(void)agora; }
int teclado_resultado(void) { return TECLADO_NADA; }
const char *teclado_texto(void) { return ""; }
int recomenda_soc_estado(void) { return REC_SOC_NADA; }
void recomenda_soc_limpar(void) {}
int recomenda_n_pedidos(void) { return 0; }
const char *recomenda_meu_codigo(void) { return ""; }
int recomenda_n_achados(void) { return 0; }
int recomenda_pesquisavel(void) { return 0; }
int recomenda_comunidade_fechada(void) { return 1; }
int recomenda_n_bloqueados(void) { return 0; }
int recomenda_cartao(RecPessoa *c) { (void)c;return 0; }
int recomenda_cartao_n_recentes(void) { return 0; }
void plrui_material(GfxRect r,float raio,int modal,float a) { (void)modal;gfx_cor(r,raio,1,1,1,a); }
void ponteiro_alvo_faixa(float x,float y,float w,float h,float top,float bottom,PonteiroFn f,PonteiroFn a,int i,int b) { float end=fminf(y+h,bottom);y=fmaxf(y,top);if(end>y)ponteiro_alvo(x,y,w,end-y,f,a,i,b); }
float plrui_kicker(const char *s,float x,float y,int r,int g,int b,float a) { TxtLinha t=txt_linha(TXT_MINI,s,r,g,b,255);if(x>=0)txt_desenhar_alpha(t,x,y,a);return t.w; }
void plrui_anel(float x,float y,float d,int gray,Uint32 now,float a) { (void)gray;(void)now;gfx_cor((GfxRect){x-d*.5f,y-d*.5f,d,d},.5f,1,1,1,a); }
float plrui_dicas(const char *const *k,const char *const *r,int n,float x,float y,int dir,float a) { (void)k;(void)r;(void)n;(void)x;(void)y;(void)dir;(void)a;return 0; }
void gfx_esqueleto(GfxRect r,float raio,float cr,float cg,float cb,float a) { gfx_cor(r,raio,cr,cg,cb,a); }
void rec_identidade(const char *n,const char *p,char *one,size_t no,char *two,size_t nt) { snprintf(one,no,"%s",n);if(two&&nt)snprintf(two,nt,"%s",p); }
void rec_avatar_estilo(GfxRect r,const char *u,const char *n,const char *id,float a,int e) { (void)u;(void)n;(void)id;(void)e;gfx_cor(r,.5f,1,1,1,a); }
float rec_selo_pessoa_largura(const char *s) { (void)s;return 0; }
float rec_selo_pessoa(float x,float y,const char *s,float a) { (void)x;(void)y;(void)s;(void)a;return 0; }
void plrui_linha_foco(GfxRect r,float raio,float a) { gfx_cor(r,raio,1,1,1,a); }
float badge_desenhar(float x,float y,const char *s,BadgeEstilo e,float a) { (void)x;(void)y;(void)s;(void)e;(void)a;return 0; }
float plrui_botao_largura(const char *s,const char *i) { return txt_largura(TXT_ILHA_ITEM,s)+56+(i?30:0); }
float plrui_botao(float x,float y,const char *s,const char *i,float f,float a) { (void)f;float w=plrui_botao_largura(s,i);gfx_cor((GfxRect){x,y,w,60},.5f,1,1,1,a);return w; }
void plrui_botao_repouso(GfxRect r,float a) { gfx_cor(r,.5f,1,1,1,a); }
float ajustes_acento_tinta(float *r,float *g,float *b) { if(r)*r=.4f;if(g)*g=.7f;if(b)*b=1;return 0; }
const char *rec_genero_rotulo(int i) { (void)i;return "Drama"; }
void teclado_desenhar(Uint32 now) { (void)now; }
int SDL_strcasecmp(const char *a,const char *b) { return strcasecmp(a,b); }
int main(void) {
 const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
 const float zooms[]={1,1.2f,1.3f,1.5f};
 for(int t=0;t<4;t++)for(int z=0;z<4;z++)for(int error=0;error<2;error++)for(longText=0;longText<2;longText++) {
  nv_layout_w=screens[t][0];nv_layout_h=screens[t][1];ui=zooms[z];scale=1;
  aberto=1;pagina=PG_LISTA;listaOrigem=2;listaErro=error;listaCarregando=0;
  opAtual=tecladoPara=avisoAte=0;feitoPub[0]=0;foco=0;rolagem=rolagemAlvo=0;toquerol_limpar(&toque);
  pessoas_atualizar(1.0f/60,5000);assert(nL==2&&foco==1);
  nTargets=0;pessoas_desenhar(5000);assert(nTargets==1&&targets[0].a==1);
  float rowTop=pnTopo+8+linhaY[foco]-rolagem;
  assert(rowTop>=pnTopo-.01f&&rowTop+alturaLinha(&linhas[foco])<=pnBase+.01f);
  assert(fabsf(targets[0].r.h-alturaLinha(&linhas[foco]))<.01f);
  assert(fabsf(janelaFoco()-toque.regiao.h)<.01f&&fabsf(scale-1)<.01f);
  if(toque.maximo>0) {
    int oldFocus=foco,oldWrites=writes,oldRequests=requests;
    PonteiroRolagem e={PONT_ROL_INICIO,1,0,0,(toque.regiao.x+10)*toque.escala,(toque.regiao.y+10)*toque.escala};
    assert(toqueRolar(&e));e.fase=PONT_ROL_MOVER;e.delta=13.25f*toque.escala;assert(toqueRolar(&e));
    assert(foco==oldFocus&&writes==oldWrites&&requests==oldRequests);
    nv_layout_w=1080;nv_layout_h=2340;float expected=janelaFoco();
    pessoas_atualizar(1.0f/60,5016);assert(fabsf(rolagem-fmaxf(0,conteudoH-expected))<.01f);
  }
 }
 puts("pessoas_phone_focus: actual updater/draw, initial Retry/Profile target visible, four viewports/zooms PASS");
 return 0;
}
