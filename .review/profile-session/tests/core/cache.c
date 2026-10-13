#define main legacy_cache_fixture_main
#include "catcache.c"
#undef main
#include <dirent.h>
int main(void){
 char dir[]="/tmp/nuvio-profile-cache-XXXXXX"; assert(mkdtemp(dir)); snprintf(dirDados,sizeof dirDados,"%s",dir);
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-A"); perfilAtual=1;
 cat_cache_perfis(0); semearCatalogo("legacy-A1"); assert(cat_gravar_cache(NULL));
 cat_cache_perfis(1); cat_definir_tudo(NULL,0,NULL,0); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"legacy-A1"));
 perfilAtual=2; cat_definir_tudo(NULL,0,NULL,0); assert(!cat_ler_cache(NULL)); assert(cat_n()==0);
 assert(existe(dir,"catalogo-rede.bin"));
 semearCatalogo("A2"); assert(cat_gravar_cache(NULL));
 perfilAtual=1; cat_definir_tudo(NULL,0,NULL,0); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"legacy-A1"));
 perfilAtual=2; cat_definir_tudo(NULL,0,NULL,0); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"A2"));
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-B"); perfilAtual=1; cat_definir_tudo(NULL,0,NULL,0); assert(!cat_ler_cache(NULL));
 semearCatalogo("B1"); assert(cat_gravar_cache(NULL));
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-A"); perfilAtual=1; assert(cat_ler_cache(NULL)); assert(cat_apagar_cache());
 perfilAtual=2; cat_definir_tudo(NULL,0,NULL,0); assert(!cat_ler_cache(NULL));
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-B"); perfilAtual=1; assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"B1"));
 { char orphan[700]=""; DIR*d=opendir(dir); struct dirent*e;
   while((e=readdir(d))) if(strstr(e->d_name,"-p1.bin")){snprintf(orphan,sizeof orphan,"%s/%s.tmp-orphan",dir,e->d_name);break;}
   closedir(d);assert(orphan[0]);FILE*f=fopen(orphan,"w");assert(f);fputs("private signed URLs",f);fclose(f);
   assert(cat_apagar_cache()); assert(access(orphan,F_OK)!=0); } rmdir(dir);
 puts("PASS: scoped warm Home caches survive profile changes, reject other owners, safely migrate legacy, and logout removes only current account snapshots");
}
