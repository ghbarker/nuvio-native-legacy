/* Throwaway diagnostic, outside the product checkout.
 * Links the real perfis, catalogo, contalib and vistoep implementations.
 * UI notifications, storage and remote access are inert infrastructure stubs.
 * The transition calls the actual catalog/history cleanup APIs used by app.c.
 * This is a module integration reproduction, not a full Android app run.
 */
#include "catalogo.h"
#include "contalib.h"
#include "perfis.h"
#include "progresso.h"
#include "vistoep.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int m, const char *s) { (void)m; return s; }
int ajustes_idioma(void) { return 0; }
int ajustes_idioma_ingles(void) { return 0; }
const char *dados_dir(void) { return ""; }
const char *sessao_usuario(void) { return "diagnostic-household"; }
int dados_gravar(const char *name, const char *text) {
  (void)name; (void)text; return 1;
}
const char *desc_genero_pt(const char *s) { return s; }
int prog_ler(ProgRegistro *out, int max) { (void)out; (void)max; return 0; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}

static void publish_one(const char *id, const char *title) {
  CatItem item = {0};
  CatFileira row = {0};
  snprintf(item.imdb, sizeof item.imdb, "%s", id);
  snprintf(item.tipo, sizeof item.tipo, "movie");
  snprintf(item.titulo, sizeof item.titulo, "%s", title);
  snprintf(row.chave, sizeof row.chave, "diagnostic_movie_popular");
  snprintf(row.titulo, sizeof row.titulo, "Popular");
  row.n = 1;
  cat_definir_tudo(&item, 1, &row, 1);
}

static int failures;
static void expect(const char *name, int actual, int wanted) {
  int ok = actual == wanted;
  printf("%s: actual=%d expected=%d => %s\n", name, actual, wanted,
         ok ? "PASS" : "FAIL (profile isolation expectation)");
  failures += !ok;
}

int main(int argc, char **argv) {
  int control = 1;
  (void)argc; (void)argv;
  uint64_t oldEpoch;
  int empty_library, empty_watched;
  puts(control ? "POSITIVE CONTROL: explicitly clear private runtime state"
               : "CURRENT TRANSITION: product runtime state is retained");
  perfis_definir_ativo(1);
  cat_historico_contexto(sessao_usuario(), perfis_ativo());
  publish_one("tt2000001", "Profile A public catalog title");
  assert(contalib_ler_biblioteca(
    "[{\"content_id\":\"tt1000001\",\"content_type\":\"movie\","
    "\"name\":\"Profile A private saved movie\",\"added_at\":1791924000000}]") == 1);
  assert(contalib_aplicar_catalogo() == 1);
  assert(cat_indice_por_imdb("tt1000001") >= 0);
  assert(contalib_ler_vistos(
    "[{\"content_id\":\"tt1000002\",\"content_type\":\"series\","
    "\"season\":1,\"episode\":1,\"watched_at\":1791924000000}]") == 1);
  contalib_aplicar_vistos();
  assert(vistoep_estado("tt1000002", 1, 1) == 1);
  oldEpoch = perfis_sessao_geracao();
  cat_historico_definir_id("tt1000003", "movie", 1);
  assert(cat_historico_estado_id("tt1000003", "movie") == 1);

  /* Real profile selection plus the relevant current app transition cleanup:
   * sync_reaplicar_ajustes() changes title history ownership;
   * desc_esquecer_catalogo_perfil() publishes an empty catalog.
   * Other app cleanup does not call contalib_esquecer or vistoep_esquecer.
   */
  perfis_definir_ativo(2);
  cat_historico_contexto(sessao_usuario(), perfis_ativo());
  cat_definir_tudo(NULL, 0, NULL, 0);
  assert(perfis_ativo() == 2);
  assert(cat_n() == 0);
  if (control) { contalib_esquecer(); vistoep_esquecer(); }
  assert(!perfis_sessao_atual(oldEpoch));
  assert(vistoep_fonte_se_sessao("tt1000002", 1, 1, 1, 1791924000000LL, NULL, oldEpoch) == 0);

  /* Profile B's successful, legitimately empty library/watched API payloads. */
  empty_library = contalib_ler_biblioteca("[]");
  empty_watched = contalib_ler_vistos("[]");
  printf("Profile B [] reader results: library=%d watched=%d\n",
         empty_library, empty_watched);
  expect("Profile B private account library count", contalib_n(), 0);
  expect("Profile B private account watched count", contalib_n_vistos(), 0);
  expect("Profile B A-only episode watched state", vistoep_estado("tt1000002", 1, 1), -1);
  expect("Profile B title history isolation (existing working guard)",
         cat_historico_estado_id("tt1000003", "movie"), -1);

  publish_one("tt2000002", "Profile B public catalog title");
  assert(cat_n() == 1);
  assert(cat_indice_por_imdb("tt1000001") < 0);
  /* The real sync_passo() invokes this before testing whether sync finished. */
  contalib_reconciliar();
  expect("Profile B catalog contains only its published title", cat_n(), 1);
  expect("Profile B catalog excludes A-only saved movie",
         cat_indice_por_imdb("tt1000001"), -1);
  if (cat_indice_por_imdb("tt1000001") >= 0) {
    const CatItem *leaked = cat_item(cat_indice_por_imdb("tt1000001"));
    printf("Observed injected item: %s; title=%s; naLista=%d\n",
           leaked->imdb, leaked->titulo, leaked->naLista);
  }
  printf("Observed profile-isolation expectation failures: %d\n", failures);
  uint64_t epochB = perfis_sessao_geracao();
  assert(vistoep_fonte_se_sessao("tt3000001", 2, 3, 1, 1791924000000LL, NULL, epochB) == 1);
  assert(vistoep_estado("tt3000001", 2, 3) == 1);
  perfis_definir_ativo(1);
  vistoep_esquecer();
  assert(vistoep_fonte_se_sessao("tt3000001", 2, 3, 1, 1791924000000LL, NULL, epochB) == 0);
  assert(vistoep_estado("tt3000001", 2, 3) == -1);
  puts("PASS: delayed A/B watched replies cannot repopulate a new session map");
  contalib_esquecer(); vistoep_esquecer(); cat_definir_tudo(NULL, 0, NULL, 0);
  cat_quadro(); cat_quadro();
  return failures ? 1 : 0;
}
