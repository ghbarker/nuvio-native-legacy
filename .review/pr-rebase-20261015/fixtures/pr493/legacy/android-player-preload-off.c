// Review-only companions for tests/player_android_retomada.sh. That reduced
// fixture compiles player.c for Android and supplies a fake playback backend,
// while all remaining sources use the host backend. It has no Media3 preload
// backend. Keep the opt-in feature unavailable, as with no prepared video;
// the original resume, fallback, retained-player and profile assertions remain.
#ifndef NV_ANDROID
#define NV_ANDROID 1
#endif
#include "video.h"
#include "faixas.h"
#include <assert.h>
#if defined(__GNUC__) || defined(__clang__)
#define REVIEW_WEAK __attribute__((weak))
#else
#define REVIEW_WEAK
#endif
REVIEW_WEAK int video_precarregar(const char *url, const char *headers,
    double seconds, double percent, int buffer, int cache_mb, unsigned token,
    const char *audio_language) {
  (void)url; (void)headers; (void)seconds; (void)percent; (void)buffer;
  (void)cache_mb; (void)token; (void)audio_language;
  return 0;
}
REVIEW_WEAK int video_precarregar_proximo(const char *url, const char *headers,
    double seconds, double percent, int buffer, int cache_mb, unsigned token,
    const char *audio_language) {
  return video_precarregar(url, headers, seconds, percent, buffer, cache_mb,
                           token, audio_language);
}
REVIEW_WEAK int video_precarregar_estado(unsigned token) {
  (void)token; return 0;
}
REVIEW_WEAK void video_precarregar_cancelar(unsigned token) { (void)token; }
REVIEW_WEAK void video_precarregar_adotar(unsigned token) {
  // This fake backend cannot produce a ready preload. Any attempted adoption
  // would invalidate the scope of the retained original resume assertions.
  (void)token; assert(!"Resume fixture must not adopt an unavailable preload");
}
REVIEW_WEAK void faixas_reiniciar_preparada(void) {
  assert(!"Resume fixture must not initialize tracks for an unavailable preload");
}
