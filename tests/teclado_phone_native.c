/* Real keyboard + Android text bridge + touch hit-test, without SDL/GL or
   network services. Only platform, drawing and transport boundaries vary. */
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/teclado.c"
#define NV_ANDROID 1
#include "../src/sistexto.c"
#undef NV_ANDROID
#include "../src/ponteiro.c"
/* Keep the real cellphone ownership/one-shot receipt lifecycle. Its network
   server is replaced below; the QR encoder and drawing path are real. */
void tecladoFixtureGlGenTextures(GLsizei n,GLuint *t);
void tecladoFixtureGlBindTexture(GLenum a,GLuint b);
void tecladoFixtureGlPixelStorei(GLenum a,GLint b);
void tecladoFixtureGlTexImage2D(GLenum a,GLint b,GLint c,GLsizei w,GLsizei h,GLint d,GLenum e,GLenum f,const void *p);
void tecladoFixtureGlTexParameteri(GLenum a,GLenum b,GLint c);
#define glGenTextures tecladoFixtureGlGenTextures
#define glBindTexture tecladoFixtureGlBindTexture
#define glPixelStorei tecladoFixtureGlPixelStorei
#define glTexImage2D tecladoFixtureGlTexImage2D
#define glTexParameteri tecladoFixtureGlTexParameteri
#define dono celDonoFixture
#define aberto celAbertoFixture
#include "../src/celbotao.c"
#undef dono
#undef aberto
#undef glGenTextures
#undef glBindTexture
#undef glPixelStorei
#undef glTexImage2D
#undef glTexParameteri
#undef NV_ESCALA_TELA
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W nv_layout_w
#define NV_TELA_H nv_layout_h
#include "../src/qr.c"

#define SDL_GetTicks tecladoFixtureZeroTicks
#include "ponteiro_sdl.h"
#undef SDL_GetTicks

float nv_layout_w=1080,nv_layout_h=2340;
float gfx_opacidade_grupo=1,gfx_tex_aspect_atual;
static Uint32 relogioFixture=1000;
static float escalaUiFixture=1,escalaAtivaFixture=1;
static int fonteLargaFixture,vidroFixture,reduzidasFixture=1;
static int imePedidosFixture,vozPedidosFixture,imeFechosFixture;
static char imeInicialFixture[TECLADO_LONGO+1];
static int imeMaxFixture;
static int celDisponivelFixture=1,celEstadoFixture,celPedidosFixture,celFechosFixture,celPendenteFixture;
static char celValorFixture[TECLADO_LONGO+1];
static int quadrosFixture,toquesFixture,swipesFixture,casosFixture,textosFixture;
static int qrDesenhosFixture,qrUploadsFixture,alvosAntigosFixture;
static int medindoFixture;
static int rolagensAntigasFixture;
static int coresQuadroFixture,outrosQuadroFixture;
static GfxRect ultimaCorQuadroFixture;
static float ultimaCorRgbFixture[3];
static int clipFixture;
static GfxRect recorteFixture;
static char textoLinhasFixture[512][1201];
static int nTextoLinhasFixture;
static int segredoDesenhadoFixture;
static const char *segredoFixture;

