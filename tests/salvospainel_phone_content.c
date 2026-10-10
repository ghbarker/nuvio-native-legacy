/* Production saved-panel draw functions and update loop with inert boundaries.
 * Font measurements are supplied by the shell driver from bundled TTF files.
 * The inline context renderer is a height/action contract double; its actual
 * labels and interaction need the dedicated context fixture and GL captures.
 * This intentionally reports/fails known phone regressions until source fixes.
 */
#define TESTE_SALVOS 1
#define NV_TOUCH_PREVIEW 1
#define SDL_MAIN_HANDLED 1
#ifdef _WIN32
#include <time.h>
static struct tm *telefone_localtime_r(const time_t *t, struct tm *out) {
  struct tm *p = localtime(t); if (p) *out = *p; return p ? out : NULL;
}
#define localtime_r telefone_localtime_r
#endif
#if defined(TESTE_STREAMS)
#include "../src/streams.c"
#elif defined(TESTE_SALVOS)
#include "../src/salvospainel.c"
#elif defined(TESTE_AVISOS)
#include "../src/avisos.c"
#else
#error choose STREAMS, SALVOS or AVISOS
#endif
#ifdef gfx_recorte
#undef gfx_recorte
#endif
#include <assert.h>
float nv_layout_w = 1080, nv_layout_h = 2340;
float gfx_tex_aspect_atual, gfx_card_forcar_cover_atual;
int txt_pendentes, anim_politica_reduzida;
static float escala = 1, zoom = 1;
static int vidroTeste, sdrTeste, traducaoTeste;
static GfxRect recorte, botoesTeste[32];
static int alvoA[32], alvoB[32];
static PonteiroFn alvoFocar[32], alvoAtivar[32];
static int cortando, nBotoesTeste, nTextos;
static float limiteTexto;
static float telaW(void) { return nv_layout_w / escala; }
static float telaH(void) { return nv_layout_h / escala; }
static int foraTeste;
static void limites(GfxRect r,int clipping) {
  assert(r.w>=0&&r.h>=0&&isfinite(r.x)&&isfinite(r.y));
  if(clipping&&cortando){float dir=fminf(r.x+r.w,recorte.x+recorte.w),esq=fmaxf(r.x,recorte.x),top=fmaxf(r.y,recorte.y),bottom=fminf(r.y+r.h,recorte.y+recorte.h);if(dir<=esq||bottom<=top)return;r.x=esq;r.w=dir-esq;r.y=top;r.h=bottom-top;}
  if(r.x<-.01f||r.x+r.w>telaW()+.01f||r.y<-.01f||r.y+r.h>telaH()+.01f)foraTeste++;
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
static int fonteTeste, escalaIndice, falhas;
typedef struct { int fonte, escala, estilo, altura; float largura; char texto[512]; } FontMedida;
static FontMedida fontMedidas[20000];
static int nFontMedidas;
static int fontEstilo(TxtEstilo e) {
  return e == TXT_ILHA_SUB ? 0 : e == TXT_ILHA_SEG ? 1 : e == TXT_MINI ? 2 : e == TXT_ILHA_NOME ? 3 : -1;
}
static int caracteres(const char *s) { int n=0; for(;*s;s++)if(((unsigned char)*s&0xc0)!=0x80)n++;return n; }
static FontMedida *medida(TxtEstilo e,const char *s) {
  int estilo=fontEstilo(e); for(int i=0;i<nFontMedidas;i++)if(fontMedidas[i].fonte==fonteTeste&&fontMedidas[i].escala==escalaIndice&&fontMedidas[i].estilo==estilo&&!strcmp(fontMedidas[i].texto,s))return &fontMedidas[i];return NULL;
}
static void carregarFontes(const char *caminho) {
  FILE *f=fopen(caminho,"rb");char linha[1024];assert(f);
  while(fgets(linha,sizeof linha,f)) {
    int off=0;assert(nFontMedidas<20000);FontMedida *m=&fontMedidas[nFontMedidas];
    assert(sscanf(linha,"%d %d %d %f %d %n",&m->fonte,&m->escala,&m->estilo,&m->largura,&m->altura,&off)==5);
    char *s=linha+off;size_t n=strcspn(s,"\r\n");assert(n<sizeof m->texto);memcpy(m->texto,s,n);m->texto[n]=0;nFontMedidas++;
  } fclose(f);assert(nFontMedidas>0);
}
int txt_largura(TxtEstilo e,const char *s) { FontMedida *m=medida(e,s);return m?(int)(m->largura+.5f):caracteres(s)*12; }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)r;(void)g;(void)b;(void)a;FontMedida *m=medida(e,s);return (TxtLinha){0,txt_largura(e,s),m?m->altura:28,0,0};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) {
  assert(w>0);TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=(int)w;return t;
}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a) {
  if(a<=0)return;limites((GfxRect){x,y,t.w,t.h},1);nTextos++;if(y+t.h>limiteTexto)limiteTexto=y+t.h;
}
void txt_desenhar(TxtLinha t,float x,float y) { txt_desenhar_alpha(t,x,y,1); }
static int blocosLinhas[8], nBlocos;
float txt_bloco_corta(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float lead,float a,int max) {
  char cur[512]="",word[512],candidate[512];int n=1;assert(w>0);
  while(*s) {
    while(*s==' ')s++;if(!*s)break;size_t len=strcspn(s," ");assert(len<sizeof word);memcpy(word,s,len);word[len]=0;s+=len;
    size_t used=strlen(cur);assert(used+len+2<sizeof candidate);snprintf(candidate,sizeof candidate,"%s%s%s",cur,used?" ":"",word);
    if(used&&txt_largura(e,candidate)>w){n++;snprintf(cur,sizeof cur,"%s",word);}else snprintf(cur,sizeof cur,"%s",candidate);
  }
  if(max>0&&n>max)n=max;
  if(a>0){assert(nBlocos<8);blocosLinhas[nBlocos++]=n;txt_desenhar_alpha((TxtLinha){0,(int)w,(int)(n*lead),0,0},x,y,a);}
  (void)r;(void)g;(void)b;return n*lead;
}
float txt_bloco(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float w,float lead,float a,int max) { return txt_bloco_corta(e,s,r,g,b,x,y,w,lead,a,max); }
const char *i18n(const char *s) {
  if(!traducaoTeste)return s;
  if(!strcmp(s,"Fontes"))return "Available playback sources and filters";
  if(!strcmp(s,"Salvos"))return "Previously saved titles";
  if(!strcmp(s,"Atividade"))return "Current activity from friends";
  if(!strcmp(s,"Amigos"))return "Friends and recommendations";
  if(!strcmp(s,"Agenda"))return "Upcoming episodes and reminders";
  if(!strcmp(s,"Avisos"))return "Notifications and recommendations";
  if(!strcmp(s,"Sem HDR"))return "Force standard dynamic range";
  if(!strcmp(s,"Só MP4"))return "Only compatible MP4 containers";
  if(!strcmp(s,"Em cache"))return "Available in the provider cache";
  if(!strcmp(s,"Dublado"))return "Includes a matching dubbed language";
  if(!strcmp(s,"O app fechou sozinho"))return "The application closed unexpectedly during your previous session. The application closed unexpectedly during your previous session. The application closed unexpectedly during your previous session. The application closed unexpectedly during your previous session.";
  return s;
}
Uint32 SDL_GetTicks(void) { return 5000; }
int ajustes_idioma(void) { return 0; }
int ajustes_idioma_ingles(void) { return 1; }
int ajustes_vidro(void) { return vidroTeste; }
int ajustes_tinta_foco(void) { return 0; }
float ajustes_acento_tinta(float *r,float *g,float *b) { if(r)*r=.4f;if(g)*g=.7f;if(b)*b=1;return 0; }
int ajustes_relogio_12h(void) { return 0; }
float txt_tracking(TxtEstilo e,const char *s,int r,int g,int b,float x,float y,float a,float spacing) {
  TxtLinha t=txt_linha(e,s,r,g,b,255);t.w+=(int)((caracteres(s)-1)*spacing+.5f);if(x>=0)txt_desenhar_alpha(t,x,y,a);return t.w;
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
    if(y<0||y+h>telaH()+.01f)foraTeste++; assert(nBotoesTeste<32);botoesTeste[nBotoesTeste]=(GfxRect){x,y,w,h};alvoA[nBotoesTeste]=a;alvoB[nBotoesTeste]=b;alvoFocar[nBotoesTeste]=focar;alvoAtivar[nBotoesTeste]=ativar;nBotoesTeste++;
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

void gfx_vidro_pilula_cheia(GfxRect r,float raio,float f,float a) { (void)f;gfx_cor(r,raio,1,1,1,a); }
void gfx_vidro_folha(GfxRect r,float raio,float a) { gfx_cor(r,raio,1,1,1,a); }
void gfx_luz_canto(GfxRect r,float raio,float x,float y,float alc,float cr,float cg,float cb,float a) { (void)x;(void)y;(void)alc;gfx_cor(r,raio,cr,cg,cb,a); }
int ponteiro_ativo(void) { return 1; }
static int estiloTeste, inlineTeste, inlineFocoTeste;
static float inlineAlturaTeste=440;
const AgItem *agenda_lista(int i) { (void)i;assert(0);return NULL; }
int agendaui_painel_grupo_de(const AgItem *a) { (void)a;assert(0);return 0; }
int avisos_lista_linhas(void) { assert(0);return 0; }
int sorg_estilo(void) { return estiloTeste; }
int sorg_social(void) { return 0; }
int sorg_n_categorias(void) { return 0; }
int sorg_categoria_id(int i) { (void)i;return 0; }
int sorg_categoria_indice(int i) { (void)i;return -1; }
const char *sorg_categoria_nome_id(int i) { (void)i;return ""; }
float ctx_inline_t(void) { return inlineTeste?1:0; }
float ctx_inline_altura(float w,float h) { (void)w;(void)h;return inlineAlturaTeste; }
static GfxRect inlineAcoesTeste[3];
static GfxRect inlineClipTeste;
int ctx_inline_painel_ativo(void) { return inlineTeste; }
int ctx_inline_foco_rect(float w,float h,GfxRect *r) {
  (void)h;
  if (!inlineTeste || !r) return 0;
  *r=(GfxRect){18+inlineFocoTeste*(w-36)/3,inlineAlturaTeste-68,(w-56)/3,50};
  return inlineFocoTeste+1;
}
void ctx_inline_recorte(GfxRect r) { inlineClipTeste=r; }
void ctx_inline_desenhar(float x,float y,float w,float h,float a) {
  (void)h;
  for(int k=0;k<3;k++) {
    inlineAcoesTeste[k]=(GfxRect){x+18+k*(w-36)/3,y+inlineAlturaTeste-68,(w-56)/3,50};
    GfxRect r=inlineAcoesTeste[k];gfx_cor(r,.5f,1,1,1,a);
    ponteiro_alvo_faixa(r.x,r.y,r.w,r.h,inlineClipTeste.y,inlineClipTeste.y+inlineClipTeste.h,NULL,NULL,k,0);
  }
}
int ctx_aberto(void) { return inlineTeste; }
int ajustes_tinta_foco2(void) { return 0; }
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
int perfis_ativo(void) { return 1; }
#endif
void gfx_veu_base(GfxRect r,float raio,float p,float a) { (void)p;gfx_cor(r,raio,1,1,1,a); }
float avisos_lista_y(int i,int f) { (void)i;(void)f;assert(0);return 0; }
float avisos_lista_altura_linha(int i,int f) { (void)i;(void)f;assert(0);return 0; }
int recomenda_social_estrito(void) { return 0; }
int ajustes_social(void) { return 1; }
int recomenda_ativo(void) { return 1; }
int recomenda_n_novas(void) { return 99; }
int recomenda_n_pedidos(void) { return 99; }
int socialvis_n_eventos(void) { return 0; }
int socialvis_n_amigos(void) { return 99; }
int avisos_n_novos(void) { return 99; }
int avisos_lista_n(void) { return 99; }

int sorg_ordem(void) { return SORG_ORDEM_SALVOU; }
int sorg_grupo(void) { return SORG_GRUPO_PROGRESSO; }
void sorg_carregar(void) { }
unsigned sorg_revisao(void) { return 0; }
unsigned cat_revisao_itens(void) { return 0; }
unsigned salvos_revisao(void) { return 0; }
unsigned cw_retido_rev(void) { return 0; }
int cat_n(void) { return 0; }
int agenda_versao(void) { return 0; }
int ajustes_animacoes_reduzidas(void) { return 0; }
void ctx_centro_dica(float x) { (void)x; }
const char *ctx_pediu_categoria(void) { return NULL; }
int ctx_pediu_extra(void) { return -1; }
void reacao_painel_atualizar(float dt, Uint32 agora) { (void)dt;(void)agora; }
int reacao_painel_aberta(void) { return 0; }
int recomenda_aparecer(void) { return REC_APARECER_SIM; }
int recomenda_n(void) { return 0; }
int recomenda_n_sugestoes(void) { return 0; }
void reacao_painel_desenhar(Uint32 agora) { (void)agora; }
int gfx_efeitos_minimos(void) { return 0; }
int ajustes_salvos_no_simkl(void) { return 0; }
const char *simkl_aviso_sem_vinculo(int quer) { (void)quer;return NULL; }
void gfx_sombra_sob(GfxRect s,float foco,float px,float raio,float r,float g,float b,
                     float a,GfxRect painel,float rp,float ap) {
  (void)s;(void)foco;(void)px;(void)raio;(void)r;(void)g;(void)b;(void)a;(void)rp;
  if(ap>0)limites(painel,1);
}
static void conferir(int ok,const char *nome) { if(!ok){printf("FAIL %s\n",nome);falhas++;} }
static void limparDesenho(void) { nTextos=nBotoesTeste=nBlocos=foraTeste=cortando=0;limiteTexto=0; }
static void prepararPainel(void) {
  aberto=1;entrada=entradaP=trocaT=1;entradaV=trocaV=0;
  pop=tecladoPara=editando=pilIni=deIlha=0;scrollY=0;
}
static void chips(void) {
  aba=SP_ABA_SALVOS;nLinhas=8;estiloTeste=SORG_ESTILO_PAISAGEM;spPont=1;foco=SP_FOCO_BARRA;barraFoco=0;
  limparDesenho();desenhaBarra(0,1);assert(nBotoesTeste==4);
  float end=botoesTeste[3].x+botoesTeste[3].w;
  printf("chips font%d W%.0f end%.1f panelRight%.1f\n",fonteTeste,NV_TELA_W,end,SP_X+SP_W);
  for(int k=0;k<4;k++) {
    conferir(botoesTeste[k].x>=SP_X+SP_PAD-.01f&&botoesTeste[k].x+botoesTeste[k].w<=SP_X+SP_W-SP_PAD+.01f,"chip target inside content column");
    conferir(botoesTeste[k].y+botoesTeste[k].h<=listaTopo()-12+.01f,"wrapped chip ends before list");
  }
  conferir(!foraTeste,"chip text and targets inside viewport");
}
static void consentimento(int alcance) {
  prepararPainel();
  aba=SP_ABA_SOCIAL;nSocial=alcance?3:2;foco=0;spPont=1;pop=tecladoPara=editando=0;
  for(int i=0;i<nSocial;i++){social[i].tipo=(unsigned char)(alcance?SPS_ALC_0+i:SPS_CONSENT_NAO+i);social[i].idx=0;}
  limparDesenho();if(alcance)desenhaAlcancePergunta(0,0,1);else desenhaConsentimento(0,0,1);
  printf("%s font%d width%.1f lines",alcance?"alcance":"consent",fonteTeste,SP_INTERNO);for(int i=0;i<nBlocos;i++)printf(" %d",blocosLinhas[i]);
  printf(" bottom%.1f allocation%.1f\n",limiteTexto,socialTopo());
  float promptBottom=limiteTexto;
  conferir(promptBottom<=socialTopo()+.01f,"social prompt ends before response rows");
  for(int i=0;i<nSocial;i++)desenhaBotaoLinha(i,0,topoDe(i),socialAlt(i),1,alcance?ALC_SEG[i]:i?"Sim, pode me mostrar":"Não, não quero aparecer",NULL,NULL,0);
  limparDesenho();spainel_desenhar(5000);
  int target=0;
  for(int i=0;i<nBotoesTeste;i++)if(alvoA[i]==0&&botoesTeste[i].y>=listaTopo()&&fabsf(botoesTeste[i].w-SP_LINHA_W)<.01f) {
    target=1;conferir(botoesTeste[i].y>=listaTopo()+SP_FOCO_AR+promptBottom-.01f,"actual Social response target starts after prompt");
  }
  conferir(target,"actual Social response target is visible");
  conferir(!foraTeste,"public Social draw and targets inside viewport");
}
static void acordeao(void) {
  static SPLinha dados[8];memset(dados,0,sizeof dados);linhas=dados;nLinhas=8;aba=SP_ABA_SALVOS;aberto=1;entrada=entradaP=1;trocaT=1;
  pop=tecladoPara=editando=pilIni=0;foco=0;grupoAtual=SORG_GRUPO_PROGRESSO;inlineTeste=1;
  marcaCatN=0;marcaRevCat=marcaRevSalvos=marcaRevOrg=marcaRevRet=0;agVer=0;
  snprintf(dados[0].id,sizeof dados[0].id,"%s","fixture-saved");snprintf(menuId,sizeof menuId,"%s",dados[0].id);
  for(int i=0;i<nLinhas;i++){snprintf(dados[i].titulo,sizeof dados[i].titulo,"Fixture saved title %d",i);snprintf(dados[i].meta,sizeof dados[i].meta,"%s","2026 - long summary");}
  for(estiloTeste=SORG_ESTILO_GRADE;estiloTeste<=SORG_ESTILO_PAISAGEM;estiloTeste++) {
    escala=zoom;montarLayout();scrollY=velY=0;toquerol_limpar(&toquePainel);
    float janela=SP_LISTA_BASE-listaTopo();
    for(inlineFocoTeste=0;inlineFocoTeste<3;inlineFocoTeste++) {
    escala=1;for(int i=0;i<120;i++)spainel_atualizar(1.0f/60,5000+i);escala=zoom;
    conferir(!toquePainel.livre,"changed context action restores focus following");
    limparDesenho();gfx_recorte(SP_X,listaTopo(),SP_W,janela);
    ctx_inline_recorte((GfxRect){SP_X,listaTopo(),SP_W,janela});
    spainel_desenhar(6000);
    printf("accordion style%d action%d H%.0f scroll%.2f window%.1f actionBottom%.1f base%.1f\n",estiloTeste,inlineFocoTeste,NV_TELA_H,scrollY,janela,inlineAcoesTeste[inlineFocoTeste].y+inlineAcoesTeste[inlineFocoTeste].h,SP_LISTA_BASE);
    GfxRect acao=inlineAcoesTeste[inlineFocoTeste];
    conferir(acao.y>=listaTopo()&&acao.y+acao.h<=SP_LISTA_BASE+.01f,"expanded action fits after production update");
    int alvo=0;for(int i=0;i<nBotoesTeste;i++)if(alvoA[i]==inlineFocoTeste)alvo=1;
    conferir(alvo,"expanded focused action has visible target");
    conferir(!foraTeste,"public expanded saved draw and targets inside viewport");
    toquerol_vincular(&toquePainel,(GfxRect){SP_X,listaTopo(),SP_W,janela},gfx_escala(),0,toquePainelMax(),1,&scrollY);
    if (toquePainel.maximo>37.25f) {
      float antes=scrollY,livre=antes>=37.25f?antes-37.25f:antes+37.25f;
      livre=toquerol_clamp(livre,0,toquePainel.maximo);
      PonteiroRolagem e={PONT_ROL_INICIO,1,0,0,(toquePainel.regiao.x+10)*zoom,(toquePainel.regiao.y+10)*zoom};
      conferir(toquePainelRolar(&e),"actual saved scroll callback accepts active inline gesture");
      e.fase=PONT_ROL_MOVER;e.delta=(antes-livre)*zoom;
      conferir(toquePainelRolar(&e),"actual inline gesture preserves fractional delta");
      e.fase=PONT_ROL_SOLTAR;conferir(toquePainelRolar(&e),"inline swipe release is accepted");
      conferir(fabsf(scrollY-livre)<.01f,"gesture uses cached drawing scale");
      escala=1;spainel_atualizar(1.0f/60,7000);escala=zoom;
      conferir(fabsf(scrollY-livre)<.01f,"fractional free swipe survives same-action update");
    }
    gfx_sem_recorte();
    }
  }
  inlineTeste=0;menuId[0]=0;
}
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
int main(int argc,char **argv) {
  assert(argc>=2);carregarFontes(argv[1]);int probe=argc>2&&!strcmp(argv[2],"--probe");
  const float telas[][2]={{1080,1920},{1080,2340},{2160,1080},{2340,1080}},zooms[]={1,1.2f,1.3f,1.5f};
  for(fonteTeste=0;fonteTeste<2;fonteTeste++)for(escalaIndice=0;escalaIndice<4;escalaIndice++)
    for(size_t t=0;t<sizeof telas/sizeof *telas;t++)for(vidroTeste=0;vidroTeste<2;vidroTeste++) {
      nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];zoom=escala=zooms[escalaIndice];
      printf("case font%d %.0fx%.0f UI%.0f glass%d\n",fonteTeste,nv_layout_w,nv_layout_h,zoom*100,vidroTeste);
      chips();consentimento(0);consentimento(1);acordeao();
    }
  printf("%d saved-content regression failure(s); font/GL/inline boundaries documented\n",falhas);return probe?0:falhas?1:0;
}
#endif

