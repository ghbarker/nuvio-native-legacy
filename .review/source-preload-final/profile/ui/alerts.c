#include "common.h"
#include "avisos.c"
int avisodisp_tem(const char*x){(void)x;return 0;}
int main(void){setup();vistosLer();por("update:2",AV_UPDATE,"Update","Device",NULL);por("agenda:tt1:2026-10-14",AV_AGENDA,"Episode","A private series","tt1");por("rec",AV_REC,"Friend","A friend",NULL);marcarVisto("agenda:tt1:2026-10-14");assert(foiVisto("agenda:tt1:2026-10-14"));selectProfile(2);assert(!foiVisto("agenda:tt1:2026-10-14"));avisos_abrir_sessao();assert(n==1&&itens[0].tipo==AV_UPDATE);assert(!aberto&&!toastN&&!pediuAbrir[0]);selectProfile(1);assert(foiVisto("agenda:tt1:2026-10-14"));assert(!deleted);puts("PASS alerts: private rows cleared, device notice retained, private seen flags isolated/restored");}
