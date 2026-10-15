#include <SDL2/SDL.h>
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
char *SDL_GetBasePath(void);
void SDL_free(void *);
void *SDL_AndroidGetJNIEnv(void);
void *SDL_AndroidGetActivity(void);
