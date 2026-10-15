// Original live-viewer event paths, actual C implementation, no renderer/network.
#include <assert.h>
#include <stdio.h>
#define REGISTRO_TESTE 1
#include "registro.c"
Uint32 SDL_GetTicks(void) { return 1000; }
char *dados_ler(const char *name) { (void)name; return NULL; }
int dados_gravar(const char *name, const char *text) { (void)name; (void)text; return 1; }
const char *i18n(const char *s) { return s; }
void avisos_enviar_registro_atual(void) { assert(!"Viewing must not send a report"); }
int avisos_envio_info(AvisosEnvio *env) { memset(env, 0, sizeof *env); return 0; }
int ajustes_idioma(void) { return 1; }
int layout_modo_mobile(void) { return 1; }
int main(void) {
  registro_teste_texto("[video] viewing current session\n[rede] connection ready\n");
  registro_abrir();
  assert(registro_aberto() && nLin == 2);
  SDL_Event ev; memset(&ev, 0, sizeof ev); ev.type = SDL_KEYDOWN;
  ev.key.keysym.sym = SDLK_UP;
  assert(registro_evento(&ev));
  assert(pausado);
  ev.key.keysym.sym = SDLK_ESCAPE;
  assert(registro_evento(&ev) && !registro_aberto());
  ev.type = SDL_KEYUP;
  assert(registro_evento(&ev)); // No leaked Back-up to the underlying page.
  puts("PASS original live viewer opens, scrolls/pauses and closes without report upload");
  return 0;
}
