#define _POSIX_C_SOURCE 200809L
#include "fontefalha.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static atomic_int run = 1, observed;
static void valid(const FonteFiltroConfig *f) {
  assert(f->maxGB == 1 || f->maxGB == 2);
  for (int i = 0; i < FONTEFILTRO_N; i++) assert(f->excluir[i] == fontefiltro_bit(i, (int)f->maxGB - 1));
  assert(!strcmp(f->grupos, f->maxGB == 1 ? "GROUPA" : "GROUPB"));
}
static void *reader(void *u) {
  (void)u;
  while (atomic_load(&run)) {
    FonteFalhaConfig c; FonteFiltroConfig f;
    fontefalha_config(&c); valid(&c.filtro);
    fontefalha_filtro(&f); valid(&f);
    assert(fontefalha_filtro_ativo());
    atomic_fetch_add(&observed, 1);
  }
  return NULL;
}
static void set(int pattern) {
  FonteFiltroConfig f = {.maxGB = pattern};
  for (int i = 0; i < FONTEFILTRO_N; i++) f.excluir[i] = fontefiltro_bit(i, pattern - 1);
  strcpy(f.grupos, pattern == 1 ? "GROUPA" : "GROUPB"); assert(fontefalha_filtro_definir(&f));
}
int main(int argc, char **argv) {
  assert(argc == 2); fontefalha_dir(argv[1]); set(1);
  pthread_t readers[3];
  for (int i = 0; i < 3; i++) assert(!pthread_create(&readers[i], NULL, reader, NULL));
  for (int i = 0; i < 100; i++) set((i & 1) + 1);
  atomic_store(&run, 0);
  for (int i = 0; i < 3; i++) pthread_join(readers[i], NULL);
  assert(atomic_load(&observed) > 0);
  puts("config_concurrency: coherent complete filter snapshots during 100 saved changes and three readers PASS");
}
