#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A, B) ((Sint32)((B) - (A)) <= 0)
#endif

char *SDL_GetBasePath(void);
void SDL_free(void *);