/* Unused model/request paths abort if the fixture accidentally enters them. */
#ifdef __APPLE__
#define SIMBOLO(nome) "_" #nome
#else
#define SIMBOLO(nome) #nome
#endif
#define INATIVO(nome) __attribute__((used,externally_visible)) void inativo_##nome(void) __asm__(SIMBOLO(nome)); __attribute__((used,externally_visible)) void inativo_##nome(void) { fprintf(stderr,"unexpected boundary: %s\n",#nome); abort(); }
INATIVO(agenda_montar)
INATIVO(agenda_n)
INATIVO(atividade_marcar_origem)
INATIVO(avisos_lista_item)
INATIVO(cat_fileira)
INATIVO(cat_item)
INATIVO(cat_n_fileiras)
INATIVO(ctx_abrir_salvo)
INATIVO(ctx_abrir_social)
INATIVO(ctx_inline_pedir)
INATIVO(cw_retido_exclui)
INATIVO(rec_nome_exibicao)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_alcance)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_contatos)
#endif
INATIVO(recomenda_definir_nome)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_identidade_estado)
#endif
INATIVO(recomenda_identidade_letterboxd_declarar)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_identidade_op)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_identidade_op_de)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_identidade_situacao)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_item)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_pedido)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_sugestao)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recresp_assistida)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recresp_ler)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recresp_revisao)
#endif
INATIVO(salvos_item)
INATIVO(salvos_mesmo_titulo)
INATIVO(salvos_n)
INATIVO(salvos_uniao)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(socialvis_amigo_indice)
#endif
INATIVO(socialvis_atualizar)
INATIVO(socialvis_evento)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(socialvis_revisao)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(socialvis_ultima_enviada)
#endif
INATIVO(sorg_categoria_de)
INATIVO(sorg_criar_categoria)
INATIVO(sorg_definir_grupo)
INATIVO(sorg_mover)
INATIVO(sorg_renomear_categoria)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(teclado_aberto)
#endif
INATIVO(teclado_atualizar)
INATIVO(teclado_resultado)
INATIVO(teclado_texto)
INATIVO(agendaui_painel_fio)
INATIVO(avisos_lista_desenhar_ptr)
INATIVO(cat_indice_por_imdb)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(extras_caminho_marca_nome)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(gfx_anel_fora)
#endif
INATIVO(perfis_item_ativo)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(rec_avatar)
#endif
INATIVO(rec_frase)
INATIVO(rec_quando_texto)
INATIVO(rec_selo_pessoa)
INATIVO(rec_selo_pessoa_largura)
INATIVO(rec_sugestao_origem)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_identidade_trakt)
#endif
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_identidade_usuario_letterboxd)
#endif
INATIVO(recomenda_meu_nome)
INATIVO(recomenda_perfil)
INATIVO(socialvis_amigo)
INATIVO(socialvis_enviada_rotulo)
INATIVO(socialvis_ep)
INATIVO(socialvis_meu_estado)
INATIVO(socialvis_n_ao_vivo)
INATIVO(socialvis_quando)
INATIVO(socialvis_status)
INATIVO(socialvis_verbo)
INATIVO(svd_ponto_vivo)
INATIVO(teclado_desenhar)
INATIVO(tex_obter)
/* Direct phone controls retain the existing dispatcher; unrelated branches
 * are still inert and fail loudly if a layout fixture enters them. */
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(recomenda_aceitar)
INATIVO(recomenda_recusar)
INATIVO(recomenda_responder_alcance)
INATIVO(recomenda_identidade_unir)
INATIVO(recomenda_identidade_separar)
INATIVO(recomenda_identidade_simkl_unir)
INATIVO(recomenda_identidade_simkl_separar)
INATIVO(recomenda_identidade_letterboxd_separar)
INATIVO(recomenda_identidade_op_limpar)
INATIVO(recomenda_identidade_op_limpar_de)
#endif
INATIVO(recomenda_responder_aparecer)
INATIVO(recomenda_marcar_vistas)
INATIVO(recomenda_minha_exibicao)
INATIVO(recomenda_adicionar_sugerido)
INATIVO(recenviar_abrir_amigos)
INATIVO(pessoas_abrir)
INATIVO(dados_gravar)
INATIVO(avisos_marcar_lidos)
INATIVO(recomenda_pedir_agora)
INATIVO(agenda_atualizar_seguidas)
INATIVO(sorg_definir_social)
INATIVO(sorg_definir_estilo)
INATIVO(sorg_definir_ordem)
INATIVO(sorg_excluir_categoria)
#ifndef NV_SAVED_SOCIAL_DOUBLES_ONLY
INATIVO(teclado_abrir_com)
#endif
