// Real Settings/session/profile/player/source owners with private fixture files.
// This exercises local capture/restore, not Google authorization or playback.
#include "../src/ajustes.c"
#include "backupapp.h"
#include "backupdisco.h"
#include "playerbackup.h"
#include <assert.h>
static const char sessaoFixture[]="eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ1c2VyLTEiLCJleHAiOjQxMDI0NDQ4MDB9.test-signature\nfixture-refresh\n0\n";
static void igual(const char *nome,const char *esperado) {
  char *s=dados_ler(nome);assert(s && !strcmp(s,esperado));free(s);
}
int main(void) {
  dados_iniciar(getenv("NUVIO_DADOS"));assert(dados_dir()[0]);
  BackupContexto c;assert(!backupapp_contexto(&c));
  assert(dados_gravar("sessao.txt",sessaoFixture));sessao_iniciar();
  assert(sessao_logada() && !strcmp(sessao_usuario(),"user-1"));
  assert(dados_gravar("perfil.txt","7\n"));perfis_carregar_ativo();assert(perfis_ativo()==7);
  ajustes_dir(dados_dir());player_dir(dados_dir());
  assert(backupapp_contexto(&c) && c.perfil==7 && !strcmp(c.usuario,"user-1"));
  assert(dados_gravar("debrid.txt","fixture-account-must-stay-local\n"));
  assert(dados_gravar("xtream.txt","fixture-iptv-must-stay-local\n"));
  valor[AJ_TAMANHO_AJUSTES]=0;valor[AJ_OCULTAR_FILMES_VISTOS]=1;
  assert(fontefalha_definir_provedores(1));
  PlayerBackup pl;player_backup_capturar(&pl);pl.estilo.tamanho=150;
  assert(player_leg_estilo_definir(&pl.estilo,PLR_LEG_FONTE));
  BackupLocal *b=NULL;assert(backupapp_capturar(&c,1792330000,&b) && b && backuplocal_quantidade(b)==4);
  size_t n=0;const char *s=backuplocal_parte(b,BACKUP_AJUSTES_APARELHO,&n);
  assert(s && n && !strstr(s,CHAVE[AJ_INTERFACE_MODO]) && !strstr(s,CHAVE[AJ_PROVEDORES_ESCOPO]));
  char *pacote=NULL;size_t np=0;assert(backuplocal_exportar(b,&pacote,&np));
  assert(!strstr(pacote,"fixture-refresh") && !strstr(pacote,"fixture-account") && !strstr(pacote,"fixture-iptv"));free(pacote);
  // Target credentials, hardware and Nuvio-managed values remain the target's.
  valor[AJ_TAMANHO_AJUSTES]=1;valor[AJ_OCULTAR_FILMES_VISTOS]=0;
  valor[AJ_HERO]=1;valor[AJ_INTERFACE_MODO]=0;valor[AJ_TEX_MB]=2;
  assert(gravar());ajustes_perfil_guardar(7);
  assert(fontefalha_definir_provedores(0));pl.estilo.tamanho=120;assert(player_leg_estilo_definir(&pl.estilo,0));
  assert(backupapp_validar(&c,b));
  assert(backupapp_restaurar(&c,b)==BACKUP_RESTORE_OK);
  assert(valor[AJ_TAMANHO_AJUSTES]==0 && valor[AJ_OCULTAR_FILMES_VISTOS]==1);
  assert(valor[AJ_HERO]==1 && valor[AJ_INTERFACE_MODO]==0 && valor[AJ_TEX_MB]==2);
  player_backup_capturar(&pl);assert(pl.estilo.tamanho==150 && (pl.tocado&PLR_LEG_FONTE));
  assert(fontefalha_provedores());igual("sessao.txt",sessaoFixture);
  igual("debrid.txt","fixture-account-must-stay-local\n");igual("xtream.txt","fixture-iptv-must-stay-local\n");
  assert(backupdisco_recuperar_arranque(dados_dir())==BACKUP_RESTORE_NADA);
  // A shared policy backup follows the TARGET's existing private policy scope,
  // without changing account sharing or the old shared source configuration.
  assert(fontefalha_definir_provedores(0));char *compartilhada=dados_ler("fontefalha.txt");assert(compartilhada);
  assert(ajustes_definir_provedores_compartilhados(0));
  char *contaPrivada=dados_ler("debrid-user-1-p7.txt");assert(contaPrivada);
  assert(backupapp_restaurar(&c,b)==BACKUP_RESTORE_OK && !ajustes_provedores_compartilhados());
  assert(fontefalha_provedores());igual("fontefalha.txt",compartilhada);free(compartilhada);
  igual("debrid-user-1-p7.txt",contaPrivada);free(contaPrivada);igual("provider-scope-user-1-p7.txt","1\n");
  s=NULL;char *privada=dados_ler("fontefalha-user-1-p7.txt");assert(privada && strstr(privada,"provedores 1\n"));free(privada);
  // Invalid modules fail before target writes; active memory stays unchanged.
  char *antes=dados_ler("ajustes.txt");assert(antes);
  BackupLocal *ruim=backuplocal_novo("user-1",7,1792330000);assert(ruim);
  assert(backuplocal_adicionar(ruim,BACKUP_PLAYER,"bad",3));
  assert(!backupapp_validar(&c,ruim));assert(backupapp_restaurar(&c,ruim)==BACKUP_RESTORE_INVALIDO);
  igual("ajustes.txt",antes);assert(valor[AJ_TAMANHO_AJUSTES]==0);backuplocal_liberar(ruim);
  // Session epochs and profile ownership are checked again at confirmation.
  perfis_invalidar_sessao();assert(!backupapp_validar(&c,b));
  assert(backupapp_restaurar(&c,b)==BACKUP_RESTORE_CONTEXTO);igual("ajustes.txt",antes);
  assert(backupapp_contexto(&c));assert(dados_gravar("perfil.txt","8\n"));perfis_carregar_ativo();
  assert(backupapp_contexto(&c) && c.perfil==8 && !backupapp_validar(&c,b));
  assert(backupapp_restaurar(&c,b)==BACKUP_RESTORE_INVALIDO);igual("ajustes.txt",antes);
  igual("sessao.txt",sessaoFixture);free(antes);backuplocal_liberar(b);
  puts("PASS: active local capture and actual store/runtime restore, source scope mapping, Nuvio/account/hardware preservation, invalid modules and stale/profile-mismatched confirmations");
}
