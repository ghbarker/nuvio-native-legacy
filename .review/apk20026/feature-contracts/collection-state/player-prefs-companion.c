/* Genuine player preference producer; deterministic inactive video/platform boundary. */
#include <SDL2/SDL.h>
char *SDL_GetBasePath(void); void SDL_free(void *);
#include PLAYER_SOURCE
char *SDL_GetBasePath(void) { return strdup("/nonexistent-offline-settings-fixture/"); }
void SDL_free(void *p) { free(p); }
void video_legenda_estilo(const VideoLegendaEstilo *e) { (void)e; }
