#include <SDL2/SDL.h>
#define SDL_TICKS_PASSED(A, B) ((Sint32)((B) - (A)) <= 0)
#include NV_AJUSTES_SOURCE
#include <assert.h>

int main(void) {
  int cases=0;
  ling_local_audio("");
  const char *modes[]={"*", "ORIGINAL", "off"};
  for (unsigned i=0; i<sizeof modes/sizeof *modes; i++) {
    ling_conta_audio(modes[i]);
    assert(!ling_audio_fonte("de")[0]); cases++;
    ling_conta_audio(NULL);
    assert(!strcmp(ling_audio_fonte("de"),"de")); cases++;
    ling_conta_audio(modes[i]); ling_conta_audio("");
    assert(!strcmp(ling_audio_fonte("de"),"de")); cases++;
    // Actual reset called by upstream and PR493 sync_reaplicar_ajustes.
    ling_conta_audio(modes[i]); ajustes_idiomas_da_conta(NULL);
    assert(!strcmp(ling_audio_fonte("de"),"de")); cases++;
    ling_conta_audio(modes[i]); ajustes_idiomas_da_conta("{}");
    assert(!strcmp(ling_audio_fonte("de"),"de")); cases++;
  }
  ajustes_idiomas_da_conta("{\"features\":{\"player_settings\":{\"preferred_audio_language\":\"original\"}}}");
  assert(!ling_audio_fonte("en")[0]); cases++;
  ajustes_idiomas_da_conta("{\"features\":{\"player_settings\":{\"preferred_audio_language\":\"fr\"}}}");
  assert(!strcmp(ling_audio_fonte("en"),"fr")); cases++;
  ajustes_idiomas_da_conta(NULL); assert(!strcmp(ling_audio_fonte("ja"),"ja")); cases++;
  ling_local_audio("*"); ling_conta_audio("fr"); ajustes_idiomas_da_conta(NULL);
  assert(!ling_audio_fonte("de")[0]); cases++;
  ling_local_audio("~"); ling_definir_original("it"); ajustes_idiomas_da_conta(NULL);
  assert(!strcmp(ling_audio_fonte("en"),"it")); cases++;
  ling_definir_original(""); assert(!ling_audio_fonte("en")[0]); cases++;
  printf("account-reset actual Settings + language policy cases=%d\n",cases);
}
