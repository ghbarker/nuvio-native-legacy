#include "common.h"
#include "home.c"
static unsigned cr = 1;
unsigned cat_revisao(void) {return cr;}
int trailer_aberto(void){return 0;}
int trailer_dono(void){return TRAILER_DONO_HOME;}
void trailer_fechar(void){}
int cw_retido_definir(const char*x){(void)x;return 1;}
int main(void){
 setup(); nBd=7; snprintf(bd[0],sizeof bd[0],"public-art.jpg");
 heroAtual=heroAnterior=heroPendente=4; heroDesejado=8; focoHero=0;
 scrollY=99; scrollX[0]=33; pedidoAbrir=pedidoTocar=1;
 heroTrailerMarcarTocou("tt123"); heroAutoDesligado=1;
 expFileira=0; expColuna=3;expAbre=1;
 temItemFoco=1;itemFoco.indice=4;publicarItemFoco();
 HomeItem h;assert(home_item_focado(&h)); selectProfile(2);
 assert(!home_item_focado(&h)); home_abrir_sessao();
 assert(focoHero && foco.fileira==0 && foco.coluna==0);
 assert(!scrollY&&!scrollX[0]&&!pedidoAbrir&&!pedidoTocar);
 assert(heroAtual==0&&heroAnterior==0&&heroPendente==0&&heroDesejado==-1);
 assert(!heroTrailerJaTocou("tt123")&&!heroAutoDesligado&&!expAbre);
 assert(!home_item_focado(&h)&&!nFileiras&&nBd==7&&!strcmp(bd[0],"public-art.jpg"));
 itemFoco.indice=3;publicarItemFoco();assert(home_item_focado(&h));cr++;assert(!home_item_focado(&h));
 heroAtual=5; heroTrailerMarcarTocou("tt456");selectProfile(2);home_abrir_sessao();
 assert(heroAtual==0&&!heroTrailerJaTocou("tt456"));
 assert(!deleted); puts("PASS Home: fresh B/same-profile sessions, art reuse, stale epoch/catalog focus rejection");
}
