#include "../src/esportesui.c"
#include <assert.h>
/* Navigation/matching fixture: no SDL video/audio runtime is needed. */
Uint32 SDLCALL SDL_GetTicks(void) { return 1000; }
static int fonteAberta,aberturasFonte;
static uint64_t perfilTeste=1;
uint64_t perfis_sessao_geracao(void) { return perfilTeste; }
int perfis_sessao_atual(uint64_t v) { return v==perfilTeste; }
void stream_folha_canais_abrir(const char *titulo) { fontescanais_iniciar(titulo);fonteAberta=1;aberturasFonte++; }
int stream_folha_canais_aberta(void) { return fonteAberta; }
void stream_folha_canais_fechar(void) { fonteAberta=0;fontescanais_limpar(); }
int ctx_aberto(void) { return 0; }
int ponteiro_ok_longo(void) { return 0; }
int segurar_venceu(Uint32 a,Uint32 d,const char *t) { (void)t;return a-d>=600; }
void ilha_atividade(const char *t,float p) { (void)t;(void)p; }
float anim_clamp(float v,float a,float b) { return fmaxf(a,fminf(v,b)); }
void ponteiro_alvo_segurar(PonteiroFn f) { (void)f; }
static unsigned rev=1;
static int ligasProntas=1, visitasOcultas;
float nv_layout_w=1920,nv_layout_h=1080;
static float escalaTeste=1;
float gfx_escala_ui(void) { return escalaTeste; }
float menu_barra_borda(void) { return 0; }
static int habilitados[16], pediuLigas, pediuEventos, favorito, tecladoResultado;
static EspLiga lTeste={"nba","NBA",""};
static EspEvento eTeste;
const char *i18n(const char *s) { return s; }
int layout_modo_mobile(void) { return 1; }
void esportes_abrir_sessao(void) { for(int i=0;i<16;i++) habilitados[i]=1; }
int esportes_habilitado(int i) { return i>=0 && i<16 && habilitados[i]; }
int esportes_habilitar(int i,int v) { habilitados[i]=v; return 1; }
void esportes_cancelar(void) {}
int esportes_favorito(const EspEvento *e,int p) { (void)e;(void)p;return favorito; }
int esportes_alternar_favorito(const EspEvento *e,int p) { (void)e;(void)p;favorito=!favorito;rev++;return 1; }
void esportes_pedir_ligas(int m,int a) { (void)m;(void)a;pediuLigas++;ligasProntas=0;rev++; }
void esportes_pedir_eventos(int m,const EspLiga *l,int a) { (void)m;(void)l;(void)a;pediuEventos++;rev++; }
void esportes_passo(void) {}
int esportes_carregando(void) { return 0; }
int esportes_erro(void) { return 0; }
unsigned esportes_revisao(void) { return rev; }
int esportes_ligas_n(void) { return ligasProntas; }
const EspLiga *esportes_liga(int i) { return i>=0 && i<ligasProntas?&lTeste:NULL; }
int esportes_eventos_n(void) { return 1; }
const EspEvento *esportes_evento(int i) { return i==0?&eTeste:NULL; }
const char *teclado_alfabeto(void) { return "abc"; }
void teclado_abrir_com(const char *t,const char *d,int n,const char *a,const char *i) { (void)t;(void)d;(void)n;(void)a;(void)i; }
void teclado_evento(const SDL_Event *e) { (void)e; }
void teclado_atualizar(float dt,Uint32 a) { (void)dt;(void)a; }
int teclado_resultado(void) { return tecladoResultado; }
const char *teclado_texto(void) { return "does not match"; }
void teclado_esquecer(void) { tecladoResultado=0; }
void guia_carregar(void) {}
void epg_iniciar(void) {}
void epg_passo(void) {}
void xtepg_passo(void) {}
int epg_estado(void) { return EPG_PRONTO; }
unsigned epg_revisao(void) { return 1; }
unsigned guia_busca_revisao(void) { return 1; }
int guia_busca_carregando(void) { return 0; }
int guia_busca_elegivel(int i) { return i>=0 && i<3; }
int guia_canal_campos(int i,const char **id,const char **n,const char **l,const char **c,const char **b) {
  (void)l;(void)c;(void)b;
  static const char *ids[]={"wrong","match","broadcaster","hidden"},*names[]={"Sports One","Sports Two","ESPN HD","Hidden channel"};
  if(i<0 || i>=4)return 0;
  *id=ids[i];*n=names[i];return 1;
}
const char *guia_busca_conta(int i) { return i==1?"Account Two":"Account One"; }
int guia_busca_programas(int i,time_t d,time_t a,int p,EpgProg *out,int cap) {
  (void)d;(void)a;(void)p;(void)cap;
  if(i==3)visitasOcultas++;
  if(i==2)return 0;
  *out=(EpgProg){eTeste.inicio,eTeste.inicio+7200,i==0?"Celtics vs Cavaliers":"Celtics vs Magic"};return 1;
}
/* Record calls from the real draw routine, without an SDL/GL runtime. */
static int imagens,imagensDesenhadas,alvosGrade;
float gfx_escala(void) { return 1; }
float gfx_escala_entrar(void) { return 1; }
void gfx_escala_sair(float v) { (void)v; }
void gfx_recorte(float x,float y,float w,float h) { (void)x;(void)y;assert(w>0 && h>0); }
void gfx_sem_recorte(void) {}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float px,float py,float raio,float a,float b,float c,float d) {
  (void)m;(void)f;(void)px;(void)py;(void)raio;(void)a;(void)b;(void)c;(void)d;
  assert(r.w>0 && r.h>0);if(t)imagensDesenhadas++;
}
void plrui_cartao(GfxRect r,float raio,float a) { (void)raio;(void)a;assert(r.w>0 && r.h>0); }
void plrui_linha_foco(GfxRect r,float raio,float a) { plrui_cartao(r,raio,a); }
void plrui_pilula_foco(GfxRect r,float a) { plrui_cartao(r,0,a); }
void plrui_botao_repouso(GfxRect r,float a) { plrui_cartao(r,0,a); }
int plrui_tinta(void) { return 245; }
float ajustes_raio_poster_px(void) { return 20; }
int ajustes_relogio_12h(void) { return 0; }
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)e;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){0,(int)strlen(s)*12,24,0,0};
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) {
  assert(w>0);TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=(int)w;return t;
}
void txt_desenhar(TxtLinha t,float x,float y) { (void)t;(void)x;(void)y; }
GLuint tex_obter_larg(const char *url,float w) { assert(url[0] && w>0);imagens++;return 1; }
float tex_aspecto(const char *url) { (void)url;return 2; }
int tex_marca_escura(const char *url) { (void)url;return 0; }
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int i,int b) {
  (void)x;(void)y;(void)w;(void)h;(void)f;(void)a;(void)i;(void)b;
}
void ponteiro_alvo_faixa(float x,float y,float w,float h,float y0,float y1,PonteiroFn f,PonteiroFn a,int i,int b) {
  assert(x>=xLista && x+w<=xLista+larguraLista+0.1f);alvosGrade++;
  (void)y0;(void)y1;ponteiro_alvo(x,y,w,h,f,a,i,b);
}
void ponteiro_rolagem(PonteiroRolagemFn fn) { (void)fn; }
void teclado_desenhar(Uint32 a) { (void)a; }
static void tecla(SDL_Keycode k) { SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;esportesui_evento(&e); }
int main(void) {
  strcpy(eTeste.id,"event");strcpy(eTeste.nome,"Celtics at Magic");strcpy(eTeste.modalidade,"basketball");strcpy(eTeste.ligaId,"nba");
  eTeste.nParticipantes=2;eTeste.estado=1;eTeste.inicio=time(NULL)-60;
  strcpy(eTeste.participantes[0].nome,"Celtics");strcpy(eTeste.participantes[1].nome,"Magic");
  eTeste.nEmissoras=1;strcpy(eTeste.emissoras[0],"ESPN");
  esportesui_iniciar();assert(pagina==ESP_LIGAS && pediuLigas==1 && !nItens);
  assert(foco==-FOCO_MODALIDADE);
  tecla(SDLK_UP);assert(foco==-1);tecla(SDLK_RIGHT);assert(foco==-2);tecla(SDLK_LEFT);assert(foco==-1);
  pointerTopoAgir(2,0);assert(pagina==ESP_CONFIG);
  pointerAtivar(0,0);assert(!habilitados[0]);tecla(SDLK_ESCAPE);assert(pagina==ESP_LIGAS && modalidade==1);
  assert(foco==-FOCO_MODALIDADE-1);
  tecla(SDLK_RIGHT);assert(modalidade==2 && pediuLigas==3);
  esportesui_atualizar(0.016f,SDL_GetTicks());assert(!nItens);
  ligasProntas=7;rev++;
  esportesui_atualizar(0.016f,SDL_GetTicks());assert(nItens==7 && foco==-FOCO_MODALIDADE-2);
  /* Grid arrows use columns, including the incomplete final row. */
  tecla(SDLK_DOWN);assert(foco==0 && colunas==5);
  tecla(SDLK_RIGHT);assert(foco==1);tecla(SDLK_DOWN);assert(foco==6);
  tecla(SDLK_UP);assert(foco==1);tecla(SDLK_UP);assert(foco==-FOCO_MODALIDADE-2);
  /* Hidden sports are skipped in both remote directions. */
  tecla(SDLK_LEFT);assert(modalidade==1);tecla(SDLK_LEFT);assert(modalidade==1 && esportesui_quer_sair());
  /* A touch tab opens the league grid, not another sport-selection list. */
  pointerModalidadeAgir(2,0);ligasProntas=1;rev++;
  esportesui_atualizar(0.016f,SDL_GetTicks());tecla(SDLK_DOWN);assert(foco==0);
  tecla(SDLK_RETURN);assert(pagina==ESP_EVENTOS && pediuEventos==1);
  esportesui_atualizar(0.016f,SDL_GetTicks());assert(nItens==1);
  /* A normal event tap opens the shared Sources sheet without navigation. */
  pointerAtivar(0,0);assert(pagina==ESP_EVENTOS && fonteAberta && aberturasFonte==1);
  assert(!fontescanais_estado()->n && fontescanais_estado()->buscando);
  esportesui_atualizar(0.016f,SDL_GetTicks());
  assert(nCanais==1 && !procurando && !visitasOcultas);
  assert(fontescanais_estado()->n==1 && !fontescanais_estado()->buscando);
  assert(!strcmp(fontescanais_estado()->titulo,eTeste.nome));
  assert(!strcmp(fontescanais_estado()->canais[0].conta,"Account Two"));
  assert(!strcmp(fontescanais_estado()->canais[0].id,"match"));
  assert(fontescanais_escolher(0,time(NULL))==1);fonteAberta=0;
  char id[128];assert(esportesui_pediu_canal(id,sizeof id) && !strcmp(id,"match"));
  assert(!esportesui_pediu_canal(id,sizeof id));
  /* Team favorites remain a secondary action, not a page before Sources. */
  tecla(SDLK_MENU);assert(pagina==ESP_DETALHE);foco=1;tecla(SDLK_RETURN);assert(favorito);
  tecla(SDLK_ESCAPE);assert(pagina==ESP_EVENTOS);
  pointerAtivar(0,0);assert(!fontescanais_estado()->n && !fontescanais_estado()->escolha[0]);
  fontescanais_atualizar();esportesui_atualizar(0.016f,1000);assert(fontescanais_estado()->n==1);
  /* Upcoming programmes still allow live tuning; profile changes invalidate choices. */
  FonteCanal futuro={0};strcpy(futuro.id,"future");futuro.inicio=time(NULL)+3600;
  fontescanais_publicar(&futuro,1,0);assert(fontescanais_escolher(0,time(NULL))==1);
  futuro.inicio=time(NULL)-60;fontescanais_publicar(&futuro,1,0);assert(fontescanais_escolher(0,time(NULL))==1);
  perfilTeste++;assert(!esportesui_pediu_canal(id,sizeof id) && !fontescanais_estado()->ativo);
  stream_folha_canais_fechar();
  pointerTopoAgir(1,0);assert(entradaTexto);tecladoResultado=1;esportesui_atualizar(0.016f,1000);assert(!entradaTexto && !nItens);
  /* Suggestions cannot crowd a later exact match out of the bounded list. */
  nCanais=0;for(int i=0;i<64;i++){char c[24];snprintf(c,sizeof c,"suggestion-%d",i);adicionar(c,c,"Account",NULL,0);}
  EpgProg p={time(NULL)-60,time(NULL)+60,"Celtics vs Magic"};adicionar("exact","Channel","Account",&p,1);
  assert(nCanais==64 && canais[0].confirmado && !strcmp(canais[0].id,"exact"));
  /* Real drawing requests league and participant artwork and lays out cards
   * inside the content area on TV, portrait phone and enlarged interface. */
  strcpy(lTeste.logo,"https://art.example/nba.png");
  strcpy(eTeste.participantes[0].logo,"https://art.example/celtics.png");
  strcpy(eTeste.participantes[1].logo,"https://art.example/magic.png");
  const float tamanhos[][3]={{1920,1080,1},{1080,2300,1},{1920,1080,1.5},{1080,2300,1.5}};
  for (int i=0;i<4;i++) {
    nv_layout_w=tamanhos[i][0];nv_layout_h=tamanhos[i][1];escalaTeste=tamanhos[i][2];
    abrir(ESP_LIGAS);modalidade=2;ligasProntas=7;montar();foco=0;imagens=imagensDesenhadas=alvosGrade=0;
    esportesui_desenhar(1000);assert(imagens && imagensDesenhadas==imagens && alvosGrade);
    abrir(ESP_EVENTOS);montar();imagens=imagensDesenhadas=0;
    esportesui_desenhar(1000);assert(imagens==2 && imagensDesenhadas==2);
  }
  for(int i=0;i<16;i++)habilitados[i]=0;
  abrir(ESP_CONFIG);tecla(SDLK_ESCAPE);assert(modalidade==-1 && !nItens);
  esportesui_desenhar(1000); /* No stale leagues when every sport is hidden. */
  esportesui_fechar();puts("sports remote/touch actions, settings and guide matches: passed");return 0;
}
