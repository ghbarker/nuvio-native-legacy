#include "support.h"
#include "ilhasalvar.c"
#include "ilhaacao.c"
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 100; }
static int island_prompt, saves, saved_profile, undos;
static char saved_id[24];
int tex_falhou(const char *s) { (void)s; return 0; }
GLuint tex_obter_larg(const char *s, float w) { (void)s; (void)w; return 0; }
void ilha_acao(const char *icon, const char *text, int type, const char *poster, const GfxRect *from, const IlhaModal *modal) {
  (void)icon; (void)text; (void)type; (void)poster; (void)from; (void)modal;
}
void ilha_avisar_ex(const IlhaAvisoEx *e) { assert(e && e->chave); island_prompt = 1; }
int ilha_tem(const char *key) { (void)key; return island_prompt; }
int ilha_modal_aberto(void) { return 0; }
int ilha_modal_visivel(void) { return 0; }
int sessao_logada(void) { return 1; }
int salvos_n(void) { return 0; }
int trakt_ativo(void) { return 0; }
int simkl_ativo(void) { return 0; }
int ajustes_salvos_no_trakt(void) { return 0; }
int ajustes_salvos_no_simkl(void) { return 0; }
void ajustes_definir_salvos_destino(int d) { (void)d; }
int salvos_definir(const CatItem *ci, int enter) { assert(enter); saves++; saved_profile = perfis_ativo(); snprintf(saved_id, sizeof saved_id, "%s", ci->imdb); return 1; }
void atividade_salvo(const CatItem *ci, int enter) { (void)ci; (void)enter; }
int trakt_watchlist_tipo(const char *id, const char *type, int enter) { (void)id; (void)type; (void)enter; assert(0); return 0; }
int simkl_lista_tipo(const char *id, const char *type, int enter) { (void)id; (void)type; (void)enter; assert(0); return 0; }
int cat_definir_na_lista_imdb(const char *id, int enter) { (void)id; (void)enter; return 1; }
void desc_remontar_fileiras(void) {}
static void outgoing(void) { ilhasalvar_abrir_sessao(); ilhaacao_abrir_sessao(); island_prompt = 0; }
static void undo(const void *data) { assert(*(const int *)data == 42); undos++; }
int main(void) {
  fixture_init(); perfis_ao_abrir_sessao(outgoing);
  CatItem a = {0}; strcpy(a.imdb, "tt1000001"); strcpy(a.titulo, "Private A title"); strcpy(a.tipo, "movie");
  assert(ilhasalvar_perguntar(&a, 1));
  int data = 42; IlhaAcao action = {0}; action.frase = "Previous A action"; action.desfazer = undo; action.ctx = &data; action.ctxN = sizeof data;
  ilhaacao_feita(&action); assert(ilhaacao_tem_desfazer());
  perfis_definir_ativo(2); assert(!ilhasalvar_pergunta_aberta() && !pendente.imdb[0]);
  ilhasalvar_passo(1000); ilhasalvar_acao("salvar:onde", 1); ilhaacao_botao(1);
  assert(!saves && !undos && !ilhaacao_tem_desfazer());
  puts("PASS: clearing A’s unconfirmed save prompt cannot auto-save its title or run old Undo under B");
  perfis_definir_ativo(1); assert(ilhasalvar_perguntar(&a, 1));
  ilhasalvar_acao("salvar:onde", 1); assert(saves == 1 && saved_profile == 1 && !strcmp(saved_id, a.imdb));
  assert(ilhaacao_tem_desfazer()); perfis_definir_ativo(2); ilhasalvar_passo(2000); ilhaacao_botao(1);
  assert(saves == 1 && !undos && !ilhaacao_tem_desfazer());
  puts("PASS: a confirmed save stays committed to A; opening B neither undoes nor repeats it");
  fixture_cleanup();
}
