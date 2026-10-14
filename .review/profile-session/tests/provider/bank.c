#define _POSIX_C_SOURCE 200809L
#include "fontefalha.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void same(const FonteFalhaConfig *expected) {
  FonteFalhaConfig actual; fontefalha_config(&actual);
  assert(!memcmp(expected,&actual,sizeof actual));
}
static void touch(const char *dir, const char *name) {
  char path[1200]; snprintf(path,sizeof path,"%s/%s",dir,name);
  FILE *f=fopen(path,"w"); assert(f); assert(fclose(f)==0);
}
int main(void) {
  char dir[]="/tmp/nuvio-profile-bank-XXXXXX",a[FONTEFALHA_CHAVE],b[FONTEFALHA_CHAVE];
  assert(mkdtemp(dir)); fontefalha_dir(dir);
  assert(fontefalha_definir_provedores(1)); assert(fontefalha_definir_addons(1));
  assert(fontefalha_provedor_mover(1,-1)); assert(fontefalha_provedor_definir_limite(1,7));
  fontefalha_addon_chave("https://example.invalid/a",a,sizeof a);
  fontefalha_addon_chave("https://example.invalid/b",b,sizeof b);
  assert(fontefalha_addon_definir_limite(a,4)); assert(fontefalha_addon_definir_limite(b,6));
  assert(fontefalha_addon_mover(b,-1));
  FonteFalhaConfig shared,privateA,empty; fontefalha_config(&shared);
  assert(fontefalha_preparar_privado("fontefalha-account-a-p1.txt"));
  same(&shared); assert(fontefalha_arquivo("fontefalha-account-a-p1.txt")); same(&shared);
  assert(fontefalha_provedor_definir_limite(1,9)); assert(fontefalha_addon_mover(a,-1));
  assert(fontefalha_definir_addons(0)); fontefalha_config(&privateA);
  // Cloning while another private bank is loaded must still read the shared bank.
  assert(fontefalha_preparar_privado("fontefalha-account-a-p2.txt")); same(&privateA);
  assert(fontefalha_arquivo("fontefalha-account-a-p2.txt")); same(&shared);
  assert(fontefalha_arquivo("fontefalha.txt")); same(&shared);
  assert(fontefalha_arquivo("fontefalha-account-a-p1.txt")); same(&privateA);
  assert(fontefalha_preparar_privado("fontefalha-account-a-p1.txt")); same(&privateA);
  assert(fontefalha_arquivo("fontefalha-account-a-p1.txt")); same(&privateA);
  // Deliberately empty private copies are preserved, with independent defaults.
  touch(dir,"fontefalha-account-a-p3.txt");
  assert(fontefalha_preparar_privado("fontefalha-account-a-p3.txt"));
  assert(fontefalha_arquivo("fontefalha-account-a-p3.txt")); fontefalha_config(&empty);
  assert(!empty.provedores&&!empty.addons&&!empty.nAddons);
  for(int i=0;i<4;i++) assert(empty.ordem[i]==i&&empty.limites[i]==3);
  // Missing private bank never falls back to shared preferences or overwrites it.
  assert(fontefalha_arquivo("fontefalha-account-c-p1.txt")); same(&empty);
  assert(fontefalha_arquivo("fontefalha.txt")); same(&shared);
  assert(!fontefalha_arquivo("../fontefalha.txt")); same(&shared);
  assert(!fontefalha_preparar_privado("../fontefalha.txt")); same(&shared);
  assert(!fontefalha_arquivo("")); same(&shared);
  // Resetting the directory restores the legacy shared bank, unchanged on disk.
  assert(fontefalha_arquivo("fontefalha-account-a-p1.txt")); same(&privateA);
  fontefalha_dir(dir); same(&shared);
  puts("upstream bank: shared/private persistence, immutable shared origin, addon order/caps, existing/empty/missing copies, path validation PASS");
  return 0;
}
