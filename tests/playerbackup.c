#include "playerbackup.h"
#include "player.h"
#include "text.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void recusar(const char *s,size_t n) {
  PlayerBackup b,antes;memset(&b,0xa5,sizeof b);antes=b;
  assert(!playerbackup_ler(s,n,&b) && !memcmp(&b,&antes,sizeof b));
}
int main(void) {
  PlayerBackup b={.aspecto=PLR_ASP_CROP,.estilo={.tamanho=150,.cor=2,.fundo=3,.posicao=6,.borda=1,.atrasoMs=-1250,.opacidade=2,.familia=TXT_FAMILIA_INTER,.negrito=1,.corBorda=-1},.tocado=PLR_LEG_COR|PLR_LEG_FONTE},d;
  char *s=NULL,*out=NULL;assert(playerbackup_exportar(&b,&s));assert(playerbackup_ler(s,strlen(s),&d));
  assert(d.estilo.atrasoMs==-1250 && d.estilo.corBorda==-1 && d.estilo.tamanho==150 && d.tocado==b.tocado);
  assert(playerbackup_exportar(&d,&out) && !strcmp(s,out));free(out);
  for(size_t n=0;n<strlen(s);n++)recusar(s,n);
  char m[4096];strcpy(m,s);strcat(m,"session secret\n");recusar(m,strlen(m));
  strcpy(m,s);m[50]=0;recusar(m,strlen(s));
  const char *de[]={"SETTINGS 1","aspecto 1","leg_tamanho 150","leg_atraso_sinal 1","leg_cor_borda_explicita 1","leg_tocado 3"};
  const char *para[]={"SETTINGS 9","aspecto 9","leg_tamanho 155","leg_atraso_sinal 0","leg_cor_borda_explicita 0","leg_tocado 999999999999999999999"};
  for(size_t i=0;i<sizeof de/sizeof *de;i++) {
    const char *p=strstr(s,de[i]);assert(p);size_t k=(size_t)(p-s);
    memcpy(m,s,k);m[k]=0;strcat(m,para[i]);strcat(m,p+strlen(de[i]));recusar(m,strlen(m));
  }
  b.estilo.tamanho=50;assert(playerbackup_exportar(&b,&out));free(out);
  b.estilo.tamanho=300;b.estilo.atrasoMs=60000;b.estilo.corBorda=5;b.tocado=127;
  assert(playerbackup_exportar(&b,&out));assert(playerbackup_ler(out,strlen(out),&d));free(out);
  b.estilo.familia=TXT_FAMILIA_N;out=(char*)1;assert(!playerbackup_exportar(&b,&out) && !out);
  assert(!playerbackup_estilo_valido(NULL));free(s);
  puts("PASS: player/subtitle snapshot, signed delay, border sentinel, explicit-style flags and strict complete validation");
}
