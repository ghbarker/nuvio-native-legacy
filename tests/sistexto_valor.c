#include "sistexto.h"
#include "entrada_texto.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int texto_sistema_disponivel(void) { return 0; }
void texto_sistema_abrir(const char *s, int v) { (void)s; (void)v; }
void texto_sistema_fechar(void) {}
int texto_sistema_aberto(void) { return 0; }
Uint32 texto_sistema_evento(void) { return SDL_USEREVENT + 7; }
const char *texto_sistema_valor(void) { return ""; }
int texto_sistema_engole(const SDL_Event *e) { (void)e; return 0; }
const char *desc_tmdb_idioma(void) { return "pt"; }

static int cases;
static void open_field(void) {
  st_fechar(ST_TECLADO);
  assert(st_ime_abrir(ST_TECLADO, "retained", 120));
}
static void batch(const char *first, const char *last, int expected, const char *value, int received) {
  char out[128];int changed = 7;
  open_field();
  if(first) st_teste_evento(first);
  if(last) st_teste_evento(last);
  assert(st_ler_valor(ST_TECLADO, out, sizeof out, &changed) == expected);
  assert(changed == received && !strcmp(out, value));
  changed = 7;
  assert(st_ler_valor(ST_TECLADO, out, sizeof out, &changed) == ST_NADA);
  assert(changed == 0 && !out[0]);
  cases++;
}
int main(void) {
  char out[128];int changed;
  st_teste_ligar(1);
  batch("Tnew", "X", ST_CANCELOU, "new", 1);
  batch("T", "X", ST_CANCELOU, "", 1);
  batch(NULL, "X", ST_CANCELOU, "", 0);
  batch("Told", "Tnew", ST_TEXTO, "new", 1);
  batch("Told", "T", ST_TEXTO, "", 1);
  batch(NULL, "Ddone", ST_FIM, "done", 1);
  batch("Ppartial", "Vspoken", ST_FIM, "spoken", 1);
  batch("Tnew", "Enada", ST_CANCELOU, "new", 1);
  batch("T", "Steclado:negada", ST_PEDE_TECLADO, "", 1);
  batch(NULL, "Steclado:negada", ST_PEDE_TECLADO, "", 0);
  batch(NULL, "R30", ST_NADA, "", 0);
  batch(NULL, NULL, ST_NADA, "", 0);
  open_field();st_teste_evento("Tnot-owned");st_teste_evento("X");changed=7;
  assert(st_ler_valor(ST_BUSCA,out,sizeof out,&changed)==ST_NADA);
  assert(!out[0]&&changed==0);cases++;
  open_field();st_teste_evento("Tstale");st_teste_evento("X");
  open_field();changed=7;
  assert(st_ler_valor(ST_TECLADO,out,sizeof out,&changed)==ST_NADA);
  assert(!out[0]&&changed==0);cases++;
  open_field();st_teste_evento("Tbefore");st_teste_evento("X");st_teste_evento("Tafter");changed=7;
  assert(st_ler_valor(ST_TECLADO,out,sizeof out,&changed)==ST_CANCELOU&&changed==1&&!strcmp(out,"before"));
  assert(st_ler_valor(ST_TECLADO,out,sizeof out,&changed)==ST_TEXTO&&changed==1&&!strcmp(out,"after"));cases++;
  open_field();st_teste_evento("Tnew");st_teste_evento("X");
  assert(st_ler(ST_TECLADO,out,sizeof out)==ST_CANCELOU&&!strcmp(out,"new"));cases++;
  open_field();st_teste_evento("T");st_teste_evento("X");
  assert(st_ler(ST_TECLADO,out,sizeof out)==ST_CANCELOU&&!out[0]);cases++;
  open_field();st_teste_evento("Tnew");changed=7;
  assert(st_ler_valor(ST_TECLADO,NULL,0,&changed)==ST_TEXTO&&changed==1);cases++;
  open_field();st_teste_evento("Tnew");
  assert(st_ler_valor(ST_TECLADO,out,2,&changed)==ST_TEXTO&&changed==1&&!strcmp(out,"n"));cases++;
  printf("sistexto_valor: %d real queue receipt/empty/cancel/finish/ownership/drain/bound cases PASS\n",cases);
  return 0;
}
