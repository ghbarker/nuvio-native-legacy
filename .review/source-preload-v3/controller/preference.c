#define _POSIX_C_SOURCE 200809L
#include SOURCE_PREFERENCE
#include <assert.h>
char *dados_ler(const char *file) { (void)file; assert(!"test seeds captured preference in memory"); return NULL; }
static int eligible = 1;
static int semPerda(int i, void *ctx) { (void)ctx; return i == eligible; }
int main(void) {
  carregado=1; nTab=1; memset(tabela,0,sizeof tabela);
  strcpy(tabela[0].id,"tt-pref"); strcpy(tabela[0].provedor,"Addon A"); strcpy(tabela[0].trilha,"DUB");
  strcpy(tabela[0].bingeGroup,"manual-release"); tabela[0].altura=1080; tabela[0].quandoS=time(NULL);
  Stream next[3]={{0}};
  strcpy(next[0].provedor,"Addon B"); strcpy(next[0].bingeGroup,"other-release"); next[0].altura=2160;
  strcpy(next[1].provedor,"Addon A"); strcpy(next[1].bingeGroup,"manual-release"); next[1].altura=1080;
  strcpy(next[2].provedor,"Addon A"); strcpy(next[2].bingeGroup,"manual-release"); next[2].altura=720;
  assert(fontepref_escolher_lista("tt-pref:2:8",next,3,semPerda,NULL)==1);
  next[1].bingeGroup[0]=next[2].bingeGroup[0]=0;
  strcpy(next[1].rotulo,"1080p Dublado"); strcpy(next[2].rotulo,"720p Original");
  assert(fontepref_escolher_lista("tt-pref:2:8",next,3,semPerda,NULL)==1);
  nTab=0; nAuto=1; memset(tabAuto,0,sizeof tabAuto);
  strcpy(tabAuto[0].id,"tt-pref"); strcpy(tabAuto[0].bingeGroup,"auto-release"); tabAuto[0].quandoS=time(NULL);
  strcpy(next[1].bingeGroup,"auto-release"); strcpy(next[2].bingeGroup,"auto-release"); next[2].altura=2160;
  assert(fontepref_escolher_lista("tt-pref:2:8",next,3,semPerda,NULL)==1);
  eligible=2; assert(fontepref_escolher_lista("tt-pref:2:8",next,3,semPerda,NULL)==2);
  eligible=-1; assert(fontepref_escolher_lista("tt-pref:2:8",next,3,semPerda,NULL)==-1);
  puts("explicit-list memory: manual binge/audio match, automatic same-release quality callback PASS");
}
