/* Fixed offline clock boundary for existing private diagnostics, not playback. */
#include <SDL2/SDL.h>
Uint32 SDL_GetTicks(void) { return 1000u; }