Uint32 SDL_GetTicks(void) { return relogioFixture; }
float gfx_escala_ui(void) { return escalaUiFixture; }
float gfx_escala(void) { return escalaAtivaFixture; }
float gfx_escala_entrar(void) { float a=escalaAtivaFixture;escalaAtivaFixture=escalaUiFixture;return a; }
void gfx_escala_sair(float a) { escalaAtivaFixture=a; }
int ajustes_animacoes_reduzidas(void) { return reduzidasFixture; }
int ajustes_vidro(void) { return vidroFixture; }
int ajustes_tinta_foco(void) { return 24; }
int ajustes_idioma(void) { return 0; }
void ajustes_acento(float *r,float *g,float *b) { *r=.7f;*g=.8f;*b=.9f; }
const char *desc_tmdb_idioma(void) { return "pt-BR"; }
const char *i18n(const char *s) {
  if(fonteLargaFixture&&(!strcmp(s,"Concluir")||!strcmp(s,"Cancelar")||!strcmp(s,"Falar")||
      !strcmp(s,"Digitar pelo celular")||!strcmp(s,"apagar")||!strcmp(s,"limpar")||
      !strcmp(s,"mostrar")||!strcmp(s,"ocultar")))
    return "A deliberately long translated action label that needs measured clipping";
  return s;
}
static int caracteresFixture(const char *s) { int n=0;for(;*s;s++)if(((unsigned char)*s&0xc0)!=0x80)n++;return n; }
int txt_largura(TxtEstilo e,const char *s) {
  (void)e;if(medindoFixture)assert(fabsf(gfx_escala()-gfx_escala_ui())<.0001f);
  return caracteresFixture(i18n(s))*(fonteLargaFixture?27:12);
}
static GLuint linhaFixture(const char *s) {
  for(int i=0;i<nTextoLinhasFixture;i++)if(!strcmp(textoLinhasFixture[i],s))return (GLuint)(i+1);
  assert(nTextoLinhasFixture<512);
  snprintf(textoLinhasFixture[nTextoLinhasFixture],sizeof textoLinhasFixture[0],"%s",s);
  return (GLuint)(++nTextoLinhasFixture);
}
static void visivelFixture(GfxRect r) {
  float e=gfx_escala();r.x*=e;r.y*=e;r.w*=e;r.h*=e;
  if(clipFixture) {
    float x=fmaxf(r.x,recorteFixture.x),y=fmaxf(r.y,recorteFixture.y);
    r.w=fminf(r.x+r.w,recorteFixture.x+recorteFixture.w)-x;
    r.h=fminf(r.y+r.h,recorteFixture.y+recorteFixture.h)-y;r.x=x;r.y=y;
  }
  if(teTelefoneIme()&&teclado_aberto()&&r.w>0&&r.h>0) {
    if(r.x<-.01f||r.y<-.01f||r.x+r.w>nv_layout_w+.01f||r.y+r.h>nv_layout_h+.01f)
      fprintf(stderr,"phone visible drawing outside canvas: %.2f,%.2f %.2fx%.2f canvas%.0fx%.0f UI%.0f\n",r.x,r.y,r.w,r.h,nv_layout_w,nv_layout_h,escalaUiFixture*100);
    assert(r.x>=-.01f&&r.y>=-.01f&&r.x+r.w<=nv_layout_w+.01f&&r.y+r.h<=nv_layout_h+.01f);
  }
}
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)r;(void)g;(void)b;(void)a;
  return (TxtLinha){.tex=linhaFixture(i18n(s)),.w=txt_largura(e,s),.h=28};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float max) {
  assert(max>0);TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>max)t.w=(int)max;return t;
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) {
  if(a<=.001f)return;
  assert(isfinite(x)&&isfinite(y)&&l.w>=0&&l.h>0);
  visivelFixture((GfxRect){x,y,(float)l.w,(float)l.h});
  textosFixture++;
  if(segredoFixture&&l.tex&&strstr(textoLinhasFixture[l.tex-1],segredoFixture))segredoDesenhadoFixture++;
}
void txt_desenhar(TxtLinha l,float x,float y) { txt_desenhar_alpha(l,x,y,1); }
float txt_tracking(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float a,float trk) {
  float w=txt_largura(e,s)+fmaxf(0,(float)caracteresFixture(s)-1)*trk;
  if(x>=0)txt_desenhar_alpha(txt_linha(e,s,r,g,b,255),x,y,a);return w;
}
float txt_bloco(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float leading,float a,int max) {
  s=i18n(s);
  assert(w>0&&leading>0);if(!s[0])return 0;
  int n=(int)ceilf(txt_largura(e,s)/w);if(n<1)n=1;if(max>0&&n>max)n=max;
  if(a>.001f)txt_desenhar_alpha(txt_linha_corta(e,s,r,g,b,255,w),x,y,a);
  return leading*n;
}
float txt_bloco_corta(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float leading,float a,int max) {
  return txt_bloco(e,s,r,g,b,x,y,w,leading,a,max);
}
static void desenhoFixture(GfxRect r) { assert(isfinite(r.x)&&isfinite(r.y)&&r.w>=0&&r.h>=0); }
void gfx_cor(GfxRect r,float raio,float cr,float cg,float cb,float a) {
  (void)raio;(void)a;desenhoFixture(r);coresQuadroFixture++;
  float e=gfx_escala();ultimaCorQuadroFixture=(GfxRect){r.x*e,r.y*e,r.w*e,r.h*e};
  ultimaCorRgbFixture[0]=cr;ultimaCorRgbFixture[1]=cg;ultimaCorRgbFixture[2]=cb;
}
void gfx_rect(GfxRect r,GLuint tex,GfxModo modo,float f,float px,float py,float raio,float cr,float cg,float cb,float a) {
  (void)tex;(void)modo;(void)f;(void)px;(void)py;(void)raio;(void)cr;(void)cg;(void)cb;(void)a;desenhoFixture(r);
  outrosQuadroFixture++;
  if(tex&&modo==GFX_SNAP&&a>.001f) { visivelFixture(r);qrDesenhosFixture++; }
}
void gfx_icone(GfxRect r,const char *nome,float cr,float cg,float cb,float a) { (void)nome;(void)cr;(void)cg;(void)cb;desenhoFixture(r);outrosQuadroFixture++;if(a>.001f)visivelFixture(r); }
void gfx_luz_canto(GfxRect r,float raio,float x,float y,float alcance,float cr,float cg,float cb,float a) {
  (void)raio;(void)x;(void)y;(void)alcance;(void)cr;(void)cg;(void)cb;(void)a;desenhoFixture(r);outrosQuadroFixture++;
}
void gfx_vidro_folha(GfxRect r,float raio,float a) { (void)raio;(void)a;desenhoFixture(r);outrosQuadroFixture++; }
void gfx_recorte(float x,float y,float w,float h) { assert(w>0&&h>0);float e=gfx_escala();recorteFixture=(GfxRect){x*e,y*e,w*e,h*e};clipFixture=1;outrosQuadroFixture++; }
void gfx_sem_recorte(void) { clipFixture=0; }
void tecladoFixtureGlGenTextures(GLsizei n,GLuint *t) { while(n-->0)*t++=1; }
void tecladoFixtureGlBindTexture(GLenum a,GLuint b) { (void)a;(void)b; }
void tecladoFixtureGlPixelStorei(GLenum a,GLint b) { (void)a;(void)b; }
void tecladoFixtureGlTexImage2D(GLenum a,GLint b,GLint c,GLsizei w,GLsizei h,GLint d,GLenum e,GLenum f,const void *p) {
  (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;assert(w>0&&h>0&&p);qrUploadsFixture++;
}
void tecladoFixtureGlTexParameteri(GLenum a,GLenum b,GLint c) { (void)a;(void)b;(void)c; }

/* Real sistexto state machine, simulated Android platform only. */
int android_st_teclado(const char *t,int max) { imePedidosFixture++;snprintf(imeInicialFixture,sizeof imeInicialFixture,"%s",t);imeMaxFixture=max;return 1; }
int android_st_ditar(const char *idioma) { assert(!strcmp(idioma,"pt-BR"));vozPedidosFixture++;return 1; }
void android_st_fechar(void) { imeFechosFixture++; }
int android_st_evento(char *dst,size_t n) { (void)dst;(void)n;return 0; }
int texto_sistema_disponivel(void) { return 0; }
int texto_sistema_aberto(void) { return 0; }
int texto_sistema_engole(const SDL_Event *e) { (void)e;return 0; }
Uint32 texto_sistema_evento(void) { return SDL_USEREVENT+7; }
const char *texto_sistema_valor(void) { return ""; }

/* Real celbotao ownership and receipt; simulated LAN transport only. */
int celular_disponivel(void) { return celDisponivelFixture; }
int celular_abrir(const char *t) { assert(t&&t[0]);celPedidosFixture++;celEstadoFixture=CEL_ESPERANDO;return 1; }
void celular_fechar(void) { celFechosFixture++;celEstadoFixture=CEL_PARADO; }
int celular_estado(void) { return celEstadoFixture; }
const char *celular_url(void) { return celEstadoFixture==CEL_ESPERANDO?"http://127.0.0.1:9999/fixture-token":""; }
int celular_pegar(char *dst,size_t n) { if(!celPendenteFixture)return 0;snprintf(dst,n,"%s",celValorFixture);celPendenteFixture=0;return 1; }

static void exigirTexto(const char *s) { if(strcmp(teclado_texto(),s))fprintf(stderr,"text mismatch: expected <%s>, got <%s>\n",s,teclado_texto());assert(!strcmp(teclado_texto(),s)); }
static void quadroFixture(void) {
  relogioFixture+=17;ponteiro_quadro(relogioFixture);teclado_atualizar(1.0f/60,relogioFixture);celb_atualizar(1.0f/60);
  clipFixture=0;segredoDesenhadoFixture=0;coresQuadroFixture=outrosQuadroFixture=0;int textosAntes=textosFixture;
  teclado_desenhar(relogioFixture);assert(!clipFixture);ponteiro_desenhar();quadrosFixture++;
  const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
  if(teTelefoneIme()&&teclado_aberto()&&tePhoneNativo()) {
    assert(n==1&&!v[0].focar&&!v[0].ativar);
    assert(v[0].x==0&&v[0].y==0&&fabsf(v[0].w-nv_layout_w)<.01f&&fabsf(v[0].h-nv_layout_h)<.01f);
    assert(textosFixture==textosAntes&&outrosQuadroFixture==0&&coresQuadroFixture==1);
    assert(ultimaCorQuadroFixture.x==0&&ultimaCorQuadroFixture.y==0&&fabsf(ultimaCorQuadroFixture.w-nv_layout_w)<.01f&&fabsf(ultimaCorQuadroFixture.h-nv_layout_h)<.01f);
    assert(ultimaCorRgbFixture[0]==0&&ultimaCorRgbFixture[1]==0&&ultimaCorRgbFixture[2]==0);
  }
  if(teTelefoneIme()&&teclado_aberto())for(int i=0;i<n;i++) {
    assert(v[i].focar!=focarTecla);
    if(st_dono()==ST_TECLADO&&st_estado()==ST_DIGITANDO)assert(v[i].ativar!=tePhoneAcao);
    if(v[i].ativar==tePhoneAcao) {
      assert(v[i].focar==tePhoneFocar&&v[i].w>0&&v[i].h>0);
      assert(v[i].x>=-.01f&&v[i].y>=-.01f&&v[i].x+v[i].w<=nv_layout_w+.01f&&v[i].y+v[i].h<=nv_layout_h+.01f);
    }
  }
}
static void quadrosFixtureN(int n) { while(n-->0)quadroFixture(); }
static void dedoFixture(Uint32 tipo,float x,float y) {
  SDL_Event e={0};e.type=tipo;e.tfinger.touchId=17;e.tfinger.fingerId=1;
  e.tfinger.x=x/nv_layout_w;e.tfinger.y=y/nv_layout_h;
  assert(ponteiro_evento(&e,teclado_evento));
}
static int alvoExiste(int id) {
  const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
  for(int i=0;i<n;i++)if(v[i].ativar==tePhoneAcao&&v[i].a==id)return 1;return 0;
}
static void rolarCorpoFixture(int subir,float fracao) {
  TePhoneLayout l=tePhoneMedir();float e=gfx_escala_ui(),x=(l.corpo.x+l.corpo.w*.8f)*e;
  float y=(l.corpo.y+l.corpo.h*(subir?.8f:.2f))*e;
  float yf=y+l.corpo.h*fracao*e*(subir?-1:1);
  char antes[TECLADO_LONGO+1];snprintf(antes,sizeof antes,"%s",teclado_texto());
  int ime=imePedidosFixture,voz=vozPedidosFixture,cel=celPedidosFixture,mask=teclado_mascarado();
  relogioFixture+=10;dedoFixture(SDL_FINGERDOWN,x,y);quadroFixture();
  relogioFixture+=100;dedoFixture(SDL_FINGERMOTION,x,yf);quadroFixture();
  relogioFixture+=100;dedoFixture(SDL_FINGERUP,x,yf);quadroFixture();
  quadrosFixtureN(reduzidasFixture?20:90);
  exigirTexto(antes);assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  assert(imePedidosFixture==ime&&vozPedidosFixture==voz&&celPedidosFixture==cel&&teclado_mascarado()==mask);
  swipesFixture++;
}
static void revelarAlvoFixture(int id) {
  for(int tentativa=0;tentativa<12&&!alvoExiste(id);tentativa++) {
    assert(teclado_aberto()&&!tePhoneNativo());
    TePhoneLayout l=tePhoneMedir();GfxRect r={0};
    for(int i=0;i<l.n;i++)if(l.controles[i].id==id)r=l.controles[i].r;
    assert(r.w>0&&l.maximo>0&&l.corpo.h>0);
    int subir=r.y>=tePhoneOffset+l.corpo.h;
    rolarCorpoFixture(subir,.6f);
  }
}
static void revelarQrFixture(void) {
  int antes=qrDesenhosFixture;
  for(int tentativa=0;tentativa<32&&qrDesenhosFixture==antes;tentativa++) {
    TePhoneLayout l=tePhoneMedir();
    if(l.qr.h>l.corpo.h)fprintf(stderr,"QR cannot fit: %.2f > body %.2f canvas%.0fx%.0f UI%.0f wide%d\n",l.qr.h,l.corpo.h,nv_layout_w,nv_layout_h,escalaUiFixture*100,fonteLargaFixture);
    assert(l.qr.h>0&&l.qr.h<=l.corpo.h+.01f);
    quadroFixture();
    if(qrDesenhosFixture==antes)rolarCorpoFixture(l.qr.y>=tePhoneOffset,.18f);
  }
  assert(qrDesenhosFixture>antes&&qrUploadsFixture>0);
}
static PonteiroAlvo alvoFixture(int id) {
  revelarAlvoFixture(id);
  const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
  for(int i=0;i<n;i++)if(v[i].ativar==tePhoneAcao&&v[i].a==id)return v[i];
  fprintf(stderr,"phone target missing: id%d canvas%.0fx%.0f UI%.0f wide%d, targets%d\n",id,nv_layout_w,nv_layout_h,escalaUiFixture*100,fonteLargaFixture,n);assert(0);return (PonteiroAlvo){0};
}
static void tocarFixture(int id) {
  PonteiroAlvo v=alvoFixture(id);float x=v.x+v.w*.5f,y=v.y+v.h*.5f;
  char antes[TECLADO_LONGO+1];snprintf(antes,sizeof antes,"%s",teclado_texto());
  int ime=imePedidosFixture,voz=vozPedidosFixture,cel=celPedidosFixture,mask=teclado_mascarado();
  relogioFixture+=10;dedoFixture(SDL_FINGERDOWN,x,y);quadroFixture();
  exigirTexto(antes);assert(teclado_aberto()&&imePedidosFixture==ime&&vozPedidosFixture==voz&&celPedidosFixture==cel&&teclado_mascarado()==mask);
  relogioFixture+=25;dedoFixture(SDL_FINGERUP,x,y);quadroFixture();toquesFixture++;
}
static void deslizarFixture(int id) {
  PonteiroAlvo v=alvoFixture(id);float x=v.x+v.w*.5f,y=v.y+v.h*.5f;
  char antes[TECLADO_LONGO+1];snprintf(antes,sizeof antes,"%s",teclado_texto());
  int ime=imePedidosFixture,voz=vozPedidosFixture,cel=celPedidosFixture,mask=teclado_mascarado();
  relogioFixture+=10;dedoFixture(SDL_FINGERDOWN,x,y);quadroFixture();
  relogioFixture+=25;dedoFixture(SDL_FINGERMOTION,x+70,y);quadroFixture();
  relogioFixture+=100;dedoFixture(SDL_FINGERUP,x+70,y);quadroFixture();
  exigirTexto(antes);assert(imePedidosFixture==ime&&vozPedidosFixture==voz&&celPedidosFixture==cel&&teclado_mascarado()==mask);
  assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);swipesFixture++;
}
static void editorAtivoFixture(void) {
  assert(st_dono()==ST_TECLADO&&st_estado()==ST_DIGITANDO);
  const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
  for(int i=0;i<n;i++)assert(v[i].ativar!=tePhoneAcao&&v[i].focar!=focarTecla);
  char antes[TECLADO_LONGO+1];snprintf(antes,sizeof antes,"%s",teclado_texto());
  int ime=imePedidosFixture,voz=vozPedidosFixture,cel=celPedidosFixture;
  const SDL_Keycode teclas[]={SDLK_RETURN,SDLK_KP_ENTER,SDLK_SPACE};
  for(int k=0;k<3;k++) {
    SDL_Event e={.type=SDL_KEYDOWN};e.key.keysym.sym=teclas[k];teclado_evento(&e);quadroFixture();
    exigirTexto(antes);assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
    assert(imePedidosFixture==ime&&vozPedidosFixture==voz&&celPedidosFixture==cel&&st_estado()==ST_DIGITANDO);
  }
  dedoFixture(SDL_FINGERDOWN,nv_layout_w*.5f,200);quadroFixture();
  dedoFixture(SDL_FINGERUP,nv_layout_w*.5f,200);quadroFixture();
  exigirTexto(antes);assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  assert(imePedidosFixture==ime&&vozPedidosFixture==voz&&celPedidosFixture==cel&&st_estado()==ST_DIGITANDO);
}
static void fecharEditorFixture(void) {
  char antes[TECLADO_LONGO+1];snprintf(antes,sizeof antes,"%s",teclado_texto());
  assert(st_dono()==ST_TECLADO&&st_estado()==ST_DIGITANDO);
  st_teste_evento("X");quadroFixture();exigirTexto(antes);
  assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA&&st_estado()==ST_PARADO);
  assert(alvoExiste(TE_P_CAMPO)&&alvoExiste(TE_P_PRONTO)&&alvoExiste(TE_P_CANCELAR));
}
static void exigirRetanguloFixture(GfxRect a,GfxRect b) {
  assert(fabsf(a.x-b.x)<.0001f&&fabsf(a.y-b.y)<.0001f&&fabsf(a.w-b.w)<.0001f&&fabsf(a.h-b.h)<.0001f);
}
static void escalaMedicaoFixture(void) {
  float anterior=gfx_escala();escalaAtivaFixture=1.27f;medindoFixture=1;
  TePhoneLayout fora=tePhoneMedir();assert(fabsf(gfx_escala()-1.27f)<.0001f);
  escalaAtivaFixture=gfx_escala_ui();TePhoneLayout dentro=tePhoneMedir();
  assert(fabsf(gfx_escala()-gfx_escala_ui())<.0001f);medindoFixture=0;escalaAtivaFixture=anterior;
  exigirRetanguloFixture(fora.painel,dentro.painel);exigirRetanguloFixture(fora.corpo,dentro.corpo);
  exigirRetanguloFixture(fora.rodape,dentro.rodape);exigirRetanguloFixture(fora.qr,dentro.qr);
  assert(fora.n==dentro.n&&fora.nativo==dentro.nativo&&fabsf(fora.total-dentro.total)<.0001f&&fabsf(fora.maximo-dentro.maximo)<.0001f);
  for(int i=0;i<fora.n;i++) {
    assert(fora.controles[i].id==dentro.controles[i].id);exigirRetanguloFixture(fora.controles[i].r,dentro.controles[i].r);
  }
}
static void abrirFixture(int tipo,const char *a,const char *valor,int max,int voz,int cel) {
  if(teclado_aberto()) { SDL_Event e={.type=SDL_KEYDOWN};e.key.keysym.sym=SDLK_ESCAPE;teclado_evento(&e); }
  (void)teclado_resultado();st_fechar(ST_TECLADO);celb_fechar();
  ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)nv_layout_w,(int)nv_layout_h);ponteiro_teste_relogio(SDL_GetTicks);
  const char *segredoAnterior=segredoFixture;
  if(tipo==TECLADO_TIPO_SENHA)segredoFixture=valor;
  teclado_teste_modos(1,voz,cel);teclado_abrir_com("A long caller title for the native phone field","A caller hint with enough text to wrap in a narrow portrait panel.",max,a,valor);teclado_tipo(tipo);
  quadrosFixtureN(90);assert(teTelefoneIme());assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  escalaMedicaoFixture();
  if(tipo==TECLADO_TIPO_EMAIL||tipo==TECLADO_TIPO_SENHA) {
    assert(!strcmp(imeInicialFixture,valor)&&(imeMaxFixture&0xffff)==maxN&&(imeMaxFixture>>16)==tipoIme);
    if(tipo==TECLADO_TIPO_SENHA)assert(teclado_mascarado()&&!segredoDesenhadoFixture);
    editorAtivoFixture();fecharEditorFixture();
  }
  escalaMedicaoFixture();
  assert(alvoExiste(TE_P_CAMPO)&&alvoExiste(TE_P_PRONTO)&&alvoExiste(TE_P_CANCELAR));
  exigirTexto(valor);segredoFixture=segredoAnterior;casosFixture++;
}
static void abrirCampoFixture(void) {
  char valor[TECLADO_LONGO+1];snprintf(valor,sizeof valor,"%s",teclado_texto());int antes=imePedidosFixture;
  PonteiroAlvo antigos[]={alvoFixture(TE_P_CAMPO),alvoFixture(TE_P_PRONTO),alvoFixture(TE_P_CANCELAR)};
  TePhoneLayout anterior=tePhoneMedir();
  tocarFixture(TE_P_CAMPO);assert(imePedidosFixture==antes+1&&st_dono()==ST_TECLADO&&st_estado()==ST_DIGITANDO);
  assert(!strcmp(imeInicialFixture,valor)&&(imeMaxFixture&0xffff)==maxN&&(imeMaxFixture>>16)==tipoIme);
  editorAtivoFixture();
  for(int i=0;i<3;i++) {
    antigos[i].ativar(antigos[i].a,antigos[i].b);quadroFixture();exigirTexto(valor);
    assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA&&st_estado()==ST_DIGITANDO&&imePedidosFixture==antes+1);
    alvosAntigosFixture++;
  }
  if(anterior.maximo>0) {
    float offset=tePhoneOffset;int livre=tePhoneRol.livre;
    PonteiroRolagem rolagem={.fase=PONT_ROL_INICIO,.eixoY=1,.x=(anterior.corpo.x+anterior.corpo.w*.5f)*gfx_escala_ui(),.y=(anterior.corpo.y+anterior.corpo.h*.5f)*gfx_escala_ui()};
    assert(!tePhoneRolagem(&rolagem));rolagem.fase=PONT_ROL_MOVER;rolagem.delta=40;
    assert(!tePhoneRolagem(&rolagem)&&tePhoneOffset==offset&&tePhoneRol.livre==livre);rolagensAntigasFixture++;
  }
}
static void eventoFixture(const char *s) { st_teste_evento(s);quadroFixture(); }
static void resultadoFixture(int r) {
  assert(!teclado_aberto()&&teclado_resultado()==r&&teclado_resultado()==TECLADO_NADA);
  assert(st_dono()==ST_DONO_NENHUM&&!celb_embutido(CELB_TECLADO));
  quadrosFixtureN(4);assert(teclado_resultado()==TECLADO_NADA);
}
static void textoETerminais(void) {
  abrirFixture(TECLADO_TIPO_TEXTO,NULL,"seed",6,0,0);abrirCampoFixture();
  eventoFixture("TAB-1\xc3\xa9\xf0\x9f\x98\x80Z9");exigirTexto("ab1z9");
  eventoFixture("Tx");exigirTexto("x");eventoFixture("Tabcdefghijk");exigirTexto("abcdef");
  eventoFixture("X");exigirTexto("abcdef");assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  abrirCampoFixture();
  st_teste_evento("TNEW2");st_teste_evento("X");quadroFixture();exigirTexto("new2");
  assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);abrirCampoFixture();
  st_teste_evento("T");st_teste_evento("X");quadroFixture();exigirTexto("");
  assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  tocarFixture(TE_P_PRONTO);assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  abrirCampoFixture();eventoFixture("D");assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
  abrirCampoFixture();eventoFixture("DFINAL99");exigirTexto("final9");resultadoFixture(TECLADO_PRONTO);
  st_teste_evento("Dstale");abrirFixture(TECLADO_TIPO_TEXTO,NULL,"new",16,0,0);abrirCampoFixture();quadrosFixtureN(2);exigirTexto("new");
  eventoFixture("Tkeep");fecharEditorFixture();tocarFixture(TE_P_CANCELAR);exigirTexto("keep");resultadoFixture(TECLADO_CANCELOU);
  abrirFixture(TECLADO_TIPO_TEXTO,NULL,"abcd",16,0,0);
  const int ids[]={TE_P_CAMPO,TE_P_APAGAR,TE_P_LIMPAR,TE_P_PRONTO,TE_P_CANCELAR};
  for(int i=0;i<5;i++)deslizarFixture(ids[i]);
  tocarFixture(TE_P_APAGAR);exigirTexto("abc");tocarFixture(TE_P_LIMPAR);exigirTexto("");
  abrirCampoFixture();eventoFixture("Tready");fecharEditorFixture();tocarFixture(TE_P_PRONTO);resultadoFixture(TECLADO_PRONTO);
  abrirFixture(TECLADO_TIPO_TEXTO,"ABCDEF0123456789:","00",16,0,0);abrirCampoFixture();
  eventoFixture("Tabc:defXYZ123!");exigirTexto("ABC:DEF123");fecharEditorFixture();tocarFixture(TE_P_PRONTO);resultadoFixture(TECLADO_PRONTO);
  char longo[TECLADO_LONGO+1],ev[TECLADO_LONGO+2];memset(longo,'x',TECLADO_LONGO);longo[TECLADO_LONGO]=0;
  abrirFixture(TECLADO_TIPO_TEXTO,NULL,"",TECLADO_LONGO,0,0);abrirCampoFixture();
  snprintf(ev,sizeof ev,"T%s",longo);eventoFixture(ev);exigirTexto(longo);
  eventoFixture("X");abrirCampoFixture();exigirTexto(longo);fecharEditorFixture();tocarFixture(TE_P_CANCELAR);resultadoFixture(TECLADO_CANCELOU);
  abrirFixture(TECLADO_TIPO_TEXTO,NULL,"cancel",16,0,0);abrirCampoFixture();int fechos=imeFechosFixture;
  SDL_Event sair={.type=SDL_KEYDOWN};sair.key.keysym.sym=SDLK_ESCAPE;teclado_evento(&sair);
  assert(imeFechosFixture==fechos+1);exigirTexto("cancel");resultadoFixture(TECLADO_CANCELOU);
}
static void senhaEEmail(void) {
  const char *senha="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,_-@!#$%&*+=?/\\:;'\"()[]{}<>^~`|";
  abrirFixture(TECLADO_TIPO_SENHA,senha,"PaSs123!",128,1,1);segredoFixture="PaSs123!";
  quadroFixture();assert(teclado_mascarado()&&!segredoDesenhadoFixture&&!alvoExiste(TE_P_CEL));
  deslizarFixture(TE_P_MASCARA);tocarFixture(TE_P_MASCARA);assert(!teclado_mascarado());
  (void)alvoFixture(TE_P_CAMPO);quadroFixture();assert(segredoDesenhadoFixture>0);
  tocarFixture(TE_P_MASCARA);assert(teclado_mascarado()&&!segredoDesenhadoFixture);abrirCampoFixture();
  segredoFixture=NULL;eventoFixture("DCase123!");exigirTexto("Case123!");resultadoFixture(TECLADO_PRONTO);
  const char *email="abcdefghijklmnopqrstuvwxyz0123456789@._-+";
  abrirFixture(TECLADO_TIPO_EMAIL,email,"user@",120,1,1);
  assert(!alvoExiste(TE_P_CEL));for(int i=0;i<4;i++)assert(alvoFixture(TE_P_ATALHO+i).w>0);
  tocarFixture(TE_P_ATALHO+1);exigirTexto("user@gmail.com");
  abrirCampoFixture();eventoFixture("TUSER+tag@EXAMPLE.invalid");exigirTexto("user+tag@example.invalid");
  fecharEditorFixture();tocarFixture(TE_P_CANCELAR);resultadoFixture(TECLADO_CANCELOU);
  abrirFixture(TECLADO_TIPO_EMAIL,email,"user",8,0,0);tocarFixture(TE_P_ATALHO+1);exigirTexto("user");
  tocarFixture(TE_P_ATALHO);exigirTexto("user.com");tocarFixture(TE_P_PRONTO);resultadoFixture(TECLADO_PRONTO);
}
static void modosFixture(void) {
  for(int voz=0;voz<2;voz++)for(int cel=0;cel<2;cel++) {
    abrirFixture(TECLADO_TIPO_TEXTO,NULL,"seed",64,voz,cel);
    assert(alvoExiste(TE_P_FALAR)==voz&&alvoExiste(TE_P_CEL)==cel);
    if(voz) {
      int antes=vozPedidosFixture;deslizarFixture(TE_P_FALAR);tocarFixture(TE_P_FALAR);
      assert(vozPedidosFixture==antes+1&&st_estado()==ST_OUVINDO);
      eventoFixture("Ppartial");exigirTexto("partial");eventoFixture("Vfinal");exigirTexto("final");
      assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
      tocarFixture(TE_P_FALAR);int ime=imePedidosFixture;eventoFixture("Steclado:negada");
      assert(imePedidosFixture==ime+1&&!strcmp(imeInicialFixture,"final")&&st_estado()==ST_DIGITANDO);
      editorAtivoFixture();fecharEditorFixture();
    }
    if(cel) {
      int antes=celPedidosFixture;deslizarFixture(TE_P_CEL);tocarFixture(TE_P_CEL);
      assert(celPedidosFixture==antes+1&&celb_embutido(CELB_TECLADO));
      revelarQrFixture();
      snprintf(celValorFixture,sizeof celValorFixture,"CELL-42");celPendenteFixture=1;quadroFixture();
      exigirTexto("cell42");assert(!celb_aberto()&&!celPendenteFixture&&teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
      tocarFixture(TE_P_PRONTO);resultadoFixture(TECLADO_PRONTO);
    } else { tocarFixture(TE_P_CANCELAR);resultadoFixture(TECLADO_CANCELOU); }
  }
}
static void substituirPublicoFixture(void) {
  const char *filaAntiga[]={"Tstale","T","X","Dstale"};
  const int tipos[]={TECLADO_TIPO_TEXTO,TECLADO_TIPO_EMAIL,TECLADO_TIPO_SENHA};
  const char *alfabetos[]={"0123456789","abcdefghijklmnopqrstuvwxyz0123456789@._-+","abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!"};
  const char *iniciais[]={"12345","new@a","N3w!"};
  const char *finais[]={"56789","x@y.z","N3w!"};
  const int limites[]={5,8,7};
  for(int tipo=0;tipo<3;tipo++)for(int fila=0;fila<4;fila++) {
    abrirFixture(TECLADO_TIPO_TEXTO,NULL,"seed",64,0,0);abrirCampoFixture();
    int ime=imePedidosFixture,fechos=imeFechosFixture;
    st_teste_evento(filaAntiga[fila]);
    /* Replace solely through the public API: no test cleanup or bridge reset. */
    teclado_abrir_com("Replacement caller","Replacement hint",limites[tipo],alfabetos[tipo],iniciais[tipo]);
    teclado_tipo(tipos[tipo]);quadroFixture();
    exigirTexto(iniciais[tipo]);assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
    assert(maxN==limites[tipo]&&imeFechosFixture==fechos+1);
    if(tipo) {
      assert(imePedidosFixture==ime+1&&st_estado()==ST_DIGITANDO);
      assert(!strcmp(imeInicialFixture,iniciais[tipo])&&(imeMaxFixture&0xffff)==limites[tipo]&&(imeMaxFixture>>16)==tipoIme);
      editorAtivoFixture();fecharEditorFixture();
    } else assert(imePedidosFixture==ime&&st_dono()==ST_DONO_NENHUM);
    abrirCampoFixture();assert(!strcmp(imeInicialFixture,iniciais[tipo]));
    char ev[32];snprintf(ev,sizeof ev,"D%s",finais[tipo]);eventoFixture(ev);exigirTexto(finais[tipo]);resultadoFixture(TECLADO_PRONTO);casosFixture++;
  }
  const int acoes[]={TE_P_PRONTO,TE_P_CANCELAR};
  for(int i=0;i<2;i++) {
    abrirFixture(TECLADO_TIPO_TEXTO,NULL,"seed",64,0,0);
    PonteiroAlvo velho=alvoFixture(acoes[i]);float x=velho.x+velho.w*.5f,y=velho.y+velho.h*.5f;
    dedoFixture(SDL_FINGERDOWN,x,y);quadroFixture();exigirTexto("seed");
    teclado_abrir_com("A long caller title for the native phone field","A caller hint with enough text to wrap in a narrow portrait panel.",5,"0123456789","123");
    teclado_tipo(TECLADO_TIPO_TEXTO);quadroFixture();exigirTexto("123");
    PonteiroAlvo novo=alvoFixture(acoes[i]);
    assert(x>=novo.x&&x<novo.x+novo.w&&y>=novo.y&&y<novo.y+novo.h);
    dedoFixture(SDL_FINGERUP,x,y);quadroFixture();exigirTexto("123");
    assert(teclado_aberto()&&teclado_resultado()==TECLADO_NADA);
    abrirCampoFixture();assert(!strcmp(imeInicialFixture,"123")&&(imeMaxFixture&0xffff)==5);
    eventoFixture("D456");exigirTexto("456");resultadoFixture(TECLADO_PRONTO);casosFixture++;
  }
}
static void gradePreservadaFixture(void) {
  const float telas[][2]={{1920,1080},{1728,1080},{1080,1728},{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  for(int t=0;t<7;t++)for(int ui=0;ui<2;ui++)for(int ime=0;ime<2;ime++)for(int modos=0;modos<4;modos++) {
    if(t>=3&&ime)continue;
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];escalaUiFixture=ui?1.5f:1;
    teclado_teste_modos(ime,modos&1,modos>>1);teclado_abrir_com("Legacy alphabet","",64,NULL,"abc");
    ponteiro_iniciar();ponteiro_teste_toque(1);ponteiro_teste_janela((int)nv_layout_w,(int)nv_layout_h);quadrosFixtureN(90);
    assert(!teTelefoneIme());const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v),letras=0;
    for(int i=0;i<n;i++)if(v[i].focar==focarTecla&&v[i].a<fileirasChar())letras++;
    assert(letras==36);
    PonteiroAlvo alvo={0};for(int i=0;i<n;i++)if(v[i].focar==focarTecla&&v[i].a==0&&v[i].b==0)alvo=v[i];assert(alvo.w>0);
    float x=alvo.x+alvo.w*.5f,y=alvo.y+alvo.h*.5f;
    dedoFixture(SDL_FINGERDOWN,x,y);quadroFixture();dedoFixture(SDL_FINGERUP,x,y);quadroFixture();exigirTexto("abca");
    SDL_Event e={.type=SDL_KEYDOWN};e.key.keysym.sym=SDLK_ESCAPE;teclado_evento(&e);resultadoFixture(TECLADO_CANCELOU);casosFixture++;
  }
}
int main(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  for(int t=0;t<4;t++)for(int ui=0;ui<2;ui++)for(int fonte=0;fonte<2;fonte++)for(int vidro=0;vidro<2;vidro++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];escalaUiFixture=ui?1.5f:1;fonteLargaFixture=fonte;vidroFixture=vidro;
    reduzidasFixture=1;textoETerminais();senhaEEmail();modosFixture();substituirPublicoFixture();
  }
  /* Repeat gestures while the real animation springs run. */
  nv_layout_w=1080;nv_layout_h=2340;escalaUiFixture=1.5f;fonteLargaFixture=0;vidroFixture=0;reduzidasFixture=0;textoETerminais();
  reduzidasFixture=1;gradePreservadaFixture();
  assert(textosFixture>0&&imePedidosFixture>0&&vozPedidosFixture>0&&celPedidosFixture>0&&imeFechosFixture>0&&celFechosFixture>0&&qrDesenhosFixture>0&&alvosAntigosFixture>0&&rolagensAntigasFixture>0);
  printf("teclado_phone_native: PASS %d modal cases, %d DOWN/redraw/UP taps, %d redraw swipes without action, %d rejected stale native-editor callbacks, %d real QR draws, %d real pipeline frames; phone UI100/150, long labels, bridge terminals, password/email, real cellphone ownership, TV/tablet/native-unavailable grade\n",casosFixture,toquesFixture,swipesFixture,alvosAntigosFixture,qrDesenhosFixture,quadrosFixture);
  return 0;
}
