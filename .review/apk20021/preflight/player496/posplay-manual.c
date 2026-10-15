#include POSPLAY_SOURCE
#include <assert.h>
#include <stdio.h>
static int legacyPrefetchCancelled, independentNextCancelled, deletedCache;
/* Offline monotonic clock for private diagnostic timestamps only. */
Uint32 SDL_GetTicks(void) { return 5000; }
void fontecache_cancelar_proximo(void) { legacyPrefetchCancelled++; }
void fontecache_vod_apagar(const char *id,const char *tipo,const char *origem,const FontecacheEscopo *scope) {
  (void)id;(void)tipo;(void)origem;(void)scope;deletedCache++;
}
void proximopreparo_cancelar(void) { independentNextCancelled++; }
int main(void) {
  pedT=1;pedE=3;pedAutomatico=automaticoEntregue=1;
  visivel=1;fecharEm=12345;foco=2;dispensado=0;
  snprintf(preEpisodio,sizeof preEpisodio,"ttfixture:1:2");
  snprintf(preAlvo,sizeof preAlvo,"ttfixture:1:3");
  preTentou=2;preVideoTentou=1;preVideoRenovou=1;
  posplay_troca_manual();
  assert(!pedT&&!pedE&&!pedAutomatico&&!automaticoEntregue);
  assert(!visivel&&!fecharEm&&!foco&&dispensado);
  assert(legacyPrefetchCancelled==1&&!independentNextCancelled&&!deletedCache);
  assert(!strcmp(preEpisodio,"ttfixture:1:2")&&!strcmp(preAlvo,"ttfixture:1:3"));
  assert(preTentou==2&&preVideoTentou&&preVideoRenovou);
  int t=-1,e=-1;assert(!posplay_pediu_episodio(&t,&e));
  puts("PASS actual manual transition clears stale card/countdown mail without cancelling independent prepared Next");
  return 0;
}
