/* Read the actual text colour passed by the production focused button. */
#define TESTE_DIAG 1
#define main diagnostic_builder_main
#include "telefone_dialogos.c"
#undef main
int main(void) {
  nv_layout_w=1080;nv_layout_h=2340;zoom=escala=1;
  for(int tint=0;tint<=255;tint+=255) {
    tintaFocoTeste=tint;
    dgPhoneBotao("Testar de novo",2,0,1,200,1080,116,2304,1);
    assert(corTextoR==tint&&corTextoG==tint&&corTextoB==tint);
    dgPhoneBotao("Restaurar anteriores",2,1,0,300,1080,116,2304,1);
    assert(corTextoR==243&&corTextoG==242&&corTextoB==239);
  }
  puts("diagnostico_phone_colors: focused contrast follows active palette; unfocused text preserved PASS");
}
