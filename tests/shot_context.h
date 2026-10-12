#ifndef NV_TEST_SHOT_CONTEXT_H
#define NV_TEST_SHOT_CONTEXT_H
#include <SDL2/SDL.h>

#ifdef NV_SHOT_FIXED_EPOCH
#include <time.h>
// Freeze calendar/clock fixtures without changing SDL's animation clock.
time_t time(time_t *result) {
  const time_t fixed = (time_t)NV_SHOT_FIXED_EPOCH;
  if (result) *result = fixed;
  return fixed;
}
#endif

#ifdef NV_SHOT_FIXED_TICKS
#include <stdatomic.h>
static _Atomic Uint32 shotTicks = 1;
// Every reader sees the same frame time, including asynchronous workers.
Uint32 SDL_GetTicks(void) { return atomic_load_explicit(&shotTicks, memory_order_relaxed); }
static void shot_frame_tick(void) { atomic_fetch_add_explicit(&shotTicks, 17, memory_order_relaxed); }
#else
static void shot_frame_tick(void) {}
#endif

// The production Linux renderer uses GLES2, whereas macOS uses OpenGL 2.1.
static void shot_context(void) {
#ifdef NV_LINUX_DESKTOP
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
}
#endif
