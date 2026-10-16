#include "fileiras.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
static const char *dir;
static int writes;
int dados_gravar(const char *name,const char *text){char path[1024];snprintf(path,sizeof path,"%s/%s",dir,name);FILE*f=fopen(path,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));writes++;return 1;}
char*dados_caminho(char*out,unsigned size,const char*name){snprintf(out,size,"%s/%s",dir,name);return out;}
void fil_teste_recarregar(void);
static int filAba,filLista[FIL_MAX+1],filListaN,filSep,filFoco,filForaAgrupada=1;
#define AJ_FIL_DESTAQUE (-2)
#include "settings-projection.inc"
#define DECL_MAX 1024
static struct {char chave[FIL_CHAVE];}decls[DECL_MAX];
static int nDecl;
static unsigned versaoManifestos=1,addonVersion=1;
static int addonN,addonProfile=1,addonEnabled[16];
static const char *addonId[16],*addonBase[16];
int addons_n(void){return addonN;}
const char*addons_id_manifesto(int i){return addonId[i];}
const char*addons_base(int i){return addonBase[i];}
int addons_ativo(int i){return addonEnabled[i];}
unsigned addons_versao(void){return addonVersion;}
int addons_perfil_da_lista(void){return addonProfile;}
static void reconcile(void){
#include "discovery-guards.inc"
}
static int find(const char *key){for(int i=0;i<fil_n();i++)if(!strcmp(fil_chave(i),key))return i;return-1;}
static int listed(const char*key,int tab){filAba=tab;filMontarLista();for(int i=0;i<filListaN;i++)if(filLista[i]>=0&&!strcmp(fil_chave(filLista[i]),key))return 1;return 0;}
static void absent(const char*key){int a=listed(key,0),b=listed(key,1);if(a||b)fprintf(stderr,"REMOVED_ROW_STILL_IN_SETTINGS key=%s home=%d outside=%d\n",key,a,b);assert(!a&&!b);}
static void present(const char*key,int tab){int yes=listed(key,tab);if(!yes)fprintf(stderr,"EXPECTED_AVAILABLE_ROW_MISSING key=%s tab=%d\n",key,tab);assert(yes);assert(!listed(key,!tab));}
static char*saved(int profile){char path[1024];snprintf(path,sizeof path,"%s/fileirasui-p%d.txt",dir,profile);FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>=0);rewind(f);char*b=calloc((size_t)n+1,1);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);return b;}
static void snapshot(int n){addonN=n;for(int i=0;i<n;i++){addonId[i]=i?"removed":"alive";addonBase[i]=i?"https://removed.test/config":"https://alive.test/config";addonEnabled[i]=1;}nDecl=1;snprintf(decls[0].chave,sizeof decls[0].chave,"alive_movie_main");}
static void setup(void){fil_definir_perfil(1);fil_registrar("alive_movie_main","Alive","Alive addon","movie",20);fil_registrar("removed_movie_custom","Saved custom row","Removed addon","movie",20);fil_registrar("removed_series_hidden","Saved hidden row","Removed addon","series",20);fil_registrar("continue_watching","Continue watching","","",4);fil_registrar("collection_group","Collection","","",4);fil_gravar_registro();assert(fil_definir_tipo("removed_movie_custom",FIL_TIPO_RANKING));fil_ciclar_tam(find("removed_movie_custom"));fil_remover(find("removed_series_hidden"));fil_definir_hero_fonte("removed_movie_custom");assert(fil_mover(find("removed_movie_custom"),-1)>=0);snapshot(2);reconcile();present("alive_movie_main",0);present("removed_movie_custom",0);present("removed_series_hidden",1);}
static void preserved(const char*before){char*after=saved(1);assert(!strcmp(before,after));free(after);assert(!strcmp(fil_hero_fonte(),"removed_movie_custom"));int i=find("removed_movie_custom");assert(i>=0&&fil_linha_tipo(i)==FIL_TIPO_RANKING&&fil_linha_tam(i)==FIL_TAM_GRANDE);assert(find("removed_series_hidden")>=0);}
static void full(const char*mode){int outside=!strcmp(mode,"full-outside");size_t cap=(size_t)FIL_MAX*200+256,used=0;char*b=malloc(cap);assert(b);used+=(size_t)snprintf(b+used,cap-used,"limite %d\nordem 0\nmigracao 197\n",FIL_LIMITE_MAX);for(int i=0;i<FIL_MAX;i++)used+=(size_t)snprintf(b+used,cap-used,"linha alive_movie_%03d\t%d\t0\t1\t0\tTitle %03d\n",i,outside?1:0,i);dados_gravar("fileirasui-p1.txt",b);free(b);fil_definir_perfil(1);fil_teste_recarregar();assert(fil_n()==FIL_MAX);filFoco=99999;filAba=outside;filMontarLista();assert(filListaN==FIL_MAX+(outside?0:1));assert(filFoco==filListaN+(outside?1:0));if(!outside)assert(filLista[0]==AJ_FIL_DESTAQUE);for(int k=outside?0:1;k<filListaN;k++)assert(filLista[k]>=0&&filLista[k]<FIL_MAX);filFoco=-999;filMontarLista();assert(filFoco==0);printf("PASS %s entries=%d focus=%d sentinel=%d\n",mode,filListaN,filFoco,!outside);}
#include "full-tmdb.inc"
int main(int argc,char**argv){assert(argc==3);const char*mode=argv[1];dir=argv[2];if(!strcmp(mode,"full-tmdb")){fullTmdb();return 0;}if(!strncmp(mode,"full-",5)){full(mode);return 0;}setup();char*before=saved(1);int beforeWrites=writes;
 if(!strcmp(mode,"removed")||!strcmp(mode,"reinstall")){snapshot(1);reconcile();absent("removed_movie_custom");absent("removed_series_hidden");present("alive_movie_main",0);present("continue_watching",0);present("collection_group",0);preserved(before);if(!strcmp(mode,"reinstall")){snapshot(2);reconcile();present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);}}
 else if(!strcmp(mode,"unknown-disabled")||!strcmp(mode,"identity-complete-after-unknown")){fil_registrar("oldid_movie_top","Saved disabled addon","Unknown disabled addon","movie",4);fil_remover(find("oldid_movie_top"));fil_gravar_registro();free(before);before=saved(1);snapshot(3);addonId[2]="";addonBase[2]="https://unrelated-base.test/config";addonEnabled[2]=0;reconcile();present("oldid_movie_top",1);present("alive_movie_main",0);preserved(before);if(!strcmp(mode,"identity-complete-after-unknown")){addonId[2]="new-known-id";reconcile();absent("oldid_movie_top");preserved(before);}}
 else if(!strcmp(mode,"unknown-reinstalled")){snapshot(1);reconcile();absent("removed_movie_custom");absent("removed_series_hidden");snapshot(2);addonId[1]="";addonEnabled[1]=0;reconcile();present("removed_movie_custom",1);present("removed_series_hidden",1);present("alive_movie_main",0);assert(fil_linha_sem_addon(find("removed_movie_custom")));preserved(before);}
 else if(!strcmp(mode,"cached-home")){snapshot(1);reconcile();absent("removed_movie_custom");FilPassada pass=fil_passada_ler();fil_registrar_de(&pass,"removed_movie_custom","Saved custom row","Removed addon","movie",20);absent("removed_movie_custom");absent("removed_series_hidden");preserved(before);}
 else if(!strcmp(mode,"failed-after-removal")){snapshot(1);reconcile();absent("removed_movie_custom");snapshot(2);addonId[0]="";reconcile();absent("removed_movie_custom");absent("removed_series_hidden");preserved(before);}
 else if(!strcmp(mode,"pending")){present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);}
 else if(!strcmp(mode,"failed-manifest")){snapshot(2);addonId[0]="";reconcile();present("removed_movie_custom",0);present("removed_series_hidden",1);present("alive_movie_main",0);preserved(before);}
 else if(!strcmp(mode,"changed-list")){snapshot(1);addonVersion++;reconcile();present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);}
 else if(!strcmp(mode,"other-profile")||!strcmp(mode,"package-list")){snapshot(1);addonProfile=!strcmp(mode,"other-profile")?2:0;reconcile();present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);}
 else if(!strcmp(mode,"disabled")){snapshot(2);addonEnabled[1]=0;reconcile();present("removed_movie_custom",1);present("removed_series_hidden",1);present("alive_movie_main",0);preserved(before);addonEnabled[1]=1;reconcile();present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);}
 else if(!strcmp(mode,"empty-list")){snapshot(0);nDecl=0;reconcile();absent("removed_movie_custom");absent("removed_series_hidden");absent("alive_movie_main");present("continue_watching",0);present("collection_group",0);preserved(before);}
 else if(!strcmp(mode,"raw-base")){fil_registrar("https://legacy.test/config_movie_older","Legacy base key","Legacy addon","movie",4);fil_remover(find("https://legacy.test/config_movie_older"));fil_gravar_registro();free(before);before=saved(1);snapshot(3);addonId[2]="legacy";addonBase[2]="https://legacy.test/config";reconcile();present("https://legacy.test/config_movie_older",1);snapshot(2);reconcile();absent("https://legacy.test/config_movie_older");preserved(before);}
 else if(!strcmp(mode,"prefix-boundary")){fil_registrar("aliveextra_movie_other","Not alive addon","Other","movie",4);fil_remover(find("aliveextra_movie_other"));fil_gravar_registro();free(before);before=saved(1);snapshot(1);reconcile();absent("aliveextra_movie_other");present("alive_movie_main",0);preserved(before);}
 else if(!strcmp(mode,"same-profile-reopen")){snapshot(1);reconcile();absent("removed_movie_custom");fil_definir_perfil(1);absent("removed_movie_custom");fil_teste_recarregar();present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);snapshot(1);reconcile();absent("removed_movie_custom");preserved(before);}
 else if(!strcmp(mode,"profile-switch")){snapshot(1);reconcile();absent("removed_movie_custom");fil_definir_perfil(2);addonProfile=2;fil_registrar("second_movie_only","Second profile","Second addon","movie",3);fil_gravar_registro();assert(fil_n()==1);present("second_movie_only",0);assert(!listed("alive_movie_main",0)&&!listed("alive_movie_main",1));fil_definir_perfil(1);addonProfile=1;present("removed_movie_custom",0);present("removed_series_hidden",1);preserved(before);snapshot(1);reconcile();absent("removed_movie_custom");preserved(before);}
 else assert(!"unknown case");
 assert(writes==beforeWrites||!strcmp(mode,"raw-base")||!strcmp(mode,"prefix-boundary")||!strcmp(mode,"profile-switch")||!strcmp(mode,"unknown-disabled")||!strcmp(mode,"identity-complete-after-unknown"));printf("PASS %s rows=%d settings_projection=actual saved_preferences=preserved\n",mode,fil_n());free(before);return 0;}
