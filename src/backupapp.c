#include "backupapp.h"
#include "backupdisco.h"
#include "ajustes.h"
#include "dados.h"
#include "fontefalha.h"
#include "perfis.h"
#include "player.h"
#include "playerbackup.h"
#include "sessao.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static int atual(const BackupContexto *c) {
  return c && memchr(c->usuario,0,sizeof c->usuario) && c->usuario[0] && c->perfil>0 &&
    perfis_sessao_atual(c->sessao) && perfis_ativo()==c->perfil && !strcmp(sessao_usuario(),c->usuario);
}
int backupapp_contexto(BackupContexto *saida) {
  if(!saida)return 0;
  memset(saida,0,sizeof *saida);BackupContexto c={0};char jwt[3000];
  c.sessao=perfis_sessao_geracao();
  // Token mutex BEFORE the profile mutex, like session renewal. The token is
  // not part of the context/snapshot and is discarded before taking the lock.
  int ok=sessao_identidade_local_copiar(c.usuario,sizeof c.usuario,jwt,sizeof jwt);
  volatile char *p=jwt;for(size_t i=0;i<sizeof jwt;i++)p[i]=0;
  if(!ok || !perfis_sessao_travar(c.sessao))return 0;
  c.perfil=perfis_ativo();ok=atual(&c);
  if(ok)*saida=c;
  perfis_sessao_destravar();return ok;
}
static int contexto(const BackupContexto *c) {
  if(!c || !memchr(c->usuario,0,sizeof c->usuario))return 0;
  BackupContexto agora;
  return backupapp_contexto(&agora) && c->sessao==agora.sessao && c->perfil==agora.perfil && !strcmp(c->usuario,agora.usuario);
}
static int adicionar(BackupLocal *b,int tipo,char **texto) {
  int ok=*texto && backuplocal_adicionar(b,tipo,*texto,strlen(*texto));
  free(*texto);*texto=NULL;return ok;
}
int backupapp_capturar(const BackupContexto *c,long long criado,BackupLocal **saida) {
  if(saida)*saida=NULL;
  if(!saida || !contexto(c) || !perfis_sessao_travar(c->sessao))return 0;
  BackupLocal *b=NULL;char *texto=NULL;int ok=0;
  if(!atual(c) || !(b=backuplocal_novo(c->usuario,c->perfil,criado)))goto fim;
  if(!ajustes_backup_capturar(AJ_BACKUP_APARELHO,&texto) || !adicionar(b,BACKUP_AJUSTES_APARELHO,&texto) ||
     !ajustes_backup_capturar(AJ_BACKUP_PERFIL,&texto) || !adicionar(b,BACKUP_AJUSTES_PERFIL,&texto))goto fim;
  PlayerBackup pl;player_backup_capturar(&pl);
  if(!playerbackup_exportar(&pl,&texto) || !adicionar(b,BACKUP_PLAYER,&texto))goto fim;
  FonteFalhaConfig cfg;fontefalha_config(&cfg);
  int tipo=ajustes_provedores_compartilhados()?BACKUP_FONTES_COMPARTILHADAS:BACKUP_FONTES_PERFIL;
  if(!fontefalha_backup_exportar(&cfg,&texto) || !adicionar(b,tipo,&texto) || !atual(c))goto fim;
  *saida=b;b=NULL;ok=1;
fim:
  free(texto);backuplocal_liberar(b);perfis_sessao_destravar();return ok;
}
static BackupLocal *normalizar(const BackupContexto *c,const BackupLocal *origem) {
  if(!backuplocal_compativel(origem,c->usuario,c->perfil))return NULL;
  int nfontes=!!backuplocal_parte(origem,BACKUP_FONTES_COMPARTILHADAS,NULL)+
              !!backuplocal_parte(origem,BACKUP_FONTES_PERFIL,NULL);
  if(nfontes>1)return NULL;
  BackupLocal *b=backuplocal_novo(c->usuario,c->perfil,backuplocal_criado(origem));if(!b)return NULL;
  int destino=ajustes_provedores_compartilhados()?BACKUP_FONTES_COMPARTILHADAS:BACKUP_FONTES_PERFIL;
  for(int i=0;i<BACKUP_PARTES_N;i++) {
    size_t n=0;const char *s=backuplocal_parte(origem,i,&n);if(!s)continue;
    int tipo=(i==BACKUP_FONTES_COMPARTILHADAS || i==BACKUP_FONTES_PERFIL)?destino:i;
    if(!backuplocal_adicionar(b,tipo,s,n)){backuplocal_liberar(b);return NULL;}
  }
  return b;
}
static int vazio(void *ctx,int tipo,char **s,size_t *n){(void)ctx;(void)tipo;*s=NULL;*n=0;return 0;}
int backupapp_validar(const BackupContexto *c,const BackupLocal *backup) {
  if(!contexto(c) || !perfis_sessao_travar(c->sessao))return 0;
  BackupLocal *b=atual(c)?normalizar(c,backup):NULL;BackupPlano *plano=NULL;
  int ok=b && backupplano_criar(b,c->usuario,c->perfil,c->sessao,vazio,NULL,&plano)==BACKUP_PLANO_OK;
  backupplano_liberar(plano);backuplocal_liberar(b);perfis_sessao_destravar();return ok;
}
typedef struct {BackupIO io;const BackupContexto *c;} Atual;
static int lerAtual(void *ctx,int tipo,char **s,size_t *n) {
  Atual *a=ctx;return a->io.ler(a->io.ctx,a->c->usuario,a->c->perfil,tipo,s,n);
}
static int mesmaSessao(void *ctx,const char *dono,int perfil,uint64_t sessao) {
  const BackupContexto *c=ctx;
  return c->perfil==perfil && c->sessao==sessao && !strcmp(dono,c->usuario) && atual(c);
}
int backupapp_restaurar(const BackupContexto *c,const BackupLocal *backup) {
  if(!contexto(c) || !perfis_sessao_travar(c->sessao))return BACKUP_RESTORE_CONTEXTO;
  int resultado=BACKUP_RESTORE_INVALIDO;BackupLocal *b=NULL;BackupPlano *plano=NULL;BackupDisco *disco=NULL;
  if(!atual(c) || player_aberto() || player_mini_ativo() || player_retido() || !(b=normalizar(c,backup)))goto fim;
  disco=backupdisco_abrir(dados_dir(),mesmaSessao,(void *)c);if(!disco){resultado=BACKUP_RESTORE_FALHOU;goto fim;}
  Atual a={.io=backupdisco_io(disco),.c=c};
  int ok=backupplano_criar(b,c->usuario,c->perfil,c->sessao,lerAtual,&a,&plano);
  if(ok!=BACKUP_PLANO_OK){resultado=ok==BACKUP_PLANO_LEITURA?BACKUP_RESTORE_FALHOU:BACKUP_RESTORE_INVALIDO;goto fim;}
  resultado=backuprestauro_aplicar(plano,&a.io);
  if(resultado==BACKUP_RESTORE_OK || resultado==BACKUP_RESTORE_LIMPEZA) {
    // These are the existing owner load paths; do not clone credentials,
    // reinitialize login, replay a cloud blob or push local settings to Nuvio.
    if(backuplocal_parte(b,BACKUP_AJUSTES_APARELHO,NULL) || backuplocal_parte(b,BACKUP_AJUSTES_PERFIL,NULL)) {
      ajustes_backup_recarregar(dados_dir());ajustes_perfil_restaurar(c->perfil);
    }
    if(backuplocal_parte(b,BACKUP_FONTES_COMPARTILHADAS,NULL) || backuplocal_parte(b,BACKUP_FONTES_PERFIL,NULL))
      ajustes_provedores_abrir_sessao();
    if(backuplocal_parte(b,BACKUP_PLAYER,NULL)){player_dir(dados_dir());player_leg_estilo_carregar();}
  }
fim:
  backupplano_liberar(plano);backuplocal_liberar(b);backupdisco_fechar(disco);
  perfis_sessao_destravar();return resultado;
}
