/* All current VOD fixture assertions run first against actual cache/addons.
 * Only source selection, explicit epoch capture and a deterministic tick
 * hook differ in its review copy. Product/original fixture files stay intact.
 */
#include "perfis.h"
#define main original_vod_main
#include "current-vod-fixture.inc"
#undef main
void fixture_epoch_advance(void);
void fixture_epoch_flip_on_tick(void);

static void miss(const FontecacheEscopo *s) {
  Stream *out = (Stream *)1;
  int count = -1;
  Uint32 age = 1;
  assert(fontecache_vod_pegar("ttActor:1:2", "series", "", s,
                             &out, &count, &age) == FC_NADA);
  assert(!out && count == 0 && age == 0);
}
static void hit(const FontecacheEscopo *s, const char *url) {
  Stream *out = NULL;
  int count = 0;
  Uint32 age;
  assert(fontecache_vod_pegar("ttActor:1:2", "series", "", s,
                             &out, &count, &age) == FC_ACERTO);
  assert(count == 1 && out && !strcmp(out[0].url, url));
  free(out);
}
int main(void) {
  assert(original_vod_main() == 0);
  Stream old = {0}, current = {0};
  strcpy(old.url, "https://actor.invalid/old-A.mp4");
  strcpy(current.url, "https://actor.invalid/current-A.mp4");
  fontecache_vod_limpar();
  conta = "conta-a"; perfil = 1;
  fixture_epoch_advance();
  FontecacheEscopo a = escopo();
  Uint32 when = SDL_GetTicks();
  fontecache_vod_guardar("ttActor:1:2", "series", "", &a, &old, 1, when);
  hit(&a, old.url);
  unsigned generation = fontecache_vod_geracao();

  conta = "conta-b"; fixture_epoch_advance();
  miss(&a); /* B cannot use a caller's old A scope. */
  conta = "conta-a"; fixture_epoch_advance();
  FontecacheEscopo newA = escopo();
  assert(a.sessao != newA.sessao && a.geracao == newA.geracao);
  assert(a.perfil == newA.perfil && a.addons == newA.addons &&
         !strcmp(a.conta, newA.conta));
  miss(&newA);
  assert(bytesCache() == 0);
  fontecache_vod_guardar("ttActor:1:2", "series", "", &a, &old, 1, when);
  miss(&newA);
  assert(bytesCache() == 0);
  puts("PASS: A->B->A keeps the same cache generation and scope fields; old hit/store rejected");

  fontecache_vod_guardar("ttActor:1:2", "series", "", &newA, &current, 1, when);
  hit(&newA, current.url);
  size_t bytes = bytesCache();
  fontecache_vod_guardar("ttActor:1:2", "series", "", &a, &old, 1, when);
  assert(bytesCache() == bytes);
  hit(&newA, current.url);
  puts("PASS: current actor stores/hits; stale store cannot replace the current cached result");

  fontecache_vod_limpar();
  FontecacheEscopo preparing = escopo();
  when = SDL_GetTicks();
  generation = fontecache_vod_geracao();
  fixture_epoch_flip_on_tick();
  /* Actual guardarVodSemTrava reads its clock before checking the epoch.
   * Advance there to force invalidation after capture but during store.
   */
  fontecache_vod_guardar("ttActor:1:2", "series", "", &preparing, &old, 1, when);
  assert(!perfis_sessao_atual(preparing.sessao));
  assert(generation == fontecache_vod_geracao() && bytesCache() == 0);
  miss(&preparing);
  newA = escopo();
  miss(&newA);
  fontecache_vod_guardar("ttActor:1:2", "series", "", &newA, &current, 1, when);
  hit(&newA, current.url);
  fontecache_vod_limpar();
  puts("PASS: epoch changes inside store preparation; late response rejected, fresh store accepted");
  return 0;
}
