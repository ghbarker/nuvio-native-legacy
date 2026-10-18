#include "backupplano_interno.h"
#include "ajustes.h"
#include "playerbackup.h"
#include "fontefalha.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
void backupplano_liberar(BackupPlano *p) {
  if(!p)return;
  for(int i=0;i<BACKUP_PARTES_N;i++){free(p->parte[i].antes);free(p->parte[i].depois);}
  free(p);
}
static char *copiar(const char *s,size_t n) {char *p=malloc(n+1);if(p){memcpy(p,s,n);p[n]=0;}return p;}
static int validar(int tipo,const char *texto,size_t n,char **nativo) {
  if(tipo==BACKUP_AJUSTES_APARELHO || tipo==BACKUP_AJUSTES_PERFIL)
    return ajustes_backup_mesclar(tipo==BACKUP_AJUSTES_PERFIL,"",texto,nativo);
  if(tipo==BACKUP_PLAYER) {
    PlayerBackup b;if(!playerbackup_ler(texto,n,&b))return 0;
    return playerbackup_exportar(&b,nativo);
  }
  if(tipo==BACKUP_FONTES_COMPARTILHADAS || tipo==BACKUP_FONTES_PERFIL) {
    FonteFalhaConfig c;if(!fontefalha_backup_ler(texto,n,&c))return 0;
    return fontefalha_backup_exportar(&c,nativo);
  }
  return 0;
}
int backupplano_criar(const BackupLocal *backup,const char *dono,int perfil,uint64_t sessao,
                      BackupLerAtual ler,void *ctx,BackupPlano **saida) {
  if(saida)*saida=NULL;
  if(!saida || !ler || !backuplocal_compativel(backup,dono,perfil))return BACKUP_PLANO_CONTEXTO;
  if(!backuplocal_quantidade(backup))return BACKUP_PLANO_INVALIDO;
  BackupPlano *p=calloc(1,sizeof *p);if(!p)return BACKUP_PLANO_INVALIDO;
  snprintf(p->dono,sizeof p->dono,"%s",dono);p->perfil=perfil;p->sessao=sessao;p->criado=backuplocal_criado(backup);
  int erro=BACKUP_PLANO_INVALIDO;
  for(int i=0;i<BACKUP_PARTES_N;i++) {
    size_t n=0;const char *texto=backuplocal_parte(backup,i,&n);if(!texto)continue;
    // Home row keys can contain configured add-on URLs. A dedicated hashed
    // identity adapter is required; do not back up/restore the raw row file.
    if(i==BACKUP_FILEIRAS){erro=BACKUP_PLANO_NAO_SUPORTADO;goto falha;}
    if(!validar(i,texto,n,&p->parte[i].depois))goto falha;
    p->parte[i].nDepois=strlen(p->parte[i].depois);p->parte[i].usada=1;
  }
  // Every payload is valid before reading the selected current stores.
  for(int i=0;i<BACKUP_PARTES_N;i++)if(p->parte[i].usada) {
    char *atual=NULL;size_t n=0;int estado=ler(ctx,i,&atual,&n);
    if(estado<0 || estado>1 || (estado==0 && (atual || n)) ||
       (estado==1 && (!atual || n>BACKUP_PARTE_MAX-2u || memchr(atual,0,n)))) {
      free(atual);erro=BACKUP_PLANO_LEITURA;goto falha;
    }
    p->parte[i].existia=estado;p->parte[i].antes=atual;p->parte[i].nAntes=n;
    if(i==BACKUP_AJUSTES_APARELHO || i==BACKUP_AJUSTES_PERFIL) {
      char *entrada=atual?copiar(atual,n):copiar("",0),*mesclado=NULL;
      size_t tamanho=0;const char *snapshot=backuplocal_parte(backup,i,&tamanho);(void)tamanho;
      if(!entrada || !ajustes_backup_mesclar(i==BACKUP_AJUSTES_PERFIL,entrada,snapshot,&mesclado)) {
        free(entrada);goto falha;
      }
      free(entrada);free(p->parte[i].depois);p->parte[i].depois=mesclado;p->parte[i].nDepois=strlen(mesclado);
    }
  }
  *saida=p;return BACKUP_PLANO_OK;
falha:
  backupplano_liberar(p);return erro;
}
