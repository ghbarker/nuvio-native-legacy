#include <SDL2/SDL.h>
char *SDL_GetBasePath(void); void SDL_free(void*);
#include <assert.h>
#include PLAYER_SOURCE
char *SDL_GetBasePath(void){return NULL;}void SDL_free(void*p){free(p);}
static int applied;
void video_legenda_estilo(const VideoLegendaEstilo*e){assert(e);applied++;}
void test_player_reload(void){legEstilo=*player_leg_estilo_padrao();legTocado=0;prefsLidas=0;player_leg_estilo_carregar();}
