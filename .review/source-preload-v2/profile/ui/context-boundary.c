// Actual context-menu, pointer and layout modules. Tracker doubles keep a
// dispatched operation pending across the UI reset; no network or display.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ctxmenu.c"
#ifndef SDL_DISABLE
#define SDL_DISABLE 0
#endif
#include "ponteiro_sdl.h"

static Uint32 now = 1000;
static int activated, profile = 1, queuedOwner, queued, remoteWrites[3];
static int polls, mirrors, rebuilds, backendState = CTX_PENDENTE;
static int watchCount;
static CatItem title(const char *id, const char *name) {
  CatItem ci; memset(&ci, 0, sizeof ci);
  snprintf(ci.imdb, sizeof ci.imdb, "%s", id);
  snprintf(ci.titulo, sizeof ci.titulo, "%s", name);
  snprintf(ci.tipo, sizeof ci.tipo, "movie");
  ci.naLista = 1;
  return ci;
}
void SDL_AddEventWatch(SDL_EventFilter f, void *u) { (void)f; (void)u; watchCount++; }
int cat_n(void) { return 0; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
int cat_indice_por_imdb(const char *id) { (void)id; return -1; }
int cat_imdb_na_lista(const char *id) { (void)id; return 1; }
int cat_historico_estado_id(const char *id, const char *type) { (void)id; (void)type; return 0; }
void cat_historico_definir_id(const char *id, const char *type, int seen) {
  (void)id; (void)type; (void)seen; mirrors++;
}
void cat_definir_na_lista(int i, int saved) { (void)i; (void)saved; mirrors++; }
int cat_definir_na_lista_imdb(const char *id, int saved) { (void)id; (void)saved; mirrors++; return 1; }
void cat_zerar_progresso(int i) { (void)i; mirrors++; }
int cat_tirar_continuar(const char *id) { (void)id; mirrors++; return 1; }
int salvos_tem(const char *id) { (void)id; return 1; }
int salvos_definir(const CatItem *ci, int saved) { (void)ci; (void)saved; return 1; }
void salvos_id_titulo(const char *id, char *out, size_t n) { snprintf(out, n, "%s", id); }
int trakt_ativo(void) { return 1; }
int trakt_e_a_seguir(const char *id) { (void)id; return 0; }
static int enqueue(const char *id) {
  assert(id && id[0] && !queued); queued = 1; queuedOwner = profile;
  backendState = CTX_PENDENTE; return 1;
}
int trakt_watchlist_tipo(const char *id, const char *type, int add) {
  (void)type; (void)add; return enqueue(id);
}
int trakt_assistido_tipo(const char *id, const char *type, int mark) {
  (void)type; (void)mark; return enqueue(id);
}
int trakt_operacao_estado(int type) { (void)type; polls++; return backendState; }
static void finishBackend(void) {
  assert(queued); remoteWrites[queuedOwner]++; queued = 0; backendState = CTX_CONFIRMADA;
}
int simkl_ativo(void) { return 0; }
int simkl_na_plantowatch(const char *id) { (void)id; return 0; }
int simkl_e_a_seguir(const char *id) { (void)id; return 0; }
int simkl_lista_tipo(const char *id, const char *type, int add) { (void)type; (void)add; return enqueue(id); }
int simkl_lista_estado(void) { polls++; return backendState; }
int simkl_playback_remover(const char *id) { (void)id; return 1; }
const char *simkl_aviso_sem_vinculo(int enabled) { (void)enabled; return NULL; }
int cwo_conta_a_seguir(const char *id) { (void)id; return 0; }
int ajustes_animacoes_reduzidas(void) { return 1; }
int ajustes_salvos_no_trakt(void) { return 1; }
int ajustes_salvos_no_simkl(void) { return 0; }
int ajustes_cw_ok_toca(void) { return 0; }
int home_foco_retomada(void) { return 0; }
int home_item_focado(HomeItem *out) { (void)out; return 0; }
void home_retomar_dispensar(void) {}
void home_pedir_abrir(void) {}
void desc_remontar_fileiras(void) { rebuilds++; }
int desc_tirar_continuar(const char *id, int season, int episode) {
  (void)id; (void)season; (void)episode; return 1;
}
void atividade_salvo(const CatItem *ci, int saved) { (void)ci; (void)saved; }
int ilhasalvar_perguntar(const CatItem *ci, int add) { (void)ci; (void)add; return 0; }
void ilhasalvar_aviso(const CatItem *ci, int saved) { (void)ci; (void)saved; }
void ilhaacao_feita(const IlhaAcao *action) { (void)action; }
int recomenda_ativo(void) { return 0; }
int recenviar_abrir(const CatItem *ci) { (void)ci; return 0; }
int visto_destinos(void) { return 0; }
int visto_titulo(const char *id, const char *type, const int *seasons, int n, int seen, int destinations) {
  (void)id; (void)type; (void)seasons; (void)n; (void)seen; (void)destinations; return 1;
}
void tirarremoto_executar(const char *id, const char *key, int hide) { (void)id; (void)key; (void)hide; }
int tirarremoto_capturar(TirarRemotoPedido *out, const char *id) { (void)out; (void)id; return 0; }
void tirarremoto_executar_pedido(const TirarRemotoPedido *p, const char *key, int hide) {
  (void)p; (void)key; (void)hide;
}
void prog_chave(char *out, unsigned n, const char *id, int season, int episode) {
  (void)season; (void)episode; snprintf(out, n, "%s", id);
}
void prog_remover(const char *key) { (void)key; }
void prog_marcar_removido(const char *id) { (void)id; }
void prog_ocultar_continuar(const char *id) { (void)id; }
int fil_tipo(const char *key) { (void)key; return 0; }
int fil_estilos(const char *key, int *types, const char **labels, int n) {
  (void)key; (void)types; (void)labels; (void)n; return 0;
}
int fil_estilo_linhas(const char *key, FilEstiloLinha *rows, int n) { (void)key; (void)rows; (void)n; return 0; }
int fil_definir_tipo(const char *key, int type) { (void)key; (void)type; return 1; }
int lst_fixada(const LstLista *l) { (void)l; return 0; }
int lst_na_home(const LstLista *l) { (void)l; return 0; }
int lst_alternar_fixada(const LstLista *l) { (void)l; return 1; }
int lst_alternar_home(const LstLista *l) { (void)l; return 1; }
int lst_aceita_home(const LstLista *l, const char **why) { (void)l; (void)why; return 1; }
void lst_abrir(const LstLista *l, const char *type) { (void)l; (void)type; }
void ctxinfo_abrir(const CatItem *ci) { (void)ci; }
int segurar_venceu(Uint32 t, Uint32 start, const char *screen) { (void)t; (void)start; (void)screen; return 0; }
const char *i18n(const char *s) { return s; }
float gfx_escala(void) { return 1; }
float gfx_escala_ui(void) { return 1; }
float gfx_opacidade_grupo = 1;
void gfx_sem_recorte(void) {}
void gfx_cor(GfxRect r, float radius, float red, float green, float blue, float alpha) {
  (void)r; (void)radius; (void)red; (void)green; (void)blue; (void)alpha;
}
static Uint32 clockNow(void) { return now; }
static void activate(int a, int b) { (void)a; (void)b; activated++; }
static void deliver(const SDL_Event *e) { (void)e; }
static void key(SDL_Keycode symbol) {
  SDL_Event e; SDL_zero(e); e.type = SDL_KEYUP; e.key.keysym.sym = SDLK_RETURN;
  ctx_evento(&e); e.type = SDL_KEYDOWN; e.key.keysym.sym = symbol; ctx_evento(&e);
}
static void frame(void) {
  ponteiro_quadro(now);
  ponteiro_alvo(100,100,500,250,NULL,activate,1,0);
  // app.c's Home CAMADA_SE(ctx_aberto()); an inline menu has no floating body.
  if (ctx_aberto()) {
    ponteiro_camada(); ponteiro_alvo(0,0,NV_TELA_W,NV_TELA_H,NULL,NULL,0,0);
  }
  ponteiro_desenhar(); now += 16;
}
static void tap(void) {
  SDL_Event e; SDL_zero(e); e.tfinger.touchId = 1; e.tfinger.fingerId = 2;
  e.tfinger.x = 300.0f/NV_TELA_W; e.tfinger.y = 200.0f/NV_TELA_H;
  e.type = SDL_FINGERDOWN; now += 16; assert(ponteiro_evento(&e,deliver));
  e.type = SDL_FINGERUP; now += 16; assert(ponteiro_evento(&e,deliver));
}
static void boundary(void) { ponteiro_abrir_sessao(); ctx_abrir_sessao(); }
int main(void) {
  CatItem a = title("tt0000001","Profile A saved title");
  CatItem b = title("tt0000002","Profile B saved title");
  ponteiro_teste_relogio(clockNow); ponteiro_teste_janela(1920,1080);
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  ctx_inline_pedir(1); ctx_abrir_salvo(&a); ctx_atualizar(1.0f/60.0f,now);
  assert(ctx_aberto() && ctx_inline_painel_ativo() && ctx_do_painel());
  assert(ctx_titulo() && !strcmp(ctx_titulo()->imdb,a.imdb));
  assert(ctx_inline_t() == 1.0f);
  frame(); tap(); assert(activated == 0);
  boundary(); assert(!ctx_aberto() && !ctx_inline_painel_ativo() && !ctx_titulo());
  assert(ctx_inline_t() == 0.0f && !ctx_info_caixa(NULL,NULL,NULL));
  frame(); tap(); assert(activated == 1);
  puts("PASS: orphaned inline Saved menu closes immediately and Home receives a fresh real tap");

  ctx_inline_pedir(1); ctx_abrir_salvo(&a); key(SDLK_RETURN);
  assert(queued && queuedOwner == 1);  // actual OP_LISTA dispatch from ctx_evento
  int priorPolls = polls, priorMirrors = mirrors, priorRebuilds = rebuilds;
  boundary(); profile = 2; assert(queued && queuedOwner == 1);
  finishBackend(); assert(remoteWrites[1] == 1 && remoteWrites[2] == 0);
  ctx_atualizar(1.0f/60.0f,now);
  assert(polls == priorPolls && mirrors == priorMirrors && rebuilds == priorRebuilds);
  ctx_abrir_salvo(&b); assert(ctx_titulo() && !strcmp(ctx_titulo()->imdb,b.imdb));
  assert(!ctx_inline_painel_ativo());  // old inline request cannot enter B's menu
  ctx_atualizar(1.0f/60.0f,now);
  assert(polls == priorPolls && mirrors == priorMirrors && rebuilds == priorRebuilds);
  boundary();
  puts("PASS: actual context dispatch survives UI reset; its delayed result cannot mirror into the new profile");

  CtxExtra extra = { .rot = "Already watched", .juntaAssistido = 1 };
  profile = 1; ctx_abrir_social(&a,&extra,1); key(SDLK_DOWN); key(SDLK_RETURN);
  assert(queued && queuedOwner == 1 && pendExtra == 0);
  boundary(); profile = 2; finishBackend(); ctx_atualizar(1.0f/60.0f,now);
  assert(ctx_pediu_extra() == -1 && !ctx_aberto());
  puts("PASS: completed old watched action cannot release a social follow-up in the new profile");

  ctx_abrir_salvo(&b); key(SDLK_DOWN); key(SDLK_RETURN); boundary();
  assert(!ctx_pediu_categoria());
  LstLista l; memset(&l,0,sizeof l); snprintf(l.titulo,sizeof l.titulo,"Private list");
  ctx_abrir_lista(&l); key(SDLK_RETURN); boundary(); assert(!ctx_pediu_lista() && !ctx_lista_alterou());
  ctx_atualizar(1.0f/60.0f,now); assert(watchCount == 1);
  puts("PASS: pending category/list requests clear and the SDL hold observer remains installed once");
  return 0;
}
