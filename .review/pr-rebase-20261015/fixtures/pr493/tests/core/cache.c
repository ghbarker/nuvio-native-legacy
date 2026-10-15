#define perfis_ativo legacy_perfis_ativo
#define main legacy_cache_fixture_main
#include "catcache.c"
#undef main
#undef perfis_ativo
#include "perfis.h"
#include <dirent.h>
int dados_gravar(const char *n, const char *t) { (void)n; (void)t; return 1; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
int main(void){
 char dir[]="/tmp/nuvio-profile-cache-XXXXXX"; assert(mkdtemp(dir)); snprintf(dirDados,sizeof dirDados,"%s",dir);
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-A"); perfis_definir_ativo(1);
 cat_cache_perfis(0); semearCatalogo("legacy-A1"); assert(cat_gravar_cache(NULL));
 cat_cache_perfis(1); cat_definir_tudo(NULL,0,NULL,0); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"legacy-A1"));
 perfis_definir_ativo(2); cat_definir_tudo(NULL,0,NULL,0); assert(!cat_ler_cache(NULL)); assert(cat_n()==0);
 assert(existe(dir,"catalogo-rede.bin"));
 semearCatalogo("A2"); assert(cat_gravar_cache(NULL));
 perfis_definir_ativo(1); cat_definir_tudo(NULL,0,NULL,0); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"legacy-A1"));
 perfis_definir_ativo(2); cat_definir_tudo(NULL,0,NULL,0); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"A2"));
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-B"); perfis_invalidar_sessao(); perfis_definir_ativo(1); cat_definir_tudo(NULL,0,NULL,0); assert(!cat_ler_cache(NULL));
 semearCatalogo("B1"); assert(cat_gravar_cache(NULL));
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-A"); perfis_invalidar_sessao(); perfis_definir_ativo(1); assert(cat_ler_cache(NULL)); assert(cat_apagar_cache());
 perfis_definir_ativo(2); cat_definir_tudo(NULL,0,NULL,0); assert(!cat_ler_cache(NULL));
 snprintf(usuarioAtual,sizeof usuarioAtual,"cache-owner-B"); perfis_invalidar_sessao(); perfis_definir_ativo(1); assert(cat_ler_cache(NULL)); assert(strstr(cat_item(0)->titulo,"B1"));
 { char orphan[700]=""; DIR*d=opendir(dir); struct dirent*e;
   while((e=readdir(d))) if(strstr(e->d_name,"-p1.bin")){snprintf(orphan,sizeof orphan,"%s/%s.tmp-orphan",dir,e->d_name);break;}
   closedir(d);assert(orphan[0]);FILE*f=fopen(orphan,"w");assert(f);fputs("private signed URLs",f);fclose(f);
   assert(cat_apagar_cache()); assert(access(orphan,F_OK)!=0); } rmdir(dir);
 puts("PASS: scoped warm Home caches survive profile changes, reject other owners, safely migrate legacy, and logout removes only current account snapshots");
}
