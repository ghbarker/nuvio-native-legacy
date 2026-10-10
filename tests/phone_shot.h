/* Phone-sized captures of the existing offline GL fixtures. Test binary only. */
#ifndef NV_TEST_PHONE_SHOT_H
#define NV_TEST_PHONE_SHOT_H
#include "layout.h"
#include "gfx.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int phone_shot_w = 1080, phone_shot_h = 2340;
static SDL_Window *phone_shot_window;
static SDL_Window *phone_shot_create(const char *title, int x, int y,
                                    int width, int height, Uint32 flags) {
  const char *w = getenv("NUVIO_PHONE_SHOT_W"), *h = getenv("NUVIO_PHONE_SHOT_H");
  (void)width; (void)height; (void)flags;
  if (w && atoi(w) > 0) phone_shot_w = atoi(w);
  if (h && atoi(h) > 0) phone_shot_h = atoi(h);
  layout_tela_definir(phone_shot_w, phone_shot_h);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  phone_shot_window = SDL_CreateWindow(title, x, y, phone_shot_w, phone_shot_h,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  return phone_shot_window;
}
static void phone_shot_viewport(int x, int y, int width, int height) {
  (void)x; (void)y; (void)width; (void)height;
  glViewport(0, 0, phone_shot_w, phone_shot_h);
}
static void phone_shot_target(int width, int height) {
  (void)width; (void)height;
  gfx_tamanho_alvo(phone_shot_w, phone_shot_h);
}
static int phone_shot_save(const char *path) {
  int width = 0, height = 0, result;
  SDL_Surface *surface;
  unsigned char *pixels;
  char output[4096];
  if (!phone_shot_window) return -1;
  SDL_GL_GetDrawableSize(phone_shot_window, &width, &height);
  if (width != phone_shot_w || height != phone_shot_h) return -1;
  pixels = malloc((size_t)width * (size_t)height * 4);
  surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
  if (!pixels || !surface) { free(pixels); if (surface) SDL_FreeSurface(surface); return -1; }
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  for (int y = 0; y < height; y++)
    memcpy((char *)surface->pixels + y * surface->pitch,
           pixels + (size_t)(height - 1 - y) * width * 4, (size_t)width * 4);
  snprintf(output, sizeof output, "%s", path);
  char *extension = strrchr(output, '.');
  if (extension && !strcmp(extension, ".bmp")) strcpy(extension, ".png");
  result = IMG_SavePNG(surface, output);
  SDL_FreeSurface(surface); free(pixels);
  if (result == 0) printf("phone capture: %s (%dx%d)\n", output, width, height);
  return result;
}
/* Original fixture readback buffers are TV-sized; the save hook reads the
   actual phone framebuffer separately rather than writing past those buffers. */
static void phone_shot_read(int x, int y, int w, int h, GLenum format,
                            GLenum type, void *pixels) {
  (void)x; (void)y; (void)w; (void)h; (void)format; (void)type; (void)pixels;
}

/* These offline desktop captures model native-input availability. The Android
   editor/keyboard and text confirmation are covered separately on a device. */
int __wrap_st_ime_disponivel(void) { return 1; }

#define SDL_CreateWindow phone_shot_create
#define glViewport phone_shot_viewport
#define gfx_tamanho_alvo phone_shot_target
#define glReadPixels phone_shot_read
#undef SDL_SaveBMP
#define SDL_SaveBMP(surface, path) phone_shot_save(path)
#define IMG_SavePNG(surface, path) phone_shot_save(path)
#endif
