// Review-only fixture: actual Settings includes, no window or network.
#include REVIEW_SOURCE
#include <assert.h>

static int scrollHandlers;
float gfx_escala(void) { return 1.0f; }
void ponteiro_rolagem(PonteiroRolagemFn fn) { assert(fn); scrollHandlers++; }

static void tvGeometry(void) {
  assert(!ajTelaCheia() && !ajRetrato());
  assert(NV_LAYOUT_REAL_W == 1920 && NV_LAYOUT_REAL_H == 1080);
  GfxRect menu = aj2MenuR(), list = aj2ListaR(), editor = aj2EditorR();
  printf("TV %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
         menu.x,menu.y,menu.w,menu.h,list.x,list.y,list.w,list.h,
         editor.x,editor.y,editor.w,editor.h);
}

int main(void) {
  assert(valorPadrao[AJ_INTERFACE_MODO] == 2);
  assert(AJ_OCULTAR_FILMES_VISTOS + 1 == AJ_PROVEDORES_ESCOPO);
  // New upstream options append after providers; no persisted index moves.
  assert(AJ_PROVEDORES_ESCOPO + 1 == AJ_MIC_BUSCA);
  assert(AJ_MIC_BUSCA + 1 == AJ_FONTE_IDIOMA_AUDIO);
#ifdef NV_ANDROID
  assert(AJ_FONTE_IDIOMA_AUDIO + 1 == AJ_BAIXAR_REGISTRO);
  assert(AJ_BAIXAR_REGISTRO + 1 == AJ_HERO_SINOPSE);
  assert(!strcmp(CHAVE[AJ_BAIXAR_REGISTRO], "-baixarRegistro"));
  assert(OPCOES[AJ_BAIXAR_REGISTRO].tipo == OP_ACAO);
  assert(valorPadrao[AJ_BAIXAR_REGISTRO] == 0);
#else
  assert(AJ_FONTE_IDIOMA_AUDIO + 1 == AJ_HERO_SINOPSE);
#endif
  assert(AJ_HERO_SINOPSE + 1 == AJ_N);
  assert(sizeof OPCOES / sizeof *OPCOES == AJ_N);
  assert(sizeof CHAVE / sizeof *CHAVE == AJ_N);
  assert(sizeof valor / sizeof *valor == AJ_N);
  assert(sizeof valorPadrao / sizeof *valorPadrao == AJ_N);
  assert(!strcmp(CHAVE[AJ_MIC_BUSCA], "micBuscaLocal"));
  assert(!strcmp(CHAVE[AJ_FONTE_IDIOMA_AUDIO], "fonteIdiomaAudioLocal"));
  assert(!strcmp(CHAVE[AJ_HERO_SINOPSE], "heroSynopsisSizeLocal"));
  assert(OPCOES[AJ_MIC_BUSCA].valores == V_MIC_BUSCA && OPCOES[AJ_MIC_BUSCA].n == 3);
  assert(OPCOES[AJ_FONTE_IDIOMA_AUDIO].valores == V_LIGA && OPCOES[AJ_FONTE_IDIOMA_AUDIO].n == 2);
  assert(OPCOES[AJ_HERO_SINOPSE].valores == V_HERO_SINOPSE && OPCOES[AJ_HERO_SINOPSE].n == 2);
  assert(valorPadrao[AJ_MIC_BUSCA] == 0);
  assert(valorPadrao[AJ_FONTE_IDIOMA_AUDIO] == 1);
  assert(valorPadrao[AJ_HERO_SINOPSE] == 0);
  for (int op = 0; op < AJ_N; op++) {
    assert(CHAVE[op] && CHAVE[op][0] && OPCOES[op].rotulo && OPCOES[op].rotulo[0]);
    assert(valor[op] == valorPadrao[op]);
  }
  assert(!strcmp(CHAVE[AJ_OCULTAR_FILMES_VISTOS], "hideWatchedMoviesLocal"));
  assert(!strcmp(CHAVE[AJ_PROVEDORES_ESCOPO], "-provedoresEscopo"));
  assert(OPCOES[AJ_OCULTAR_FILMES_VISTOS].valores == V_LIGA);
  assert(valorPadrao[AJ_OCULTAR_FILMES_VISTOS] == 1);
  assert(valorPadrao[AJ_PROVEDORES_ESCOPO] == 0);
  valor[AJ_HOME_LAYOUT] = HOME_LAYOUT_DINAMICA;
  layout_tela_definir(1080,2340); layout_modo_definir(0);
  tvGeometry();
#ifdef NV_TOUCH_UI
  float offset = 777.0f;
  GfxRect region = {10,20,300,400};
  ajToqueN = 1; ajToqueAtivo = AJT_LISTA;
  ajToqueLivre[AJT_LISTA] = 1; ajToqueChave[AJT_LISTA] = 12;
  AjToqueRegiao before[AJT_N]; memcpy(before,ajToqueRegioes,sizeof before);
  assert(ajToqueRegistrar(AJT_LISTA,region,&offset,100,25,99) == 25);
  assert(offset == 777 && ajToqueN == 1 && ajToqueAtivo == AJT_LISTA);
  assert(ajToqueLivre[AJT_LISTA] == 1 && ajToqueChave[AJT_LISTA] == 12);
  assert(!memcmp(before,ajToqueRegioes,sizeof before) && !scrollHandlers);

  layout_modo_definir(1); ajToqueCamada();
  assert(ajTelaCheia() && ajRetrato());
  assert(ajToqueRegistrar(AJT_LISTA,region,&offset,100,25,99) == 25);
  assert(offset == 25 && ajToqueN == 1 && scrollHandlers == 1);
  layout_modo_definir(0); offset = 444; scrollHandlers = 0;
  assert(ajToqueRegistrar(AJT_LISTA,region,&offset,100,40,101) == 40);
  assert(offset == 444 && ajToqueN == 1 && !scrollHandlers);
  tvGeometry();
  fputs("Settings: TV registrar has no side effects; Mobile registers; return-to-TV ignores retained state PASS\n",stderr);
#endif
  return 0;
}
