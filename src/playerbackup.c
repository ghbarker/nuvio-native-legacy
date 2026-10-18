#include "playerbackup.h"
#include "player.h"
#include "text.h"
#include "legenda.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PB_MAX_TEXTO 4096u
static int valido(const PlayerBackup *b) {
  const int campos=PLR_LEG_COR|PLR_LEG_FONTE|PLR_LEG_COR_BORDA|PLR_LEG_BORDA|PLR_LEG_FUNDO|PLR_LEG_POSICAO|PLR_LEG_NEGRITO;
  return b && b->aspecto>=0 && b->aspecto<PLR_ASP_N && !(b->tocado&~campos) && playerbackup_estilo_valido(&b->estilo);
}
int playerbackup_exportar(const PlayerBackup *b,char **saida) {
  if(saida)*saida=NULL;
  if(!saida || !valido(b))return 0;
  char *out=calloc(PB_MAX_TEXTO+1u,1);if(!out)return 0;
  const VideoLegendaEstilo *e=&b->estilo;
  int n=snprintf(out,PB_MAX_TEXTO+1u,"NUVIO-PLAYER-SETTINGS 1\naspecto %d\nleg_tamanho %d\nleg_cor %d\nleg_fundo %d\nleg_pos %d\nleg_borda %d\nleg_atraso %d\nleg_atraso_sinal 1\nleg_opacidade %d\nleg_familia %d\nleg_negrito %d\nleg_tocado %d\nleg_cor_borda %d\nleg_cor_borda_explicita 1\n",
    b->aspecto,e->tamanho,e->cor,e->fundo,e->posicao,e->borda,e->atrasoMs,e->opacidade,e->familia,e->negrito,b->tocado,e->corBorda);
  if(n<0 || (unsigned)n>PB_MAX_TEXTO){free(out);return 0;}
  *saida=out;return 1;
}
static int campo(const char **p,const char *fim,const char *nome,int *out) {
  const char *e=memchr(*p,'\n',(size_t)(fim-*p));if(!e || (size_t)(e-*p)>100)return 0;
  char linha[101];size_t n=(size_t)(e-*p);memcpy(linha,*p,n);linha[n]=0;
  size_t k=strlen(nome);if(strncmp(linha,nome,k) || linha[k]!=' ')return 0;
  char *q=linha+k+1,*depois;if(!*q || *q==' ' || *q=='\t')return 0;
  errno=0;long v=strtol(q,&depois,10);
  if(q==depois || *depois || errno==ERANGE || v<INT_MIN || v>INT_MAX)return 0;
  *out=(int)v;*p=e+1;return 1;
}
int playerbackup_ler(const char *texto,size_t n,PlayerBackup *saida) {
  static const char cab[]="NUVIO-PLAYER-SETTINGS 1\n";
  if(!texto || !saida || n<sizeof cab-1 || n>PB_MAX_TEXTO || memchr(texto,0,n) || memcmp(texto,cab,sizeof cab-1))return 0;
  const char *p=texto+sizeof cab-1,*fim=texto+n;PlayerBackup b={0};int sinal,explicita;
  VideoLegendaEstilo *e=&b.estilo;
  if(!campo(&p,fim,"aspecto",&b.aspecto) || !campo(&p,fim,"leg_tamanho",&e->tamanho) ||
     !campo(&p,fim,"leg_cor",&e->cor) || !campo(&p,fim,"leg_fundo",&e->fundo) ||
     !campo(&p,fim,"leg_pos",&e->posicao) || !campo(&p,fim,"leg_borda",&e->borda) ||
     !campo(&p,fim,"leg_atraso",&e->atrasoMs) || !campo(&p,fim,"leg_atraso_sinal",&sinal) || sinal!=1 ||
     !campo(&p,fim,"leg_opacidade",&e->opacidade) || !campo(&p,fim,"leg_familia",&e->familia) ||
     !campo(&p,fim,"leg_negrito",&e->negrito) || !campo(&p,fim,"leg_tocado",&b.tocado) ||
     !campo(&p,fim,"leg_cor_borda",&e->corBorda) || !campo(&p,fim,"leg_cor_borda_explicita",&explicita) || explicita!=1 ||
     p!=fim || !valido(&b))return 0;
  *saida=b;return 1;
}
