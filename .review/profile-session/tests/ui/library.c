#include "common.h"
#include "biblioteca.c"
int main(void){setup();fonte=FONTE_SIMKL;exibicao=VIS_LISTA;pedido=10;temAberta=1;okDesde=8;selectProfile(2);biblioteca_abrir_sessao();assert(fonte==FONTE_TRAKT&&pedido==-1&&!temAberta&&!okDesde&&exibicao==VIS_LISTA);assert(!deleted);puts("PASS Library: new session source/focus reset preserves saved display preference");}
