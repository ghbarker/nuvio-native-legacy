#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A)) <= 0)
#endif
#ifndef SDL_INIT_EVENTS
#define SDL_INIT_EVENTS 0x4000
#endif
#define SDL_HINT_THREAD_STACK_SIZE "SDL_THREAD_STACK_SIZE"
typedef struct { int value; } SDL_atomic_t;
void *SDL_AndroidGetJNIEnv(void);
void *SDL_AndroidGetActivity(void);
int SDL_AtomicGet(SDL_atomic_t *);
void SDL_AtomicSet(SDL_atomic_t *, int);
Uint32 SDL_WasInit(Uint32);
Uint32 SDL_RegisterEvents(int);
SDL_bool SDL_SetHint(const char *, const char *);
