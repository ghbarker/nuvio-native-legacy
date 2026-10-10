/* Actual phone modal footer text must contrast with the selected pill. */
#define TESTE_ILHA 1
#define main ilha_builder_main
#include "telefone_dialogos.c"
#undef main
int main(void) {
  nv_layout_w=1080;nv_layout_h=2340;zoom=escala=1;
  modalAberto=modalAviso=1;modalAtividade=0;memset(&modalM,0,sizeof modalM);
  modalM.nBotoes=1;snprintf(modalM.botao[0],sizeof modalM.botao[0],"Dispensar");
  for(int tint=0;tint<=255;tint+=255) {
    tintaFocoTeste=tint;modalFocoA[0]=1;
    modalTelefone((GfxRect){40,100,1000,500},&modalM,1,1);
    assert(corTextoR==tint&&corTextoG==tint&&corTextoB==tint);
    modalFocoA[0]=0;
    modalTelefone((GfxRect){40,100,1000,500},&modalM,1,1);
    assert(corTextoR==243&&corTextoG==242&&corTextoB==239);
  }
  puts("ilha_phone_colors: actual selected footer follows contrast palette, unfocused text preserved PASS");
}
