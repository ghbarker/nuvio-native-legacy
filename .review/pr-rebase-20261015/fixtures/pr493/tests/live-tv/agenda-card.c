#include "support.h"
#include "agendaviso.c"

static Uint32 ticks = 100;
static AgItem a, b;
static int marked, marked_profile, assembled;
static char marked_id[24];

Uint32 SDL_GetTicks(void) { return ticks; }
void agenda_iniciar(void) {}
int agenda_lembretes_pendentes(void) { return 1; }
int agenda_montar(void) { assembled++; return 1; }
int agenda_devidos(const AgItem **out, int max) {
  assert(max > 0);
  out[0] = perfis_ativo() == 1 ? &a : &b;
  return 1;
}
void agenda_marcar_avisado(const char *id) {
  marked++;
  marked_profile = perfis_ativo();
  snprintf(marked_id, sizeof marked_id, "%s", id);
}
static void outgoing(void) { agendaviso_abrir_sessao(); }

int main(void) {
  fixture_init();
  perfis_ao_abrir_sessao(outgoing);
  strcpy(a.imdb, "tt1000001"); strcpy(a.titulo, "Private A episode");
  strcpy(b.imdb, "tt2000002"); strcpy(b.titulo, "Private B episode");
  agendaviso_mostrar_se_houver();
  assert(agendaviso_aberto() && !strcmp(itens[0].titulo, a.titulo));
  entrada = 1.0f;
  perfis_definir_ativo(2);
  assert(!agendaviso_aberto() && !entrada && !n && !itens[0].titulo[0]);
  assert(!marked && !ultimaMontagem);
  puts("PASS: switching clears A’s copied episode reminder without acknowledging it under B");

  ticks = 101;
  agendaviso_mostrar_se_houver();
  assert(agendaviso_aberto() && assembled == 2);
  assert(!strcmp(itens[0].titulo, b.titulo));
  SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
  agendaviso_evento(&e);
  assert(!agendaviso_aberto() && marked == 1 && marked_profile == 2);
  assert(!strcmp(marked_id, b.imdb));
  puts("PASS: B’s episode card builds immediately and OK acknowledges only B’s reminder");
  fixture_cleanup();
}
