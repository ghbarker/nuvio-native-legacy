/* Actual Live TV diagnostic page; text/GL/native-video and external services
 * are inert. The fixture never opens a provider or household account. */
#define NV_TOUCH_UI 1
#define SDL_MAIN_HANDLED 1
#ifdef _WIN32
#include <time.h>
static struct tm *ltd_localtime_r(const time_t *t, struct tm *out) { struct tm *p=localtime(t);if(p)*out=*p;return p?out:NULL; }
#define localtime_r ltd_localtime_r
#endif
#include "../src/livetvdiag.c"
#include <assert.h>
float nv_layout_w = 1080, nv_layout_h = 2340;
static int testMobile = 1;
int layout_modo_mobile(void) { return testMobile; }
float gfx_tex_aspect_atual, gfx_card_forcar_cover_atual;
int txt_pendentes, anim_politica_reduzida;
static float escala = 1, zoom = 1;
static GfxRect recorte, botoesTeste[32];
static int alvoA[32], alvoB[32];
static int cortando, nBotoesTeste, ultimoPoster, nTextos;
static float limiteTexto;
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
TxtLinha txt_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) { (void)r; (void)g; (void)b; (void)a; return (TxtLinha){0,txt_largura(e,s),28}; }
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
int ajustes_tinta_foco(void) { return 0; }
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
    assert(y>=0&&y+h<=telaH()+.01f); if(nBotoesTeste<32){botoesTeste[nBotoesTeste]=(GfxRect){x,y,w,h};alvoA[nBotoesTeste]=a;alvoB[nBotoesTeste]=b;nBotoesTeste++;}
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

static int windowCalls,holeCalls,applies,sends,queries;
static GfxRect nativeWindow,hole;
int video_largura(void) { return 1440; }
int video_altura(void) { return 1080; }
void video_janela(int x,int y,int w,int h) { nativeWindow=(GfxRect){x,y,w,h};windowCalls++;assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=nv_layout_w+1&&y+h<=nv_layout_h+1); }
void gfx_furo_raio(GfxRect r,float raio) { (void)raio;limites(r,0);hole=r;holeCalls++; }
int avisos_envio_estado(void) { return 3; }
int ajustes_livetv_resolucao(void) { return 0; }
int ajustes_livetv_formato(void) { return 0; }
Uint32 ajustes_livetv_espera_ms(void) { return 15000; }
int ajustes_livetv_modo(void) { return 0; }
void vazao_fmt_mbps(char *b,size_t n,int kbps,char sep) { (void)sep;snprintf(b,n,"%.1f",kbps/1000.0); }
void ajustes_livetv_aplicar(int r,int f,int e) { assert(r==L.rec.resolucao&&f==L.rec.formato&&e==L.rec.espera);applies++; }
void ajustes_livetv_aplicar_modo(int m) { assert(m==L.recModo); }
void ajustes_livetv_aplicar_proxy(int p) { assert(p==1); }
void avisos_enviar_registro_atual(void) { sends++; }
int xtream_configurado(void) { return 0; }
int guia_canais_para_teste(GuiaVariante *v,char bases[][600],int max,char *g,size_t n) { (void)v;(void)bases;(void)max;if(n)*g=0;queries++;return 0; }
void video_parar(void) {}
void ajustes_ui_fundo(void) {}
void ajustes_ui_ilha(GfxRect r,float raio,int modal) { plrui_material(r,raio,modal,1); }
float ajustes_ui_kicker(const char *s,float x,float y,float a) { return plrui_kicker(s,x,y,243,242,239,a); }
void ajustes_ui_neutro(GfxRect r,float raio,float a) { gfx_cor(r,raio,1,1,1,a); }
void ajustes_ui_foco_linha(GfxRect r,float raio) { gfx_cor(r,raio,1,1,1,1); }
float ajustes_rail_largura_fixa(void) { return 0; }
float ajustes_ilha_x(void) { return 48; }
float ajustes_ui_botao(const char *s,const char *ico,float x,float y,int f) { return plrui_botao(x,y,s,ico,f,1); }
/* No worker is allowed in this drawing/action fixture. Ignoring its entry
 * pointer also prevents the linker from retaining provider implementations. */
