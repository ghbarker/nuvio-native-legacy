#include <SDL2/SDL.h>
#ifdef NV_LOCAL_SDL_H
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
#endif
char *SDL_GetBasePath(void);
void SDL_free(void *);
void *SDL_AndroidGetJNIEnv(void);
void *SDL_AndroidGetActivity(void);
