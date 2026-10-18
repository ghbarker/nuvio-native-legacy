// Actual native owners, local fixture files, no Google/network/playback.
#include "../src/ajustes.c"
#include "backupapp.h"
#include "backupdisco.h"
#include "backupstores.h"
#include <assert.h>
static const char session[]="eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ1c2VyLTEiLCJleHAiOjQxMDI0NDQ4MDB9.test-signature\nfixture-refresh\n0\n";
static void same(const char *name,const char *expected) {
 char *s=dados_ler(name);assert(s && !strcmp(s,expected));free(s);
}
static void coverage(void) {
 int native=0,account=0;
 for(int i=0;i<AJ_N;i++) {
  if(!CHAVE[i] || CHAVE[i][0]=='-' || OPCOES[i].tipo==OP_ACAO || OPCOES[i].tipo==OP_LEITURA || i==AJ_PROVEDORES_ESCOPO || i==AJ_ENVIO_AUTO)continue;
  int scopes=0;
  for(int k=0;k<=AJ_BACKUP_HARDWARE;k++)scopes+=abLocal(i,k,valor[i]);
  if(ajustePortavel(i)){assert(scopes==0);account++;continue;}
  assert(scopes==1);native++;
  int old=valor[i],low=OPCOES[i].tipo==OP_NUMERO?OPCOES[i].min:0;
  int high=OPCOES[i].tipo==OP_NUMERO?OPCOES[i].max:nValores(i)-1;
  for(int v=low;;v=high) {
   valor[i]=v;int scope=abLocal(i,AJ_BACKUP_HARDWARE,v)?AJ_BACKUP_HARDWARE:abLocal(i,AJ_BACKUP_PERFIL,v)?AJ_BACKUP_PERFIL:AJ_BACKUP_APARELHO;
   char line[128],*b=NULL,*merged=NULL;snprintf(line,sizeof line,"%s %d\n",CHAVE[i],v);
   assert(ajustes_backup_capturar(scope,&b) && strstr(b,line));
   assert(ajustes_backup_mesclar(scope,"unknownKey untouched\n",b,&merged));
   assert(strstr(merged,line) && strstr(merged,"unknownKey untouched\n"));free(b);free(merged);
   if(v==high)break;
  }
  valor[i]=old;
 }
 printf("PASS: %d local settings round-trip; %d account-sync settings excluded using the actual sync policy\n",native,account);
}
int main(void) {
 dados_iniciar(getenv("NUVIO_DADOS"));assert(dados_dir()[0]);
 assert(dados_gravar("sessao.txt",session));sessao_iniciar();
 assert(dados_gravar("perfil.txt","7\n"));perfis_carregar_ativo();
 ajustes_dir(dados_dir());coverage();
 // Verify that excluded preferences really enter the native account payload,
 // while hardware choices and credentials do not. No account HTTP is used.
 char *cloud=NULL;assert(ajustes_mesclar_blob("{}",&cloud)>0 && cloud);
 assert(strstr(cloud,"c_settings_v1") && strstr(cloud,"c_legendaIdioma") && strstr(cloud,"c_audioIdioma"));
 assert(!strstr(cloud,"c_interfaceModo") && !strstr(cloud,"fixture-refresh"));free(cloud);
 BackupContexto c;assert(backupapp_contexto(&c));
 assert(dados_gravar("trakt-p7.txt","destination-rotating-token\n"));
 assert(dados_gravar("discord-p7.txt","destination-discord-token\n"));
 assert(dados_gravar("fileirasui-p7.txt","ordem 1\nlimite 7\n"));
 assert(dados_gravar("ltv-urls-p7.txt","m3u\tSource\thttps://source.invalid/list\n"));
 assert(dados_gravar("guia-ltv-fav-p7.txt","m3u:source\tchannel-1\n"));
 assert(dados_gravar("ltv-epgmapa-p7.txt","m3u:source\tchannel-1\txmltv:guide\tprogram-1\n"));
 assert(dados_gravar("ltv-indice-p7.txt","generated-channel-cache-index\n"));
 valor[AJ_QUALIDADE_IMG]=0;valor[AJ_INTERFACE_MODO]=1;
 valor[AJ_TAMANHO_AJUSTES]=0;valor[AJ_LEG_LINGUA]=1;
 assert(gravar());ajustes_perfil_guardar(7);
 BackupLocal *b=NULL;assert(backupapp_capturar(&c,1792330000,&b));
 assert(!backuplocal_parte(b,BACKUP_PLAYER,NULL));
 assert(!backuplocal_parte(b,BACKUP_FONTES_COMPARTILHADAS,NULL));
 assert(!backuplocal_parte(b,BACKUP_FONTES_PERFIL,NULL));
 char *wire=NULL;size_t n=0;assert(backuplocal_exportar(b,&wire,&n));
 assert(!strstr(wire,"fixture-refresh") && !strstr(wire,"destination-rotating-token") && !strstr(wire,"destination-discord-token") && !strstr(wire,"generated-channel-cache-index"));
 assert(!strstr(wire,"legendaIdioma ") && !strstr(wire,"ordem 1\n"));free(wire);
 valor[AJ_INTERFACE_MODO]=0;valor[AJ_QUALIDADE_IMG]=1;valor[AJ_TAMANHO_AJUSTES]=1;valor[AJ_LEG_LINGUA]=2;
 assert(gravar());ajustes_perfil_guardar(7);
 assert(dados_gravar("fileirasui-p7.txt","ordem 0\nlimite 3\n"));
 assert(dados_gravar("ltv-urls-p7.txt","m3u\tDestination\thttps://destination.invalid/list\n"));
 assert(dados_gravar("guia-ltv-fav-p7.txt","changed\n"));
 assert(dados_gravar("ltv-epgmapa-p7.txt","changed\n"));
 BackupLocal *selected=NULL;assert(backupapp_selecionar(&c,b,0,&selected));
 assert(!backuplocal_parte(selected,BACKUP_AJUSTES_HARDWARE,NULL));
 assert(backupapp_validar(&c,selected));assert(backupapp_restaurar(&c,selected)==BACKUP_RESTORE_OK);
 assert(valor[AJ_INTERFACE_MODO]==0 && valor[AJ_QUALIDADE_IMG]==1 && valor[AJ_TAMANHO_AJUSTES]==1 && valor[AJ_LEG_LINGUA]==2);
 same("fileirasui-p7.txt","ordem 0\nlimite 3\n");
 same("guia-ltv-fav-p7.txt","m3u:source\tchannel-1\n");
 same("ltv-epgmapa-p7.txt","m3u:source\tchannel-1\txmltv:guide\tprogram-1\n");
 same("ltv-urls-p7.txt","m3u\tDestination\thttps://destination.invalid/list\nm3u\tSource\thttps://source.invalid/list\n");
 assert(backupapp_restaurar(&c,selected)==BACKUP_RESTORE_OK);
 same("ltv-urls-p7.txt","m3u\tDestination\thttps://destination.invalid/list\nm3u\tSource\thttps://source.invalid/list\n");
 // Hardware opt-in restores display choices, never the account's languages.
 assert(backupapp_restaurar(&c,b)==BACKUP_RESTORE_OK);
 assert(valor[AJ_INTERFACE_MODO]==1 && valor[AJ_QUALIDADE_IMG]==0 && valor[AJ_TAMANHO_AJUSTES]==0 && valor[AJ_LEG_LINGUA]==2);
 same("fileirasui-p7.txt","ordem 0\nlimite 7\n");
 same("sessao.txt",session);same("trakt-p7.txt","destination-rotating-token\n");same("discord-p7.txt","destination-discord-token\n");
 // Legacy scalar payloads cannot replay account-managed language preferences.
 char *merged=NULL;assert(ajustes_backup_mesclar(AJ_BACKUP_PERFIL,"legendaIdioma 2\n","NUVIO-LOCAL-SETTINGS 1\nscope profile\nlegendaIdioma 1\n",&merged));
 assert(!strcmp(merged,"legendaIdioma 2\n"));free(merged);
 // Earlier encrypted previews kept display settings in the device section.
 // They remain restorable, but must still require the hardware opt-in.
 BackupLocal *legacy=backuplocal_novo(c.usuario,c.perfil,1792330001);
 const char *legacyDevice="NUVIO-LOCAL-SETTINGS 1\nscope device\nqualidadeImagem 2\nitensFileiraLocal 0\ntamanhoUiLocal 0\ntamanhoAjustesLocal 1\n";
 assert(backuplocal_adicionar(legacy,BACKUP_AJUSTES_APARELHO,legacyDevice,strlen(legacyDevice)));
 BackupLocal *legacySelected=NULL;
 assert(backupapp_selecionar(&c,legacy,0,&legacySelected));
 assert(backupapp_validar(&c,legacySelected));
 assert(!backuplocal_parte(legacySelected,BACKUP_AJUSTES_HARDWARE,NULL));
 assert(!strstr(backuplocal_parte(legacySelected,BACKUP_AJUSTES_APARELHO,NULL),"qualidadeImagem"));
 int oldQuality=valor[AJ_QUALIDADE_IMG];
 assert(backupapp_restaurar(&c,legacySelected)==BACKUP_RESTORE_OK && valor[AJ_QUALIDADE_IMG]==oldQuality);
 backuplocal_liberar(legacySelected);legacySelected=NULL;
 assert(backupapp_selecionar(&c,legacy,1,&legacySelected));
 assert(backupapp_validar(&c,legacySelected));
 assert(strstr(backuplocal_parte(legacySelected,BACKUP_AJUSTES_HARDWARE,NULL),"qualidadeImagem 2\n"));
 assert(backupapp_restaurar(&c,legacySelected)==BACKUP_RESTORE_OK && valor[AJ_QUALIDADE_IMG]==2);
 backuplocal_liberar(legacySelected);backuplocal_liberar(legacy);
 // Ignore account-managed dynamic choices without requiring the old font or
 // language list to be installed on the destination. Never replay the value.
 const char *oldSections[3]={0};
 char *newSections[3]={0};
 // Use the actual persisted key, independent of the displayed font label.
 char oldFont[160];snprintf(oldFont,sizeof oldFont,"NUVIO-LOCAL-SETTINGS 1\nscope device\n%s 99\n",CHAVE[AJ_FONTE_UI]);oldSections[0]=oldFont;
 assert(ajustes_backup_selecionar(oldSections,0,newSections));
 assert(!strstr(newSections[0],CHAVE[AJ_FONTE_UI]));
 for(int i=0;i<3;i++)free(newSections[i]);
 oldSections[0]="NUVIO-LOCAL-SETTINGS 1\nscope device\nunknownKey 0\n";
 assert(ajustes_backup_selecionar(oldSections,0,newSections));
 assert(!strstr(newSections[0],"unknownKey"));
 for(int i=0;i<3;i++)free(newSections[i]);
 oldSections[0]="NUVIO-LOCAL-SETTINGS 1\nscope device\nqualidadeImagem 1\n";
 oldSections[2]="NUVIO-LOCAL-SETTINGS 1\nscope hardware\nqualidadeImagem 2\n";
 assert(!ajustes_backup_selecionar(oldSections,1,newSections));
 oldSections[0]="NUVIO-LOCAL-SETTINGS 1\nscope device\ninterfaceModoLocal 1\n";oldSections[2]=NULL;
 assert(ajustes_backup_selecionar(oldSections,0,newSections));
 assert(!strstr(newSections[0],"interfaceModoLocal"));
 for(int i=0;i<3;i++)free(newSections[i]);
 // Mixed-version backups restore compatible settings and connections even
 // when adjacent settings were removed, changed type/range, or moved scope.
 // Keep current values for skipped settings; never print the rejected values.
 BackupLocal *future=backuplocal_novo(c.usuario,c.perfil,1792330002);assert(future);
 const char *futureDevice="NUVIO-LOCAL-SETTINGS 1\nscope device\nremovedFutureSetting PRIVATE_BACKUP_SENTINEL\ninterfaceModoLocal 1\n";
 char futureHardware[512];snprintf(futureHardware,sizeof futureHardware,
   "NUVIO-LOCAL-SETTINGS 1\nscope hardware\nqualidadeImagem 999\ntamanhoAjustesLocal 1\n%s PRIVATE_BACKUP_SENTINEL\n%s 99999999999999999999999\n",
   CHAVE[AJ_BUFFER],CHAVE[AJ_TEX_MB]);
 assert(backuplocal_adicionar(future,BACKUP_AJUSTES_APARELHO,futureDevice,strlen(futureDevice)));
 assert(backuplocal_adicionar(future,BACKUP_AJUSTES_HARDWARE,futureHardware,strlen(futureHardware)));
 size_t storesSize=0;const char *stores=backuplocal_parte(b,BACKUP_LOCAIS,&storesSize);assert(stores);
 assert(backuplocal_adicionar(future,BACKUP_LOCAIS,stores,storesSize));
 int oldBuffer=valor[AJ_BUFFER],oldMemory=valor[AJ_TEX_MB];
 valor[AJ_INTERFACE_MODO]=0;valor[AJ_QUALIDADE_IMG]=1;valor[AJ_TAMANHO_AJUSTES]=0;assert(gravar());
 BackupLocal *futureSelected=NULL;assert(backupapp_selecionar(&c,future,0,&futureSelected));
 assert(backupapp_validar(&c,futureSelected));assert(backupapp_restaurar(&c,futureSelected)==BACKUP_RESTORE_OK);
 assert(valor[AJ_TAMANHO_AJUSTES]==0);backuplocal_liberar(futureSelected);
 assert(backupapp_selecionar(&c,future,1,&futureSelected));
 assert(backupapp_validar(&c,futureSelected));assert(backupapp_restaurar(&c,futureSelected)==BACKUP_RESTORE_OK);
 assert(valor[AJ_TAMANHO_AJUSTES]==1 && valor[AJ_INTERFACE_MODO]==0 && valor[AJ_QUALIDADE_IMG]==1);
 assert(valor[AJ_BUFFER]==oldBuffer && valor[AJ_TEX_MB]==oldMemory);
 same("ltv-urls-p7.txt","m3u\tDestination\thttps://destination.invalid/list\nm3u\tSource\thttps://source.invalid/list\n");
 same("sessao.txt",session);same("trakt-p7.txt","destination-rotating-token\n");
 // The plan's direct merge path has the same skip behaviour. A rejected
 // setting does not erase the original persisted line or its current value.
 assert(ajustes_backup_mesclar(AJ_BACKUP_HARDWARE,"qualidadeImagem 1\ntamanhoAjustesLocal 0\n",futureHardware,&merged));
 assert(strstr(merged,"qualidadeImagem 1\n") && strstr(merged,"tamanhoAjustesLocal 1\n"));free(merged);
 assert(backupapp_validar(&c,future));assert(backupapp_restaurar(&c,future)==BACKUP_RESTORE_OK);
 assert(valor[AJ_INTERFACE_MODO]==0 && valor[AJ_QUALIDADE_IMG]==1);
 backuplocal_liberar(futureSelected);backuplocal_liberar(future);
 // Version/identity and duplicate-key ambiguity are still structural errors.
 oldSections[0]="NUVIO-LOCAL-SETTINGS 99\nscope device\n";
 assert(!ajustes_backup_selecionar(oldSections,1,newSections));
 BackupLocal *bad=backuplocal_novo(c.usuario,c.perfil,1792330001);assert(bad);
 assert(backuplocal_adicionar(bad,BACKUP_AJUSTES_APARELHO,"bad",3));
 assert(!backupapp_validar(&c,bad) && backupapp_restaurar(&c,bad)==BACKUP_RESTORE_INVALIDO);backuplocal_liberar(bad);
 perfis_invalidar_sessao();assert(!backupapp_validar(&c,b));assert(backupapp_restaurar(&c,b)==BACKUP_RESTORE_CONTEXTO);
 backuplocal_liberar(selected);backuplocal_liberar(b);
 puts("PASS: actual local restore, account/settings/token preservation, hardware opt-in, URL merge, repeated restore and stale-session rejection");
 return 0;
}
