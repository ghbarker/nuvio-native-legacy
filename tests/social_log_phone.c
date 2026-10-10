/* Real reaction, friend-empty and log dialog drawing/actions. No GL/network.
 * Font doubles stress wrapping; native-font captures are a separate check. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#ifdef _WIN32
#include <time.h>
static struct tm *phone_localtime_r(const time_t *t, struct tm *out) { struct tm *r = localtime(t); if (r) *out = *r; return r ? out : NULL; }
#define localtime_r phone_localtime_r
#endif
#if defined(TESTE_REACAO)
#include "../src/reacao.c"
#elif defined(TESTE_AMIGO)
#include "../src/amigoperfil.c"
#elif defined(TESTE_REGISTRO)
#define REGISTRO_TESTE 1
#include "../src/registro.c"
#else
#error choose TESTE_REACAO, TESTE_AMIGO or TESTE_REGISTRO
#endif
#include <assert.h>
#include <math.h>
float nv_layout_w = 1080, nv_layout_h = 2340;
static float scale = 1, ui = 1;
static GfxRect clip;
static int clipped, nTargets, writes, requests;
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
const char *i18n(const char *s) { return s; }
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
#if defined(TESTE_REACAO)
void plrui_material(GfxRect r,float radius,int modal,float a) { (void)modal;gfx_cor(r,radius,1,1,1,a); }
void plrui_trilho(GfxRect r,float f,float cr,float cg,float cb,float a) { (void)f;gfx_cor(r,0,cr,cg,cb,a); }
int perfis_ativo(void) { return 1; }
void atividade_id_puro(char *d,size_t n,const char *s) { snprintf(d,n,"%s",s);char *p=strchr(d,':');if(p)*p=0; }
long long atividade_origem(const char *s,char *n,size_t z) { (void)s;if(n&&z)n[0]=0;return 0; }
int atividade_envia(void) { return 0; }
void atividade_reacao(const char *id,const char *midia,const char *t,const char *p,int r,long long rec) { (void)id;(void)midia;(void)t;(void)p;(void)r;(void)rec;requests++; }
int recomenda_ativo(void) { return 1; }
void recresp_responder(long long rec,int r,const char *t) { (void)rec;(void)r;(void)t;requests++; }
void recresp_pular(long long rec) { (void)rec; }
int trakt_ativo(void) { return 0; }
int trakt_avaliar(const char *id,const char *tipo,int nota) { (void)id;(void)tipo;(void)nota;assert(0);return 0; }
int teclado_aberto(void) { return 0; }
void teclado_evento(const SDL_Event *e) { (void)e; }
int teclado_resultado(void) { return TECLADO_NADA; }
const char *teclado_texto(void) { return ""; }
void teclado_abrir_com(const char *t,const char *d,int max,const char *a,const char *v) { (void)t;(void)d;(void)max;(void)a;(void)v;c.escrevendo=1; }
#elif defined(TESTE_REGISTRO)
static AvisosEnvio envio;
int avisos_envio_info(AvisosEnvio *e) { *e=envio;return envio.estado; }
void avisos_enviar_registro_atual(void) { requests++; }
int rede_saude_resumo(int *f,int *h,char *host,size_t n,unsigned *ms) { *f=3;*h=2;snprintf(host,n,"example.invalid");*ms=4000;return 1; }
float txt_tracking(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float a,float spacing) { TxtLinha l=txt_linha(e,s,r,g,b,255);(void)spacing;if(a>0)txt_desenhar_alpha(l,x,y,a);return l.w; }
#endif
static void drag(ToqueRolagem *r,PonteiroRolagemFn fn,int axis) {
  PonteiroRolagem e={PONT_ROL_INICIO,axis,0,0,(r->regiao.x+10)*r->escala,(r->regiao.y+10)*r->escala};
  assert(fn(&e));e.fase=PONT_ROL_MOVER;e.delta=-13.25f*r->escala;assert(fn(&e));
  e.fase=PONT_ROL_SOLTAR;fn(&e);e.fase=PONT_ROL_INERCIA;e.delta=-100000;fn(&e);e.fase=PONT_ROL_FIM;fn(&e);
  assert(fabsf(*r->offset-r->maximo)<.01f);
}
int main(void) {
  const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float zooms[]={1,1.2f,1.3f,1.5f};
  for(int t=0;t<4;t++)for(int z=0;z<4;z++) {
    nv_layout_w=screens[t][0];nv_layout_h=screens[t][1];scale=ui=zooms[z];nTargets=0;clipped=0;
#if defined(TESTE_REACAO)
    char title[160];memset(title,'W',sizeof title-1);title[159]=0;
    abrir("tt1","movie",title,"",1,5000);c.anim=1;
    char perg[256];snprintf(perg,sizeof perg,"O que achou de %s?",title);
    reacaoTelefoneDesenhar(perg,"Ana mandou este filme","Ana vai ver sua resposta",ROTULO,5000,screenH()-48,1);
    assert(nTargets==4&&scrollFn==reacaoTelefoneRolar);
    for(int i=1;i<4;i++)assert(targets[i].r.y>=reacaoTelefone.corpo.y+reacaoTelefone.corpo.h+24-.01f);
    GfxRect footer=targets[1].r;int oldw=writes,oldr=requests,focus=c.foco;
    if(reacaoTelefone.rolagem.maximo>0)drag(&reacaoTelefone.rolagem,reacaoTelefoneRolar,1);
    assert(writes==oldw&&requests==oldr&&c.foco==focus&&c.aberto);
    reacaoTelefoneDesenhar(perg,"Ana mandou este filme","Ana vai ver sua resposta",ROTULO,5000,screenH()-48,1);
    assert(fabsf(footer.y-targets[1].r.y)<.01f);
    reacaoTelefoneEscolher(0,reacaoTelefoneGeracao*2+c.passo+1);assert(c.aberto&&requests==oldr);
    targets[3].action(targets[3].a,targets[3].b);assert(!c.aberto&&reacao_estado("tt1")==REACAO_NAO);
    int stale=targets[3].b;
    abrir("tt2","movie","Filme","",1,5000);c.anim=1;c.passo=1;c.rec=2;strcpy(c.nome,"Ana");
    reacaoTelefoneEscolher(0,stale);assert(c.aberto&&!c.escrevendo);
    reacaoTelefoneEscolher(1,reacaoTelefoneGeracao*2+1);assert(c.escrevendo&&c.aberto);
#elif defined(TESTE_AMIGO)
    scale=1; /* This existing screen uses the base canvas, including at UI zoom. */
    vazioFila(676,200,"Ainda não há nada compartilhado por esta pessoa nesta fileira.",1);
