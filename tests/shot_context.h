#ifndef NV_TEST_SHOT_CONTEXT_H
#define NV_TEST_SHOT_CONTEXT_H
#include <SDL2/SDL.h>

// The production Linux renderer uses GLES2, whereas macOS uses OpenGL 2.1.
static void shot_context(void) {
#ifdef NV_LINUX_DESKTOP
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
}
#endif
