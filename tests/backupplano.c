#include "../src/ajustes.c"
#include "backupplano_interno.h"
#include "playerbackup.h"
#include <assert.h>
static int leituras,falhar;
static char atual[512];
static int lerAtual(void *ctx,int tipo,char **texto,size_t *n) {
  (void)ctx;leituras++;*texto=NULL;*n=0;if(falhar)return -1;
  if(tipo!=BACKUP_AJUSTES_APARELHO && tipo!=BACKUP_AJUSTES_PERFIL)return 0;
  *texto=strdup(atual);assert(*texto);*n=strlen(atual);return 1;
}
static BackupLocal *novo(void) {BackupLocal *b=backuplocal_novo("user-1",7,1792330000);assert(b);return b;}
int main(void) {
  rotulosDeIdioma();int antes[AJ_N];memcpy(antes,valor,sizeof antes);
  char raw[512],*perfil=NULL,*aparelho=NULL,*fontes=NULL,*player=NULL;
  snprintf(raw,sizeof raw,"%s 1\n",CHAVE[AJ_OCULTAR_FILMES_VISTOS]);
  assert(ajustes_backup_exportar(AJ_BACKUP_PERFIL,raw,&perfil));
  snprintf(raw,sizeof raw,"%s 0\n",CHAVE[AJ_TAMANHO_AJUSTES]);
  assert(ajustes_backup_exportar(AJ_BACKUP_APARELHO,raw,&aparelho));
  FonteFalhaConfig c;fontefalha_config(&c);c.provedores=1;assert(fontefalha_backup_exportar(&c,&fontes));
  PlayerBackup pb={.aspecto=PLR_ASP_CROP,.estilo={.tamanho=150,.familia=TXT_FAMILIA_INTER,.corBorda=-1},.tocado=PLR_LEG_FONTE};
  assert(playerbackup_exportar(&pb,&player));
  snprintf(atual,sizeof atual,"%s 1\nfutureKey opaque:value\n",CHAVE[AJ_HERO]);
  BackupLocal *b=novo();
  assert(backuplocal_adicionar(b,BACKUP_AJUSTES_PERFIL,perfil,strlen(perfil)));
  assert(backuplocal_adicionar(b,BACKUP_AJUSTES_APARELHO,aparelho,strlen(aparelho)));
  assert(backuplocal_adicionar(b,BACKUP_PLAYER,player,strlen(player)));
  assert(backuplocal_adicionar(b,BACKUP_FONTES_COMPARTILHADAS,fontes,strlen(fontes)));
  BackupPlano *p=NULL;
  assert(backupplano_criar(b,"user-1",7,33,lerAtual,NULL,&p)==BACKUP_PLANO_OK && p);
  assert(leituras==4 && p->sessao==33 && !strcmp(p->dono,"user-1") && p->perfil==7);
  assert(strstr(p->parte[BACKUP_AJUSTES_PERFIL].depois,atual));
  assert(p->parte[BACKUP_AJUSTES_PERFIL].existia && !strcmp(p->parte[BACKUP_AJUSTES_PERFIL].antes,atual));
  assert(!p->parte[BACKUP_PLAYER].existia && !p->parte[BACKUP_PLAYER].antes);
  assert(playerbackup_ler(p->parte[BACKUP_PLAYER].depois,p->parte[BACKUP_PLAYER].nDepois,&pb));
  assert(pb.estilo.tamanho==150 && pb.tocado==PLR_LEG_FONTE);
  assert(fontefalha_backup_ler(p->parte[BACKUP_FONTES_COMPARTILHADAS].depois,p->parte[BACKUP_FONTES_COMPARTILHADAS].nDepois,&c) && c.provedores);
  backupplano_liberar(p);p=NULL;leituras=0;
  assert(backupplano_criar(b,"user-2",7,33,lerAtual,NULL,&p)==BACKUP_PLANO_CONTEXTO && !p && !leituras);
  assert(backupplano_criar(b,"user-1",8,33,lerAtual,NULL,&p)==BACKUP_PLANO_CONTEXTO && !p && !leituras);
  falhar=1;assert(backupplano_criar(b,"user-1",7,33,lerAtual,NULL,&p)==BACKUP_PLANO_LEITURA && !p);falhar=0;
  backuplocal_liberar(b);b=novo();leituras=0;
  assert(backuplocal_adicionar(b,BACKUP_AJUSTES_PERFIL,perfil,strlen(perfil)));
  assert(backuplocal_adicionar(b,BACKUP_FONTES_PERFIL,"malformed",9));
  assert(backupplano_criar(b,"user-1",7,33,lerAtual,NULL,&p)==BACKUP_PLANO_INVALIDO && !p && !leituras);
  backuplocal_liberar(b);b=novo();
  assert(backuplocal_adicionar(b,BACKUP_FILEIRAS,"configured-url-row",18));
  assert(backupplano_criar(b,"user-1",7,33,lerAtual,NULL,&p)==BACKUP_PLANO_NAO_SUPORTADO && !p && !leituras);
  assert(!memcmp(antes,valor,sizeof antes));
  backuplocal_liberar(b);free(perfil);free(aparelho);free(fontes);free(player);
  puts("PASS: full restore planning validates every module before reads, preserves Nuvio values, binds identity/session and never changes active settings");
}
