#include "observed-addons.inc"
static const char *tmdbKeys[10] = {
 "tmdb-addon_movie_tmdb.top", "tmdb-addon_series_tmdb.top",
 "tmdb-addon_movie_tmdb.year", "tmdb-addon_series_tmdb.year",
 "tmdb-addon_movie_tmdb.language", "tmdb-addon_series_tmdb.language",
 "tmdb-addon_movie_tmdb.trending", "tmdb-addon_series_tmdb.trending",
 "tmdb-addon_movie_tmdb.latest", "tmdb-addon_series_tmdb.latest"
};
static int baselineTabs[2][FIL_MAX],actualTabs[2][FIL_MAX];
static char baselineKeys[FIL_MAX][FIL_CHAVE];
static int isTmdb(int i){return !strncmp(fil_chave(i),"tmdb-addon_",11);}
static void actualBoth(int tabs[2][FIL_MAX]) {
 memset(tabs,0,2*FIL_MAX*sizeof(int));
 assert(fil_n()==FIL_MAX);
 for(int tab=0;tab<2;tab++) {
  filAba=tab;filMontarLista();int sentinel=0;
  for(int k=0;k<filListaN;k++) {
   int i=filLista[k];
   if(i==AJ_FIL_DESTAQUE){assert(!tab);sentinel++;continue;}
   assert(i>=0&&i<FIL_MAX&&!tabs[tab][i]);tabs[tab][i]=1;
  }
  assert(sentinel==(!tab));
 }
}
static void fullSnapshot(int installed,int enabled,int unknown) {
 addonN=installed?8:7;
 for(int i=0;i<7;i++){addonId[i]=observedAddonIds[i];addonBase[i]=observedAddonBases[i];addonEnabled[i]=1;}
 if(installed){addonId[7]=unknown?"":"tmdb-addon";addonBase[7]="https://tmdb-fixture.invalid";addonEnabled[7]=enabled;}
 nDecl=0;
 for(int i=0;i<fil_n();i++)if(installed||!isTmdb(i)){
  snprintf(decls[nDecl++].chave,sizeof decls[0].chave,"%s",fil_chave(i));
 }
 versaoManifestos=addonVersion;addonProfile=1;
}
static void fullCheck(const char *phase,int policy,const char *before,int beforeWrites) {
 int tmdb[2]={0},other[2]={0};actualBoth(actualTabs);
 for(int i=0;i<FIL_MAX;i++){
  assert(!strcmp(baselineKeys[i],fil_chave(i)));
  for(int tab=0;tab<2;tab++){
   if(isTmdb(i)){
    int expected=policy==1?0:policy==2?(tab==1):baselineTabs[tab][i];
    assert(actualTabs[tab][i]==expected);tmdb[tab]+=actualTabs[tab][i];
   }else{assert(actualTabs[tab][i]==baselineTabs[tab][i]);other[tab]+=actualTabs[tab][i];}
  }
 }
 assert(other[0]+other[1]==FIL_MAX-10);
 if(policy==1)assert(tmdb[0]==0&&tmdb[1]==0);
 if(policy==0)assert(tmdb[0]==5&&tmdb[1]==5);
 if(policy==2)assert(tmdb[0]==0&&tmdb[1]==10);
 char *after=saved(1);assert(!strcmp(before,after));free(after);assert(writes==beforeWrites);
 for(int k=0;k<10;k++){int i=find(tmdbKeys[k]);assert(i>=0&&fil_linha_tipo(i)==FIL_TIPO_RANKING);}
 assert(!strcmp(fil_hero_fonte(),tmdbKeys[0]));
 printf("PASS tmdb-full phase=%s known=%d addons=%d tmdb_home=%d tmdb_outside=%d other_home=%d other_outside=%d preferences=byte-identical actual_projection=both\n",phase,fil_n(),addonN,tmdb[0],tmdb[1],other[0],other[1]);
}
static void fullTmdb(void) {
 size_t cap=(size_t)FIL_MAX*400+256,used=0;char*b=malloc(cap);assert(b);
 used+=(size_t)snprintf(b+used,cap-used,"limite 7\nordem 0\nmigracao 197\n");
 for(int i=0;i<FIL_MAX;i++){
  char key[FIL_CHAVE];
  if(i<10)snprintf(key,sizeof key,"%s",tmdbKeys[i]);
  else snprintf(key,sizeof key,"%s_movie_saved%03d",observedAddonIds[i<15?4:3],i);
  used+=(size_t)snprintf(b+used,cap-used,"linha %s\t%d\t0\t1\t0\tSaved %03d\n",key,i&1,i);
  assert(used<cap);
 }
 dados_gravar("fileirasui-p1.txt",b);free(b);fil_definir_perfil(1);fil_teste_recarregar();assert(fil_n()==FIL_MAX);
 for(int k=0;k<10;k++)assert(fil_definir_tipo(tmdbKeys[k],FIL_TIPO_RANKING));
 fil_ciclar_tam(find(tmdbKeys[0]));fil_definir_hero_fonte(tmdbKeys[0]);assert(fil_mover(find(tmdbKeys[0]),1)>=0);fil_gravar_registro();
 int savedSize=fil_linha_tam(find(tmdbKeys[0]));
 fullSnapshot(1,1,0);reconcile();actualBoth(baselineTabs);
 for(int i=0;i<FIL_MAX;i++)snprintf(baselineKeys[i],sizeof baselineKeys[i],"%s",fil_chave(i));
 char*before=saved(1);int beforeWrites=writes;fullCheck("installed-eight",0,before,beforeWrites);
 fullSnapshot(0,1,0);reconcile();fullCheck("removed-seven",1,before,beforeWrites);
 fullSnapshot(1,1,0);reconcile();fullCheck("reinstalled-eight",0,before,beforeWrites);
 assert(fil_linha_tam(find(tmdbKeys[0]))==savedSize);
 fullSnapshot(1,0,0);reconcile();fullCheck("installed-disabled",2,before,beforeWrites);
 fullSnapshot(1,0,1);reconcile();fullCheck("disabled-unknown-id",2,before,beforeWrites);
 fullSnapshot(1,1,0);reconcile();fullCheck("identified-enabled",0,before,beforeWrites);
 fullSnapshot(0,1,0);addonVersion++;reconcile();fullCheck("changed-generation-deferred",0,before,beforeWrites);
 versaoManifestos=addonVersion;reconcile();fullCheck("stable-generation-replay",1,before,beforeWrites);
 assert(fil_linha_tam(find(tmdbKeys[0]))==savedSize);free(before);
}
