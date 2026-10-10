/* Production drawing with font/GL boundary doubles; this is not a screenshot test.
 * AVISOS: public panel, long/expanded/empty lists, crash prompt states and decisions.
 * SALVOS: production tab strip, row/grid/landscape placement/renderers, direct scroll.
 * STREAMS: production header measurement/chip renderers, five/six control widths.
 * Full stream list drawing and Social/model orchestration need the shot fixtures. */
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
static int cortando, nBotoesTeste, nTextos;
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
int txt_largura(TxtEstilo e, const char *s) { (void)e; return (int)strlen(i18n(s)) * 12; }
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

void gfx_vidro_pilula_cheia(GfxRect r,float raio,float f,float a) { (void)f;gfx_cor(r,raio,1,1,1,a); }
void gfx_vidro_folha(GfxRect r,float raio,float a) { gfx_cor(r,raio,1,1,1,a); }
void gfx_luz_canto(GfxRect r,float raio,float x,float y,float alc,float cr,float cg,float cb,float a) { (void)x;(void)y;(void)alc;gfx_cor(r,raio,cr,cg,cb,a); }
int ponteiro_ativo(void) { return 1; }
#if defined(TESTE_STREAMS)
int video_pode_forcar_sdr(void) { return sdrTeste; }
static void rodar(void) {
  float x=NV_TELA_W-FOLHA_W-FOLHA_MARGEM,lx=x+FOLHA_PAD_E,rw=FOLHA_W-FOLHA_PAD_E-FOLHA_PAD_D;
  assert(x>=FOLHA_MARGEM-.01f&&x+FOLHA_W<=NV_TELA_W-FOLHA_MARGEM+.01f);
  for(sdrTeste=0;sdrTeste<=1;sdrTeste++)for(int f=0;f<nBotoes();f++) {
    grupo=-1;foco=f;medirCabecalho();
    float bx=lx+rw;
    for(int i=0;i<nBotoes();i++)bx-=bwCab[i]+(i?gapCab:0);
    float tx=lx+FOLHA_TXT;
    TxtLinha t=txt_linha_corta(TXT_ILHA_TITULO,"Fontes",243,242,239,255,titMaxCab);
    assert(tx+t.w+24<=bx+.01f);
    txt_desenhar_alpha(t,tx,94,1);
    for(int i=0;i<nBotoes();i++) {
      GfxRect r={bx,FOLHA_CAB_Y,bwCab[i],FOLHA_CHIP_H};
      chipFolha(r,rotCab[i],rotWCab[i],icoCab[i],i==f,botaoLigado(botaoDe(i)),1);
      limites(r,0);bx+=bwCab[i]+gapCab;
    }
  }
}
#elif defined(TESTE_SALVOS)
static int estiloTeste;
int sorg_ordem(void) { return SORG_ORDEM_SALVOU; }
int sorg_grupo(void) { return SORG_GRUPO_PROGRESSO; }
const AgItem *agenda_lista(int i) { (void)i;assert(0);return NULL; }
int agendaui_painel_grupo_de(const AgItem *a) { (void)a;assert(0);return 0; }
int avisos_lista_linhas(void) { assert(0);return 0; }
int sorg_estilo(void) { return estiloTeste; }
int sorg_social(void) { assert(0); return 0; }
int sorg_n_categorias(void) { return 0; }
int sorg_categoria_id(int i) { (void)i;return 0; }
int sorg_categoria_indice(int i) { (void)i;return -1; }
const char *sorg_categoria_nome_id(int i) { (void)i;return ""; }
float ctx_inline_t(void) { return 0; }
int ctx_inline_painel_ativo(void) { return 0; }
int ctx_inline_foco_rect(float w,float h,GfxRect *r) { (void)w;(void)h;(void)r;assert(0);return 0; }
void ctx_inline_recorte(GfxRect r) { (void)r;assert(0); }
float ctx_inline_altura(float w,float h) { (void)w;return h; }
void ctx_inline_desenhar(float x,float y,float w,float h,float a) { (void)x;(void)y;(void)w;(void)h;(void)a;assert(0); }
int ctx_aberto(void) { return 0; }
int reacao_painel_aberta(void) { return 0; }
int ajustes_tinta_foco2(void) { return 0; }
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
static void rodar(void) {
  assert(SP_X>=24-.01f&&SP_X+SP_W<=NV_TELA_W-24+.01f);
  nLinhas=99;nAg=99;spPont=1;foco=SP_FOCO_ABAS;
  for(int i=0;i<SP_ABA_N;i++) {
    aba=i;pilIni=0;nBotoesTeste=0;
    float maxT=SP_W-SP_ABAS_X-SP_PAD-abasLargura(0)-28;
    assert(maxT>0);
    TxtLinha t=txt_linha_corta(TXT_ILHA_TITULO,i18n(rotuloAba(i)),243,242,239,255,maxT);
    txt_desenhar_alpha(t,SP_X+SP_PAD,SP_TIT_Y,1);
    desenhaAbas(0,1,SP_W-SP_ABAS_X-SP_PAD-t.w-28);
    assert(nBotoesTeste==SP_ABA_N+1);
    float esquerda=NV_TELA_W;for(int b=0;b<nBotoesTeste;b++)esquerda=fminf(esquerda,botoesTeste[b].x);
    assert(SP_X+SP_PAD+t.w+28<=esquerda+.01f);
  }
  /* The actual saved-row renderers use the production list/grid placement.
   * This omits the public Social/model orchestration, covered by shot fixtures. */
  static SPLinha dadosTeste[40];linhas=dadosTeste;nLinhas=40;aba=SP_ABA_SALVOS;aberto=1;pop=tecladoPara=editando=0;grupoAtual=SORG_GRUPO_PROGRESSO;
  memset(linhas,0,sizeof dadosTeste);memset(animFoco,0,sizeof animFoco);memset(menuId,0,sizeof menuId);
  for(int i=0;i<nLinhas;i++) { SPLinha *l=&linhas[i];memset(l->titulo,'T',sizeof l->titulo-1);memset(l->meta,'M',sizeof l->meta-1);strcpy(l->txtRestante,"T12 E100 - 120 minutes remaining");strcpy(l->txtVisto,"already viewed for a long duration");l->progresso=i%2?73:0;l->nota=86;l->ano=2026; }
  for(estiloTeste=0;estiloTeste<SORG_ESTILO_N;estiloTeste++) {
    montarLayout();toquerol_limpar(&toquePainel);scrollY=0;
    toquerol_vincular(&toquePainel,(GfxRect){SP_X,listaTopo(),SP_W,SP_LISTA_BASE-listaTopo()},gfx_escala(),0,toquePainelMax(),1,&scrollY);
    arrastar(&toquePainel,toquePainelRolar,1);assert(scrollY>0);
    gfx_recorte(SP_X,listaTopo(),SP_W,SP_LISTA_BASE-listaTopo());
    float y=listaTopo()+SP_FOCO_AR+linhas[nLinhas-1].ly-scrollY;
    assert(y+linhas[nLinhas-1].lh<=SP_LISTA_BASE+.01f);
    if(estiloTeste==SORG_ESTILO_LISTA)desenhaLinha(nLinhas-1,0,y,1);
    else if(estiloTeste==SORG_ESTILO_GRADE)desenhaCelulaGrade(nLinhas-1,0,y,1);
    else desenhaCelulaPaisagem(nLinhas-1,0,y,1);
    gfx_sem_recorte();
  }

}
#elif defined(TESTE_AVISOS)
/* None of these inactive toast/model boundaries may be reached by the fixture. */
int avisodisp_sessao_tem(const char *s) { (void)s;assert(0);return 0; }
void avisodisp_sessao_por(const char *s) { (void)s;assert(0); }
int ajustes_envio_auto(void) { assert(0);return 0; }
int recomenda_item(int i,RecItem *r) { (void)i;(void)r;assert(0);return 0; }
const char *ilha_forte(char *dst,size_t n,const char *s) { (void)dst;(void)n;(void)s;assert(0);return ""; }
void ilha_avisar_ex(const IlhaAvisoEx *a) { (void)a;assert(0); }
const char *rec_frase(const RecItem *r) { (void)r;assert(0);return ""; }
void rec_quando_texto(char *dst,size_t n,long long s) { (void)dst;(void)n;(void)s;assert(0); }
int cat_indice_por_imdb(const char *s) { (void)s;assert(0);return -1; }
const CatItem *cat_item(int i) { (void)i;assert(0);return NULL; }
const AgItem *agenda_registro(const char *s) { (void)s;assert(0);return NULL; }
int agenda_dias(const char *s) { (void)s;assert(0);return 0; }
float botao_largura(const char *s,const char *ico,int p) { (void)ico;(void)p;return txt_largura(TXT_G21B,s)+48; }
void botao_pilula(GfxRect r,const char *s,const char *ico,float f,int p,int al,float a) {
  (void)s;(void)ico;(void)f;(void)p;(void)al;gfx_cor(r,.5f,1,1,1,a);
}
static void rodar(void) {
  assert(AVP_X>=24-.01f&&AVP_X+AVP_W<=NV_TELA_W-24+.01f);
  n=AV_MAX;aberto=1;entrada=1;cartao=0;cartaoA=0;toastPendente=0;foco=0;rol=0;
  memset(itens,0,sizeof itens);
  for(int i=0;i<n;i++) { itens[i].tipo=i%6;memset(itens[i].titulo,'T',sizeof itens[i].titulo-1);memset(itens[i].texto,'a',sizeof itens[i].texto-1); }
  avisos_desenhar(5000);
  int antes=foco;arrastar(&toqueAvisos,toqueAvisosRolar,1);assert(foco==antes&&rol>0);
  nBotoesTeste=0;avisos_desenhar(5000);
  int ultimo=0;for(int i=0;i<nBotoesTeste;i++)if(alvoA[i]==n)ultimo=1;assert(ultimo);
  gfx_sem_recorte();
  /* A focused channel notice expands to its full allowed body and remains scrollable. */
  foco=3;rol=0;toquerol_limpar(&toqueAvisos);avisos_desenhar(5000);avisos_desenhar(5000);
  antes=foco;arrastar(&toqueAvisos,toqueAvisosRolar,1);assert(foco==antes);
  nBotoesTeste=0;avisos_desenhar(5000);ultimo=0;for(int i=0;i<nBotoesTeste;i++)if(alvoA[i]==n)ultimo=1;assert(ultimo);
  /* Resize an open/free-scrolling sheet and re-draw using fresh bounds. */
  float larguraAntes=nv_layout_w,alturaAntes=nv_layout_h;
  if(nv_layout_h>nv_layout_w&&nv_layout_h/nv_layout_w>=1.95f) {
    nv_layout_w=alturaAntes;nv_layout_h=larguraAntes;avisos_desenhar(5000);
    assert(toqueAvisos.regiao.x>=0&&toqueAvisos.regiao.x+toqueAvisos.regiao.w<=NV_TELA_W);
    assert(rol<=toqueAvisos.maximo);arrastar(&toqueAvisos,toqueAvisosRolar,1);
    nBotoesTeste=0;avisos_desenhar(5000);ultimo=0;for(int i=0;i<nBotoesTeste;i++)if(alvoA[i]==n)ultimo=1;assert(ultimo);
    nv_layout_w=larguraAntes;nv_layout_h=alturaAntes;avisos_desenhar(5000);assert(rol<=toqueAvisos.maximo);
  }
  n=0;rol=0;toquerol_limpar(&toqueAvisos);avisos_desenhar(5000);assert(toqueAvisos.maximo==0);
  /* Real pending opener and the existing consent/sending/success/failure footer. */
  aberto=0;entrada=0;cartao=0;cartaoA=1;cartaoPendente=1;strcpy(cartaoId,"fixture:crash");strcpy(crashQuando,"2026-10-09 12:34:56");
  avisos_mostrar_se_houver();assert(cartao&&!cartaoPendente);
  for(int estado=0;estado<4;estado++) {
    envioEstado=estado;nBotoesTeste=0;avisos_desenhar(5000);
    assert(nBotoesTeste==(estado==0?3:2));
    assert(crashPhone.corpo.h>0&&crashPhone.rolagem.escala==zoom);
    if(crashPhone.rolagem.maximo>0) {
      int antesEnvio=envioEstado,antesCartao=cartao;arrastar(&crashPhone.rolagem,crashPhoneRolar,1);
      assert(envioEstado==antesEnvio&&cartao==antesCartao);
      nBotoesTeste=0;avisos_desenhar(5000);assert(nBotoesTeste==(estado==0?3:2));
    }
    GfxRect b=telefonecartao_botao_r(&crashPhone,0);
    assert(b.y>=crashPhone.corpo.y+crashPhone.corpo.h);
  }
  /* Empty service configuration refuses the actual Send path without spawning a worker. */
  assert(!NV_REC_URL[0]);envioEstado=0;crashPhoneAcao(0,0);assert(envioEstado==3&&cartao);
  crashPhoneAcao(0,1);assert(!cartao);
  cartaoPendente=1;avisos_mostrar_se_houver();assert(cartao&&crashPhone.offset==0&&!crashPhone.rolagem.livre);
  envioEstado=0;crashPhoneAcao(1,0);assert(!cartao);
  cartaoA=0;
}
#endif
int main(void) {
  float dimensoes[][2]={{1080,1920},{1080,2340},{2340,1080}},escalas[]={1,1.2f,1.3f,1.5f};
  for(int d=0;d<3;d++)for(int s=0;s<4;s++)for(int v=0;v<2;v++)for(int l=0;l<2;l++) {
    nv_layout_w=dimensoes[d][0];nv_layout_h=dimensoes[d][1];zoom=escalas[s];escala=zoom;vidroTeste=v;traducaoTeste=l;nBotoesTeste=nTextos=cortando=0;limiteTexto=0;
    rodar();
  }
  puts("Phone bounded sheets: PASS");return 0;
}