__attribute__((always_inline)) inline int pthread_create(pthread_t *t,const pthread_attr_t *a,void *(*fn)(void *),void *arg) {
  (void)t;(void)a;(void)fn;(void)arg;assert(!"unexpected provider worker");return 1;
}
static void dados(void) {
  memset(&L,0,sizeof L);L.n=6;L.estado=E_PRONTO;L.recModo=M_B;L.rec.confianca=6;L.rec.semDecoder=2;L.rec.dezBits=1;
  L.rec.resolucao=2;L.rec.formato=1;L.rec.espera=1;L.rec.kbpsMediana=25000;L.redeMedida=1;L.kbps=25000;L.kbpsPior=12000;L.latenciaMs=1200;
  L.xtConfig=L.contaLida=L.conta.valido=1;L.conta.maxConexoes=L.conta.conexoes=1;L.conta.formatosDeclarados=1;L.conta.temTs=1;
  snprintf(L.conta.status,sizeof L.conta.status,"Active");strcpy(L.grupo,"A channel group name with many words");L.tocouModo[M_P]=1;
  L.conta.expira = 1830297599;
  L.aplicado=L.enviou=1;
  for(int i=0;i<6;i++) { snprintf(L.it[i].nome,sizeof L.it[i].nome,"Channel %d: an intentionally long channel name with several translated words to wrap",i);L.it[i].pronto=1;L.it[i].xt=1;L.it[i].f[F_HLS].servido=1;L.it[i].f[F_HLS].kbps=20000;L.it[i].f[F_HLS].altura=1080;strcpy(L.it[i].f[F_HLS].codec,"H264 · AAC · a long codec annotation"); }
  ltdPhoneReiniciar();nBotoesTeste=0;windowCalls=holeCalls=0;cortando=0;
}
int main(void) {
  const float screens[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}},scales[]={1,1.2f,1.3f,1.5f};
  for(int t=0;t<4;t++)for(int z=0;z<4;z++) {
    nv_layout_w=screens[t][0];nv_layout_h=screens[t][1];zoom=scales[z];escala=1;
    for(int st=0;st<4;st++) {
      dados();L.estado=st;L.pfVivo=st==E_PLAYER;L.pfDesde=4000;
      livetvdiag_desenhar(5000);assert(escala==1);assert(ltdPhone.corpo.h>0&&ltdPhone.corpo.w>0);assert(nBotoesTeste==(st==E_PRONTO?4:2));
      GfxRect body=ltdPhone.corpo,buttons[3];int n=nBotoesTeste-1;
      for(int b=1;b<nBotoesTeste;b++){buttons[b-1]=botoesTeste[b];assert(botoesTeste[b].y>=body.y+body.h);}
      GfxRect before=quadroPlayer;
      if(st==E_PLAYER) { assert(windowCalls==1&&holeCalls==1);assert(fabsf(hole.w/hole.h-4.0f/3)<.001f);assert(fabsf(nativeWindow.x-hole.x*zoom)<=1&&fabsf(nativeWindow.y-hole.y*zoom)<=1&&fabsf(nativeWindow.w-hole.w*zoom)<=1&&fabsf(nativeWindow.h-hole.h*zoom)<=1);assert(hole.y+hole.h<=body.y); }
      int state=L.estado,focus=L.botao,applied=L.aplicado,sent=L.enviou;
      if(ltdPhone.rolagem.maximo>0)arrastar(&ltdPhone.rolagem,ltdPhoneRolar,1);
      assert(L.estado==state&&L.botao==focus&&L.aplicado==applied&&L.enviou==sent);
      nBotoesTeste=0;livetvdiag_desenhar(5000);assert(ltdPhone.offset==ltdPhone.rolagem.maximo);assert(!memcmp(&before,&quadroPlayer,sizeof before));
      for(int b=1;b<nBotoesTeste;b++)assert(!memcmp(&buttons[b-1],&botoesTeste[b],sizeof(GfxRect)));
      float end=ltdPhoneCorpo(body.x,body.y-ltdPhone.offset,body.w,0);assert(fabsf((body.y-ltdPhone.offset+end)-(body.y+body.h))<.01f||ltdPhone.rolagem.maximo==0);
      /* Rotation clamps the real offset without changing the diagnostic state. */
      nv_layout_w=screens[(t+1)%4][0];nv_layout_h=screens[(t+1)%4][1];nBotoesTeste=0;livetvdiag_desenhar(5000);assert(L.estado==state&&ltdPhone.offset<=ltdPhone.rolagem.maximo);
      nv_layout_w=screens[t][0];nv_layout_h=screens[t][1];
    }
  }
  dados();ltdPhoneAcao(0,B_APLICAR+1);assert(applies==1&&L.aplicado);ltdPhoneAcao(0,B_ENVIAR+1);assert(sends==1&&L.enviou);
  L.estado=E_REDE;ltdPhoneAcao(0,B_APLICAR+1);ltdPhoneAcao(0,B_ENVIAR+1);assert(applies==1&&sends==1);
  L.estado=E_PRONTO;L.rec.confianca=L.redeMedida=0;L.recModo=-1;L.tocouModo[M_P]=0;ltdPhoneAcao(0,B_APLICAR+1);assert(applies==1);
  ltdPhoneAcao(0,B_ENVIAR+1);assert(sends==2);ltdPhone.offset=400;ltdPhoneAcao(0,B_DENOVO+1);assert(queries==1&&L.n==0&&L.estado==E_PRONTO&&!ltdPhone.offset);
  L.sair=1;ltdPhoneAcao(0,B_ENVIAR+1);assert(sends==2);
  puts("livetvdiag_phone: all states, measured body/footer, scroll/rotation, native plane/hole and guarded existing actions PASS");
}
