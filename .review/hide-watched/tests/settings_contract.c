#include "ajustes.c"
#include <assert.h>

int main(void) {
  int found = -1;
#ifdef NV_COMBINED_PREVIEW
  assert(AJ_OCULTAR_FILMES_VISTOS == AJ_FILTRO_GRUPOS + 1);
#else
  assert(AJ_OCULTAR_FILMES_VISTOS == AJ_INTERFACE_MODO + 1);
#endif
  assert(AJ_N == AJ_OCULTAR_FILMES_VISTOS + 1);
  assert(valor[AJ_OCULTAR_FILMES_VISTOS] == 1);
  assert(!ajustes_ocultar_filmes_vistos());
  assert(OPCOES[AJ_OCULTAR_FILMES_VISTOS].tipo == OP_ESCOLHA);
  assert(OPCOES[AJ_OCULTAR_FILMES_VISTOS].n == 2);
  assert(OPCOES[AJ_OCULTAR_FILMES_VISTOS].valores == V_LIGA);
  assert(!strcmp(CHAVE[AJ_OCULTAR_FILMES_VISTOS], "hideWatchedMoviesLocal"));
  assert(somenteDesteAparelho(AJ_OCULTAR_FILMES_VISTOS));
  assert(perfilLocal(AJ_OCULTAR_FILMES_VISTOS));
  assert(dePerfil(AJ_OCULTAR_FILMES_VISTOS));
  assert(ajCenaSecao(AJ_OCULTAR_FILMES_VISTOS) == AJC_HOME_FILEIRAS);
  assert(familiaPreviaOpcao(AJ_OCULTAR_FILMES_VISTOS) == AJPV_HOME);
  for (int i = 0; i < AJ_N_TELA; i++) {
    if (TELA[i].tipo == IT_OPC && TELA[i].op == AJ_OCULTAR_FILMES_VISTOS) {
      assert(found < 0);
      found = i;
    }
  }
  assert(found > 0 && TELA[found - 1].op == AJ_SELO_VISTO);
  valor[AJ_OCULTAR_FILMES_VISTOS] = 0;
  assert(ajustes_ocultar_filmes_vistos());
  valor[AJ_OCULTAR_FILMES_VISTOS] = 1;
  assert(!ajustes_ocultar_filmes_vistos());
  puts("PASS: watched preference default, key, profile/device classification, TV/mobile placement and preview");
  return 0;
}
