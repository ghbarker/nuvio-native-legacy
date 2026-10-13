#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((int)((B)-(A)) <= 0)
#endif
#include "ajustes.c"
#include <assert.h>
static char testDir[500];
static int testProfile = 1;
static const char *testUser = "account-a";
const char *sessao_usuario(void) { return testUser; }
int perfis_ativo(void) { return testProfile; }
char *dados_caminho(char *dst, unsigned n, const char *nome) { snprintf(dst,n,"%s/%s",testDir,nome); return dst; }
char *dados_ler(const char *nome) { char p[640];dados_caminho(p,sizeof p,nome);FILE *f=fopen(p,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);char *r=calloc((size_t)n+1,1);assert(r);assert(fread(r,1,(size_t)n,f)==(size_t)n);fclose(f);return r; }
int dados_gravar(const char *nome,const char *text) { char p[640];dados_caminho(p,sizeof p,nome);FILE *f=fopen(p,"w");if(!f)return 0;int ok=fputs(text,f)>=0;return fclose(f)==0&&ok; }
void dados_fs_travar(void) {} void dados_fs_liberar(void) {} void dados_marcar_sujo(int leve) { (void)leve; }
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 1; }
const char *ling_opcao_codigo(int i) { (void)i;return ""; }
void ling_conta_legenda(const char *s) { (void)s; }
void ling_conta_legenda2(const char *s) { (void)s; }
void ling_conta_audio(const char *s) { (void)s; }
const char *ling_conta_legenda_valor(void) { return ""; }
const char *ling_conta_legenda2_valor(void) { return ""; }
const char *ling_conta_audio_valor(void) { return ""; }
void ling_local_legenda(const char *s) { (void)s; } void ling_local_legenda2(const char *s) { (void)s; } void ling_local_audio(const char *s) { (void)s; }
static void keyIs(const char *s) { DebridContexto c;debrid_contexto_capturar(&c);assert(!strcmp(c.chave[1],s)); }
int main(void) {
 char d[]="/tmp/nuvio-provider-profile-XXXXXX",name[128],path[640];assert(mkdtemp(d));strcpy(testDir,d);
#ifdef NUVIO_PROFILE_COMBINED
 fontefalha_dir(testDir);
 assert(fontefalha_definir_provedores(1));assert(fontefalha_definir_addons(1));
 assert(fontefalha_provedor_mover(1,-1));assert(fontefalha_provedor_definir_limite(1,7));
 FonteFiltroConfig filters={0};filters.maxGB=25;filters.excluir[FONTEFILTRO_LINGUAS]=fontefiltro_bit(FONTEFILTRO_LINGUAS,0);strcpy(filters.grupos,"group-one");assert(fontefalha_filtro_definir(&filters));
 char addonKey[FONTEFALHA_CHAVE];fontefalha_addon_chave("https://example.invalid/addon-a",addonKey,sizeof addonKey);assert(fontefalha_addon_definir_limite(addonKey,4));
#endif
 assert(dados_gravar("debrid.txt","torbox=SHARED-INITIAL\n"));
 debrid_abrir_sessao(1);ajustes_provedores_abrir_sessao();assert(ajustes_provedores_compartilhados());keyIs("SHARED-INITIAL");
 assert(ajustes_definir_provedores_compartilhados(0));assert(!ajustes_provedores_compartilhados());keyIs("SHARED-INITIAL");
 assert(ajustes_provedores_arquivo(name,sizeof name,"debrid"));assert(!strcmp(name,"debrid-account-a-p1.txt"));dados_caminho(path,sizeof path,name);struct stat st;assert(stat(path,&st)==0&&(st.st_mode&0777)==0600);
 debDefinir(AJ_DEBRID_TB,"PRIVATE-P1");keyIs("PRIVATE-P1");
#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_limite(1)==7&&fontefalha_provedor_posicao(1)==0);assert(fontefalha_addon_limite(addonKey)==4);fontefalha_filtro(&filters);assert(filters.maxGB==25&&filters.excluir[FONTEFILTRO_LINGUAS]);
#endif
 testProfile=2;debrid_abrir_sessao(2);ajustes_provedores_abrir_sessao();assert(ajustes_provedores_compartilhados());keyIs("SHARED-INITIAL");
 debDefinir(AJ_DEBRID_TB,"SHARED-UPDATED");keyIs("SHARED-UPDATED");
#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_definir_limite(1,2));assert(fontefalha_addon_definir_limite(addonKey,6));fontefalha_filtro(&filters);filters.maxGB=99;assert(fontefalha_filtro_definir(&filters));
#endif
 testProfile=1;debrid_abrir_sessao(3);ajustes_provedores_abrir_sessao();assert(!ajustes_provedores_compartilhados());keyIs("PRIVATE-P1");

#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_limite(1)==7);assert(fontefalha_addon_limite(addonKey)==4);fontefalha_filtro(&filters);assert(filters.maxGB==25);assert(valor[AJ_FALHA_TB_LIMITE]==7);
#endif
 assert(ajustes_definir_provedores_compartilhados(1));keyIs("SHARED-UPDATED");
#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_limite(1)==2);assert(fontefalha_addon_limite(addonKey)==6);fontefalha_filtro(&filters);assert(filters.maxGB==99);
#endif
 assert(ajustes_definir_provedores_compartilhados(0));keyIs("PRIVATE-P1");
#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_limite(1)==7);assert(fontefalha_addon_limite(addonKey)==4);fontefalha_filtro(&filters);assert(filters.maxGB==25);
#endif
 testUser="account-b";debrid_abrir_sessao(4);ajustes_provedores_abrir_sessao();assert(ajustes_provedores_compartilhados());keyIs("SHARED-UPDATED");assert(ajustes_definir_provedores_compartilhados(0));keyIs("SHARED-UPDATED");debDefinir(AJ_DEBRID_TB,"PRIVATE-ACCOUNT-B");keyIs("PRIVATE-ACCOUNT-B");

#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_limite(1)==2);assert(fontefalha_provedor_definir_limite(1,9));
#endif
 testUser="account-a";debrid_abrir_sessao(5);ajustes_provedores_abrir_sessao();keyIs("PRIVATE-P1");

#ifdef NUVIO_PROFILE_COMBINED
 assert(fontefalha_provedor_limite(1)==7);fontefalha_filtro(&filters);assert(filters.maxGB==25);
#endif
 debDefinir(AJ_DEBRID_TB,"");keyIs("");assert(ajustes_definir_provedores_compartilhados(1));assert(ajustes_definir_provedores_compartilhados(0));keyIs("");
 testProfile=3;debrid_abrir_sessao(6);ajustes_provedores_abrir_sessao();debrid_definir_chave("premiumize","CLOUD-P3");assert(ajustes_definir_provedores_compartilhados(0));DebridContexto c;debrid_contexto_capturar(&c);assert(!strcmp(c.chave[2],"CLOUD-P3"));assert(debrid_origem("premiumize")==2);
 // Personal defaults do not reset hardware choices or the provider-mode bank.
 valor[AJ_HERO]=1;valor[AJ_RESOLUCAO]=RES_4K;valor[AJ_ADDONS_PRINCIPAL]=0;ajustes_perfil_padrao();assert(valor[AJ_HERO]==valorPadrao[AJ_HERO]);assert(valor[AJ_RESOLUCAO]==RES_4K);assert(valor[AJ_ADDONS_PRINCIPAL]==0);assert(!ajustes_provedores_compartilhados());
 // A failed first opt-in cannot remove the shared setup or publish a new mode.
 testProfile=4;debrid_abrir_sessao(7);ajustes_provedores_abrir_sessao();strcat(testDir,"/missing");assert(!ajustes_definir_provedores_compartilhados(0));assert(ajustes_provedores_compartilhados());keyIs("SHARED-UPDATED");

#ifdef NUVIO_PROFILE_COMBINED
 puts("preview scope: provider/addon order, limits, filters, mirrored UI, persistence, account separation PASS");
#endif
 puts("provider scope: shared/private roundtrip, reload, accounts, empty copies, cloud clone, 0600, defaults, save failure PASS");
 return 0;
}
