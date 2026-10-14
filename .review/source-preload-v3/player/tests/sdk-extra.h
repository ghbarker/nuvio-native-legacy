#include <SDL2/SDL.h>
char *SDL_GetBasePath(void);
void SDL_free(void *ptr);
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(a,b) ((Sint32)((b)-(a)) <= 0)
#endif
