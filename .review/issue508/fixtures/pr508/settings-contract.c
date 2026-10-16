/* Review-only settings schema and range fixture. Complete actual ajustes.c. */
#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include SETTINGS_SOURCE
#include <assert.h>
#include <stdio.h>
static int checks;
#define CHECK(c) do { checks++; assert(c); } while(0)

int main(void) {
  CHECK(!strcmp(CHAVE[AJ_TELA_ABERTURA], "telaAberturaLocal"));
  CHECK(valor[AJ_TELA_ABERTURA] == 0 && valorPadrao[AJ_TELA_ABERTURA] == 0);
  CHECK(somenteDesteAparelho(AJ_TELA_ABERTURA));
  CHECK(ajCenaSecao(AJ_TELA_ABERTURA) == AJC_REP_DURANTE);
  CHECK(OPCOES[AJ_TELA_ABERTURA].tipo == OP_ESCOLHA);
  CHECK(!strcmp(V_TELA_ABERTURA[0], "Arte desfocada"));
  CHECK(!strcmp(V_TELA_ABERTURA[1], "Detalhada"));
  CHECK(limita(AJ_TELA_ABERTURA,0) == 0);
  CHECK(limita(AJ_TELA_ABERTURA,1) == 1);
  CHECK(!ajustes_abertura_detalhada());
  valor[AJ_TELA_ABERTURA] = 1; CHECK(ajustes_abertura_detalhada());
#ifdef ISSUE508
  CHECK(OPCOES[AJ_TELA_ABERTURA].n == 3);
  CHECK(!strcmp(V_TELA_ABERTURA[2], "Arte nítida"));
  CHECK(strcmp(V_TELA_ABERTURA[0],V_TELA_ABERTURA[1]) && strcmp(V_TELA_ABERTURA[0],V_TELA_ABERTURA[2]) && strcmp(V_TELA_ABERTURA[1],V_TELA_ABERTURA[2]));
  CHECK(!ajustes_abertura_fundo_nitido());
  valor[AJ_TELA_ABERTURA] = 2;
  CHECK(!ajustes_abertura_detalhada() && ajustes_abertura_fundo_nitido());
  CHECK(limita(AJ_TELA_ABERTURA,2) == 2);
#else
  CHECK(OPCOES[AJ_TELA_ABERTURA].n == 2);
#endif
  valor[AJ_TELA_ABERTURA] = 0;
  CHECK(limita(AJ_TELA_ABERTURA,999) == 0);
  CHECK(limita(AJ_TELA_ABERTURA,-1) == 0);
  printf("PASS SETTINGS checks=%d saved0=Blurred/default saved1=Detailed saved2=Clear; local classification and range validation\n",checks);
  return 0;
}
