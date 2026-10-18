#include "../src/ajustes.c"
#include <assert.h>

static void linha(char *dst,size_t n,int op,int v) {
  snprintf(dst,n,"%s %d\n",CHAVE[op],v);
}
static void recusar(int perfil,const char *atual,const char *corpo) {
  char *out=(char *)1;
  assert(!ajustes_backup_mesclar(perfil,atual,corpo,&out) && !out);
}
int main(void) {
  rotulosDeIdioma();
  int antes[AJ_N];memcpy(antes,valor,sizeof antes);
  char conta[128],aparelho[128],perfil[128],dinamica[128],original[1024];
  linha(conta,sizeof conta,AJ_HERO,1);
  linha(aparelho,sizeof aparelho,AJ_TAMANHO_AJUSTES,0);
  linha(perfil,sizeof perfil,AJ_OCULTAR_FILMES_VISTOS,1);
  linha(dinamica,sizeof dinamica,AJ_TEMA,AJ_TEMA_DINAMICA);
  snprintf(original,sizeof original,"# preserve comment\n%s%s%s%sfutureKey opaque:value",conta,aparelho,perfil,dinamica);
  char *d=NULL,*p=NULL,*out=NULL;
  assert(ajustes_backup_exportar(AJ_BACKUP_APARELHO,original,&d));
  assert(strstr(d,"scope device\n") && strstr(d,aparelho));
  assert(!strstr(d,conta) && !strstr(d,perfil) && !strstr(d,dinamica) && !strstr(d,"futureKey"));
  assert(ajustes_backup_exportar(AJ_BACKUP_PERFIL,original,&p));
  assert(strstr(p,"scope profile\n") && strstr(p,perfil) && strstr(p,dinamica));
  assert(!strstr(p,conta) && !strstr(p,aparelho));
  // Nuvio fixed themes are not exported even when the active runtime theme
  // happens to be dynamic, nor can they be smuggled into a restore.
  char fixa[128];linha(fixa,sizeof fixa,AJ_TEMA,0);
  valor[AJ_TEMA]=AJ_TEMA_DINAMICA;
  assert(ajustes_backup_exportar(AJ_BACKUP_PERFIL,fixa,&out));
  assert(!strstr(out,CHAVE[AJ_TEMA]));free(out);out=NULL;memcpy(valor,antes,sizeof antes);
  char alvo[512];snprintf(alvo,sizeof alvo,"%s%s 0\nfutureKey opaque:value",conta,CHAVE[AJ_OCULTAR_FILMES_VISTOS]);
  assert(ajustes_backup_mesclar(AJ_BACKUP_PERFIL,alvo,p,&out));
  assert(strstr(out,conta) && strstr(out,perfil) && strstr(out,"futureKey opaque:value\n"));
  assert(strstr(out,dinamica));free(out);out=NULL;
  // A no-op empty backup preserves every byte, including missing final LF.
  assert(ajustes_backup_mesclar(AJ_BACKUP_APARELHO,original,"NUVIO-LOCAL-SETTINGS 1\nscope device\n",&out));
  assert(!strcmp(out,original));free(out);out=NULL;
  recusar(AJ_BACKUP_APARELHO,original,p);
  recusar(AJ_BACKUP_PERFIL,original,d);
  char mau[1024];
  snprintf(mau,sizeof mau,"NUVIO-LOCAL-SETTINGS 1\nscope profile\n%s%s",perfil,conta);
  recusar(AJ_BACKUP_PERFIL,original,mau);
  snprintf(mau,sizeof mau,"NUVIO-LOCAL-SETTINGS 1\nscope profile\n%s%s",perfil,fixa);
  recusar(AJ_BACKUP_PERFIL,original,mau);
  snprintf(mau,sizeof mau,"NUVIO-LOCAL-SETTINGS 1\nscope profile\n%s%s",perfil,perfil);
  recusar(AJ_BACKUP_PERFIL,original,mau);
  const char *ruins[]={"99999999999999999999999999","-9999999999999999999999","1x","1 trailing","","-1","2"};
  for(size_t i=0;i<sizeof ruins/sizeof *ruins;i++) {
    snprintf(mau,sizeof mau,"NUVIO-LOCAL-SETTINGS 1\nscope profile\n%s %s\n",CHAVE[AJ_OCULTAR_FILMES_VISTOS],ruins[i]);
    recusar(AJ_BACKUP_PERFIL,original,mau);
  }
  recusar(AJ_BACKUP_PERFIL,original,"NUVIO-LOCAL-SETTINGS 2\nscope profile\n");
  recusar(AJ_BACKUP_PERFIL,original,"NUVIO-LOCAL-SETTINGS 1\nscope profile\n../../sessao.txt 1\n");
  recusar(AJ_BACKUP_PERFIL,original,"NUVIO-LOCAL-SETTINGS 1\nscope profile\nunknown 1\n");
  // Last persisted occurrence wins when exporting; merge writes it only once.
  snprintf(mau,sizeof mau,"%s 0\n%s",CHAVE[AJ_OCULTAR_FILMES_VISTOS],perfil);
  assert(ajustes_backup_exportar(AJ_BACKUP_PERFIL,mau,&out));
  assert(strstr(out,perfil) && !strstr(strstr(out,perfil)+strlen(perfil),perfil));
  free(out);out=NULL;
  snprintf(mau,sizeof mau,"%s%s",perfil,perfil);
  assert(ajustes_backup_mesclar(AJ_BACKUP_PERFIL,mau,p,&out));
  assert(strstr(out,perfil) && !strstr(strstr(out,perfil)+strlen(perfil),perfil));
  free(out);out=NULL;
  // Automatic language and explicit memory-budget markers are local metadata.
  assert(ajustes_backup_exportar(AJ_BACKUP_APARELHO,"idiomaAutoLocal 1\ntexturasEscolhaLocal 1\n",&out));
  assert(strstr(out,"idiomaAutoLocal 1") && strstr(out,"texturasEscolhaLocal 1"));free(out);out=NULL;
  // Main subtitle appearance and failover rows are backend views. Never
  // export stale scalar shadows or accept an ineffective scalar restore.
  int externos[]={AJ_LEG1_TAMANHO,AJ_LEG1_COR,AJ_FALHA_PROVEDORES,AJ_FILTRO_TAM_MAX};
  for(size_t i=0;i<sizeof externos/sizeof *externos;i++) {
    char sombra[128];linha(sombra,sizeof sombra,externos[i],valorPadrao[externos[i]]);
    assert(ajustes_backup_exportar(AJ_BACKUP_APARELHO,sombra,&out));
    assert(!strstr(out,CHAVE[externos[i]]));free(out);out=NULL;
    snprintf(mau,sizeof mau,"NUVIO-LOCAL-SETTINGS 1\nscope device\n%s",sombra);
    recusar(AJ_BACKUP_APARELHO,original,mau);
  }
  // Secondary subtitle overrides really are owned by Settings.
  char secundaria[128];linha(secundaria,sizeof secundaria,AJ_LEG2_COR,1);
  assert(ajustes_backup_exportar(AJ_BACKUP_APARELHO,secundaria,&out));
  assert(strstr(out,secundaria));free(out);out=NULL;
  char *grande=malloc(AB_MAX_TEXTO+2);assert(grande);memset(grande,'x',AB_MAX_TEXTO+1);grande[AB_MAX_TEXTO+1]=0;
  assert(!ajustes_backup_exportar(0,grande,&out) && !out);
  recusar(0,original,grande);free(grande);
  // A valid input at the limit cannot grow beyond the same limit on merge.
  grande=malloc(AB_MAX_TEXTO+1);assert(grande);memset(grande,'x',AB_MAX_TEXTO);grande[AB_MAX_TEXTO]=0;
  recusar(AJ_BACKUP_PERFIL,grande,p);free(grande);
  assert(!memcmp(valor,antes,sizeof antes));
  free(d);free(p);
  puts("PASS: local-only scalar export/merge; cloud values preserved, profile/device scopes isolated, dynamic-theme exception, complete validation and no runtime mutation");
  return 0;
}