#elif defined(TESTE_REGISTRO)
    aberto=1;area=RG_TUDO;segEd=1;pausado=0;nLin=0;semFonte=0;regTesteTexto="[rede] teste\n";
    for(int n=0;n<2;n++) {
      toquerol_limpar(&toqueAreas);toqueAreasOffset=0;nTargets=0;for(int i=0;i<RG_N;i++)cont[i]=2214;
      rgSeg(n,RP_X+30,RP_Y+54);assert(nTargets>0&&scrollFn==toqueRegistroRolar);
      int oldArea=area,oldPause=pausado,oldFocus=foco;
      if(toqueAreas.maximo>0)drag(&toqueAreas,toqueRegistroRolar,0);
      assert(area==oldArea&&pausado==oldPause&&foco==oldFocus);
      nTargets=0;rgSeg(n,RP_X+30,RP_Y+54);assert(targets[nTargets-1].a==RG_SISTEMA);
      toqueRegArea(RG_SISTEMA,0);assert(area==RG_SISTEMA);area=RG_TUDO;
    }
    for(int st=1;st<=3;st++)for(int reason=AVISOS_ENVIO_OK;reason<=AVISOS_ENVIO_INDISPONIVEL;reason++) {
      memset(&envio,0,sizeof envio);envio.estado=st;envio.motivo=reason;strcpy(envio.codigo,"WWWWWW");envio.http=503;envio.bytes=190000;
      envTesteFixo=1;registro_envio_abrir();envTelefoneDesenhar(&envio);
      int retry=st==3&&reason!=AVISOS_ENVIO_OFFLINE&&reason!=AVISOS_ENVIO_INDISPONIVEL;
      assert(nTargets==2+retry&&scrollFn==envTelefoneRolar);
      int oldr=requests,oldw=writes;GfxRect footer=targets[1].r;
      if(envTelefone.rolagem.maximo>0)drag(&envTelefone.rolagem,envTelefoneRolar,1);
      assert(requests==oldr&&writes==oldw&&envAberto);
      envTelefoneDesenhar(&envio);assert(fabsf(footer.y-targets[1].r.y)<.01f);
      int chave=envTelefoneChave(&envio);envTelefoneEscolher(0,chave+1);assert(envAberto&&requests==oldr);
      if(retry) { envTelefoneEscolher(0,chave);assert(envAberto&&requests==oldr+1); }
      envTelefoneEscolher(retry,chave);assert(!envAberto);
      envTelefoneEscolher(0,chave);assert(requests==oldr+retry);
    }
#endif
  }
  nv_layout_w=1920;nv_layout_h=1080;assert(!telefoneui_ativo());
  puts("social_log_phone: real phone draw, four viewports/zooms, clipped hits, fixed footers, drag/state/action gates PASS");
  return 0;
}
