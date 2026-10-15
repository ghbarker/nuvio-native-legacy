#define main unused_discovery_latch_main
#include "latch.c"
#undef main

/* Keep the real reader and its retained declarations. ASan fake-stack UAR
 * exposes the escaped local URL after lerManifesto returns. */
static Decl retained[4], second[4];
int main(void) {
  int total = 0, promoted = 0;
  assert(descSessaoVincular(nv_perfil_sessao_ler()));
  {
    int n = lerManifesto(0, BASE, retained, 4, 1, &total, &promoted, addons_versao());
    assert(n == 2 && total == 2);
  }
  /* A second parse, filtering/compaction, and later request preparation must
   * retain each declaration's original URL, rather than a reader stack slot. */
  assert(!strcmp(retained[0].base, BASE));
  assert(lerManifesto(0, "https://other.example/second", second, 4, 1, &total, &promoted, addons_versao()) == 2);
  assert(!strcmp(second[0].base, "https://other.example/second"));
  assert(!strcmp(retained[0].base, BASE));
  retained[0] = retained[1];
  assert(!strcmp(retained[0].id, "cat_b"));
  assert(!strcmp(retained[0].base, BASE));
  char request[NV_ADDON_PEDIDO_MAX];
  int n = nv_addon_url(request, sizeof request, retained[0].base, "/catalog/movie/cat_b.json");
  assert(n > 0 && strstr(request, BASE) == request && strstr(request, "/cat_b.json"));
  puts("PASS: actual manifest declaration owns URL after reader return and compaction");
  return 0;
}
