#include "backupapp.h"
#include "backupdisco.h"
#include "ajustes.h"
#include "dados.h"
#include "fontefalha.h"
#include "perfis.h"
#include "player.h"
#include "playerbackup.h"
#include "sessao.h"
#include "backupstores.h"
#include "backupconnections.h"
#include "backupcrypto.h"
#include "backupfonts.h"
#include "legfontes.h"
#include "debrid.h"
#include "fileiras.h"
#include "legmemoria.h"
#include "audiomemoria.h"
#include "arteescolha.h"
#include "fontepref.h"
#include "central.h"
#include "salvosorg.h"
#include "salvospainel.h"
#include "estatvivoui.h"
#include "selospacote.h"
#include "servidores.h"
#include "plugins.h"
#include "curadoria.h"
#include "iptvfontes.h"
#include "xtream.h"
#include "m3u.h"
#include "stalker.h"
#include "iconeapp.h"
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
  if(*texto)backupcrypto_limpar(*texto,strlen(*texto));
  free(*texto);*texto=NULL;return ok;
}
static int provedoresTexto(const DebridConta *c,char out[1024]) {
  size_t n=0;out[0]=0;
  for(int i=0;i<DEBRID_N;i++)if(c->chave[i][0]) {
    int k=snprintf(out+n,1024u-n,"%s=%s\n",debrid_servico_id(i),c->chave[i]);
    if(k<0 || (size_t)k>=1024u-n)return 0;
    n+=(size_t)k;
  }
  return 1;
}
static int provedoresLer(const char *s,DebridConta *c) {
  memset(c,0,sizeof *c);unsigned vistos=0;
  for(const char *p=s;*p;) {
    const char *e=strchr(p,'\n');if(!e)e=p+strlen(p);
    const char *eq=memchr(p,'=',(size_t)(e-p));if(!eq || eq-p>24 || e-eq<=1 || e-eq>200)return 0;
    char nome[25];memcpy(nome,p,(size_t)(eq-p));nome[eq-p]=0;
    int id=debrid_servico_por_nome(nome);if(id<0 || vistos&(1u<<id))return 0;
    vistos|=1u<<id;
    for(const char *v=eq+1;v<e;v++)if(!strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.",*v))return 0;
    memcpy(c->chave[id],eq+1,(size_t)(e-eq-1));p=*e?e+1:e;
  }
  return 1;
}
int backupapp_capturar(const BackupContexto *c,long long criado,BackupLocal **saida) {
  if(saida)*saida=NULL;
  if(!saida || !contexto(c) || !perfis_sessao_travar(c->sessao))return 0;
  BackupLocal *b=NULL;char *texto=NULL;int ok=0;
  if(!atual(c) || !(b=backuplocal_novo(c->usuario,c->perfil,criado)))goto fim;
  if(!ajustes_backup_capturar(AJ_BACKUP_APARELHO,&texto) || !adicionar(b,BACKUP_AJUSTES_APARELHO,&texto) ||
     !ajustes_backup_capturar(AJ_BACKUP_PERFIL,&texto) || !adicionar(b,BACKUP_AJUSTES_PERFIL,&texto) ||
     !ajustes_backup_capturar(AJ_BACKUP_HARDWARE,&texto) || !adicionar(b,BACKUP_AJUSTES_HARDWARE,&texto))goto fim;
  PlayerBackup pl;player_backup_capturar(&pl);
  if(!playerbackup_exportar(&pl,&texto) || !adicionar(b,BACKUP_PLAYER,&texto))goto fim;
  FonteFalhaConfig cfg;fontefalha_config(&cfg);
  int tipo=ajustes_provedores_compartilhados()?BACKUP_FONTES_COMPARTILHADAS:BACKUP_FONTES_PERFIL;
  if(!fontefalha_backup_exportar(&cfg,&texto) || !adicionar(b,tipo,&texto) || !atual(c))goto fim;
  BackupDisco *disco=backupdisco_abrir(dados_dir(),NULL,NULL);
  if(!disco)goto fim;
  BackupIO io=backupdisco_io(disco);char *local=NULL;size_t nl=0;BackupStores *todos=NULL,*portavel=NULL;
  int capturado=io.ler(io.ctx,c->usuario,c->perfil,BACKUP_LOCAIS,&local,&nl)==1 &&
    backupstores_ler(local,nl,&todos) && backupconnections_capturar(todos,&portavel);
  backupcrypto_limpar(local,nl);free(local);local=NULL;backupdisco_fechar(disco);
  if(capturado) {
    backupstores_remover(portavel,perfis_ativo_addons()==1?BS_PLUGINS_PROFILE:BS_PLUGINS_SHARED,0);
    DebridConta locais;char chaves[1024];debrid_backup_local(&locais);
    capturado=provedoresTexto(&locais,chaves) && (!*chaves || backupstores_adicionar(portavel,BS_DEBRID_PROFILE,0,1,chaves,strlen(chaves)));
    backupcrypto_limpar(&locais,sizeof locais);backupcrypto_limpar(chaves,sizeof chaves);
  }
  if(capturado)capturado=backupstores_exportar(portavel,&texto,&nl) && adicionar(b,BACKUP_LOCAIS,&texto);
  backupstores_liberar(todos);backupstores_liberar(portavel);
  if(!capturado || !atual(c))goto fim;
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
  BackupStores *stores=NULL;char *normalizado=NULL;size_t nn=0,ns=0;
  const char *s=backuplocal_parte(origem,BACKUP_LOCAIS,&ns);int privado=0;
  if(s) {
    if(!backupstores_ler(s,ns,&stores))goto falha;
    BackupStore v;DebridConta entradas={0},locais={0};DebridContexto efetivas={0};char chaves[1024];
    int ok=1;
    if(backupstores_buscar(stores,BS_DEBRID_PROFILE,0,&v) && v.existe)ok=provedoresLer(v.texto,&entradas);
    BackupStore plugin;int pluginDestino=perfis_ativo_addons()==1?BS_PLUGINS_SHARED:BS_PLUGINS_PROFILE;
    int pluginOrigem=pluginDestino==BS_PLUGINS_SHARED?BS_PLUGINS_PROFILE:BS_PLUGINS_SHARED;
    if(backupstores_buscar(stores,pluginOrigem,0,&plugin)) {
      if(!backupstores_definir(stores,pluginDestino,0,plugin.existe,plugin.texto,plugin.n))ok=0;
      backupstores_remover(stores,pluginOrigem,0);
    }
    backupstores_remover(stores,BS_DEBRID_SHARED,0);backupstores_remover(stores,BS_PROVIDER_SCOPE,0);backupstores_remover(stores,BS_DEBRID_PROFILE,0);
    debrid_backup_local(&locais);debrid_contexto_capturar(&efetivas);
    for(int i=0;i<DEBRID_N && ok;i++)if(entradas.chave[i][0] && !efetivas.chave[i][0]) {
      memcpy(locais.chave[i],entradas.chave[i],sizeof locais.chave[i]);privado=1;
    }
    if(ok && privado)ok=provedoresTexto(&locais,chaves) &&
      backupstores_adicionar(stores,BS_DEBRID_PROFILE,0,1,chaves,strlen(chaves)) &&
      backupstores_adicionar(stores,BS_PROVIDER_SCOPE,0,1,"1\n",2);
    backupcrypto_limpar(&entradas,sizeof entradas);backupcrypto_limpar(&locais,sizeof locais);
    backupcrypto_limpar(&efetivas,sizeof efetivas);backupcrypto_limpar(chaves,sizeof chaves);
    if(!ok || !backupstores_exportar(stores,&normalizado,&nn))goto falha;
  }
  int destino=privado || !ajustes_provedores_compartilhados()?BACKUP_FONTES_PERFIL:BACKUP_FONTES_COMPARTILHADAS;
  for(int i=0;i<BACKUP_PARTES_N;i++) {
    size_t n=0;const char *parte=backuplocal_parte(origem,i,&n);if(!parte)continue;
    if(i==BACKUP_LOCAIS){parte=normalizado;n=nn;}
    if(i==BACKUP_FONTES_IMPORTADAS) {
      BackupFontManifest m;if(!backupfonts_manifest_ler(parte,n,&m))goto falha;
      if(!m.n)continue;
      BackupFontStage *stage=backuplocal_fontes(origem);const BackupFontManifest *disponivel=backupfonts_stage_manifest(stage);
      if(!disponivel || m.n!=disponivel->n || !legfontes_backup_validar(stage))goto falha;
      for(int j=0;j<m.n;j++)if(strcmp(m.font[j].id,disponivel->font[j].id) || m.font[j].n!=disponivel->font[j].n)goto falha;
      char *plano=NULL;size_t np=0;
      int ok=backupfonts_stage_plano(stage,&plano,&np) && backuplocal_adicionar(b,i,plano,np);
      free(plano);if(!ok)goto falha;
      backuplocal_fontes_vincular(b,stage);continue;
    }
    int tipo=(i==BACKUP_FONTES_COMPARTILHADAS || i==BACKUP_FONTES_PERFIL)?destino:i;
    if(!backuplocal_adicionar(b,tipo,parte,n))goto falha;
  }
  if(privado && !nfontes) {
    FonteFalhaConfig cfg;char *texto=NULL;fontefalha_config(&cfg);
    int ok=fontefalha_backup_exportar(&cfg,&texto) && backuplocal_adicionar(b,BACKUP_FONTES_PERFIL,texto,strlen(texto));
    free(texto);if(!ok)goto falha;
  }
  backupcrypto_limpar(normalizado,nn);free(normalizado);backupstores_liberar(stores);return b;
falha:
  backupcrypto_limpar(normalizado,nn);free(normalizado);backupstores_liberar(stores);backuplocal_liberar(b);return NULL;
}
int backupapp_selecionar(const BackupContexto *c,const BackupLocal *origem,int hardware,BackupLocal **saida) {
  if(saida)*saida=NULL;
  if(!saida || !c || !backuplocal_compativel(origem,c->usuario,c->perfil))return 0;
  BackupLocal *b=backuplocal_novo(c->usuario,c->perfil,backuplocal_criado(origem));if(!b)return 0;
  for(int i=0;i<BACKUP_PARTES_N;i++) {
    if(i==BACKUP_AJUSTES_HARDWARE && !hardware)continue;
    size_t n;const char *s=backuplocal_parte(origem,i,&n);if(!s)continue;
    if(i==BACKUP_LOCAIS && !hardware) {
      BackupStores *todos=NULL,*filtrado=NULL;char *texto=NULL;size_t nt=0;
      int ok=backupstores_ler(s,n,&todos) && backupconnections_selecionar(todos,0,&filtrado);
      if(ok)ok=backupstores_exportar(filtrado,&texto,&nt) && backuplocal_adicionar(b,i,texto,nt);
      backupcrypto_limpar(texto,nt);free(texto);backupstores_liberar(todos);backupstores_liberar(filtrado);
      if(!ok)goto falha;
    } else if(!backuplocal_adicionar(b,i,s,n))goto falha;
  }
  backuplocal_fontes_vincular(b,backuplocal_fontes(origem));
  *saida=b;return 1;
falha:backuplocal_liberar(b);return 0;
}
typedef struct {BackupIO io;const BackupContexto *c;} Atual;
static int lerAtual(void *ctx,int tipo,char **s,size_t *n) {
  Atual *a=ctx;return a->io.ler(a->io.ctx,a->c->usuario,a->c->perfil,tipo,s,n);
}
int backupapp_validar(const BackupContexto *c,const BackupLocal *backup) {
  if(!contexto(c) || !perfis_sessao_travar(c->sessao))return 0;
  BackupLocal *b=atual(c)?normalizar(c,backup):NULL;BackupPlano *plano=NULL;
  BackupDisco *disco=backupdisco_abrir(dados_dir(),NULL,NULL);Atual a={.c=c};
  if(disco)a.io=backupdisco_io(disco);
  int ok=b && disco && backupplano_criar(b,c->usuario,c->perfil,c->sessao,lerAtual,&a,&plano)==BACKUP_PLANO_OK;
  backupplano_liberar(plano);backuplocal_liberar(b);backupdisco_fechar(disco);perfis_sessao_destravar();return ok;
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
fim:
  backupplano_liberar(plano);backupdisco_fechar(disco);
  perfis_sessao_destravar();
  // Network owners may take their own locks before checking the profile epoch.
  // Reload on the UI thread after releasing the transaction's profile lock.
  if((resultado==BACKUP_RESTORE_OK || resultado==BACKUP_RESTORE_LIMPEZA) && atual(c)) {
    // Reload through the native owners after the transaction commits. Nuvio
    // login and account-owned blobs are never replaced or replayed.
    if(backuplocal_parte(b,BACKUP_AJUSTES_APARELHO,NULL) || backuplocal_parte(b,BACKUP_AJUSTES_PERFIL,NULL) || backuplocal_parte(b,BACKUP_AJUSTES_HARDWARE,NULL)) {
      ajustes_backup_recarregar(dados_dir());ajustes_perfil_restaurar(c->perfil);iconeapp_aplicar_plataforma();
    }
    if(backuplocal_parte(b,BACKUP_LOCAIS,NULL) || backuplocal_parte(b,BACKUP_FONTES_COMPARTILHADAS,NULL) || backuplocal_parte(b,BACKUP_FONTES_PERFIL,NULL))
      ajustes_provedores_abrir_sessao();
    if(backuplocal_parte(b,BACKUP_PLAYER,NULL)){player_dir(dados_dir());player_leg_estilo_carregar();}
    if(backuplocal_parte(b,BACKUP_LOCAIS,NULL)) {
      fil_backup_recarregar();central_backup_recarregar();sorg_backup_recarregar();spainel_backup_recarregar();
      estatvivo_backup_recarregar();selospacote_backup_recarregar();
      legmem_definir_perfil(0);legmem_definir_perfil(c->perfil);
      audiomem_definir_perfil(0);audiomem_definir_perfil(c->perfil);
      arteesc_definir_perfil(0);arteesc_definir_perfil(c->perfil);
      fontepref_definir_perfil(0);fontepref_definir_perfil(c->perfil);
      iptv_contas_recarregar();xtream_fontes_invalidar();m3u_fontes_invalidar();stalker_fontes_invalidar();
      servidores_perfil_trocou();plugins_iniciar();
      legfontes_backup_recarregar(dados_dir());curadoria_backup_recarregar();
    }
  }
  backuplocal_liberar(b);return resultado;
}
