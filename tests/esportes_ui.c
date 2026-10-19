#include "../src/esportesui.c"
#include <assert.h>
static unsigned rev=1;
static int ligasProntas=1;
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
const EspLiga *esportes_liga(int i) { return i==0?&lTeste:NULL; }
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
int guia_canal_campos(int i,const char **id,const char **n,const char **l,const char **c,const char **b) {
  (void)l;(void)c;(void)b;
  static const char *ids[]={"wrong","match","broadcaster"},*names[]={"Sports One","Sports Two","ESPN HD"};
  if(i<0 || i>=3)return 0;
  *id=ids[i];*n=names[i];return 1;
}
const char *guia_busca_conta(int i) { return i==1?"Account Two":"Account One"; }
int guia_busca_programas(int i,time_t d,time_t a,int p,EpgProg *out,int cap) {
  (void)d;(void)a;(void)p;(void)cap;
  if(i==2)return 0;
  *out=(EpgProg){eTeste.inicio,eTeste.inicio+7200,i==0?"Celtics vs Cavaliers":"Celtics vs Magic"};return 1;
}
static void tecla(SDL_Keycode k) { SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;esportesui_evento(&e); }
int main(void) {
  strcpy(eTeste.id,"event");strcpy(eTeste.nome,"Celtics at Magic");strcpy(eTeste.modalidade,"basketball");strcpy(eTeste.ligaId,"nba");
  eTeste.nParticipantes=2;eTeste.estado=1;eTeste.inicio=time(NULL)-60;
  strcpy(eTeste.participantes[0].nome,"Celtics");strcpy(eTeste.participantes[1].nome,"Magic");
  eTeste.nEmissoras=1;strcpy(eTeste.emissoras[0],"ESPN");
  esportesui_iniciar();assert(nItens==16);
  /* Normal remote controls: top controls move in the direction pressed. */
  tecla(SDLK_UP);assert(foco==-1);tecla(SDLK_RIGHT);assert(foco==-2);tecla(SDLK_LEFT);assert(foco==-1);
  /* Same toggle path used by pointer activation, then normal Back. */
  pointerTopoAgir(2,0);assert(pagina==ESP_CONFIG);
  pointerAtivar(0,0);assert(!habilitados[0]);tecla(SDLK_ESCAPE);assert(pagina==ESP_HOME && nItens==15);
  pointerAtivar(0,0);assert(pagina==ESP_LIGAS && pediuLigas==1);
  esportesui_atualizar(0.016f,SDL_GetTicks());assert(!nItens && foco==0);
  ligasProntas=1;rev++;
  esportesui_atualizar(0.016f,SDL_GetTicks());assert(nItens==1 && foco==0);
  tecla(SDLK_RETURN);assert(pagina==ESP_EVENTOS && pediuEventos==1);
  esportesui_atualizar(0.016f,SDL_GetTicks());assert(nItens==1);
  tecla(SDLK_RETURN);assert(pagina==ESP_DETALHE);
  foco=1;tecla(SDLK_RETURN);assert(favorito);
  foco=0;tecla(SDLK_RETURN);assert(pagina==ESP_CANAIS);
  esportesui_atualizar(0.016f,SDL_GetTicks());
  assert(nCanais==2 && !procurando);
  assert(canais[0].confirmado && !strcmp(canais[0].id,"match"));
  assert(!strcmp(canais[0].fonte,"Account Two"));assert(!canais[1].confirmado);
  foco=0;tecla(SDLK_RETURN);char id[128];assert(esportesui_pediu_canal(id,sizeof id) && !strcmp(id,"match"));
  assert(!esportesui_pediu_canal(id,sizeof id));
  tecla(SDLK_ESCAPE);assert(pagina==ESP_DETALHE);tecla(SDLK_ESCAPE);assert(pagina==ESP_EVENTOS);
  pointerTopoAgir(1,0);assert(entradaTexto);tecladoResultado=1;esportesui_atualizar(0.016f,SDL_GetTicks());assert(!entradaTexto && !nItens);
  /* Suggestions cannot crowd a later exact match out of the bounded list. */
  nCanais=0;for(int i=0;i<64;i++){char c[24];snprintf(c,sizeof c,"suggestion-%d",i);adicionar(c,c,"Account",NULL,0);}
  EpgProg p={time(NULL)-60,time(NULL)+60,"Celtics vs Magic"};adicionar("exact","Channel","Account",&p,1);
  assert(nCanais==64 && canais[0].confirmado && !strcmp(canais[0].id,"exact"));
  esportesui_fechar();puts("sports remote/touch actions, settings and guide matches: passed");return 0;
}
