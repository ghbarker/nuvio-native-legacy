#define _POSIX_C_SOURCE 200809L
#include "cachedisco.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

static void caminho(char *dst, const char *dir, const char *nome) {
  assert(snprintf(dst, 1024, "%s/%s", dir, nome) < 1024);
}
static void gravar(const char *dir, const char *nome, const char *texto) {
  char p[1024]; FILE *f;
  caminho(p, dir, nome);
  f = fopen(p, "wb"); assert(f);
  assert(fwrite(texto, 1, strlen(texto), f) == strlen(texto));
  assert(!fclose(f));
}
static int existe(const char *dir, const char *nome) {
  char p[1024]; struct stat st;
  caminho(p, dir, nome);
  return !lstat(p, &st);
}
static void igual(const char *dir, const char *nome, const char *texto) {
  char p[1024], buf[100] = {0}; FILE *f;
  caminho(p, dir, nome);
  f = fopen(p, "rb"); assert(f);
  assert(fread(buf, 1, sizeof buf - 1, f) == strlen(texto));
  assert(!fclose(f)); assert(!strcmp(buf, texto));
}
static void apagar(const char *dir, const char *nome) {
  char p[1024]; caminho(p, dir, nome); assert(!unlink(p));
}
static int protegido(const char *p, void *ctx) { return !strcmp(p, ctx); }

int main(int argc, char **argv) {
  char origem[1024], destino[1024], conta[1024], p[1024], alias[1024];
  struct stat st;
  struct utimbuf data = {1000000000, 1000000000};
  long total = 0;
  assert(argc == 2);
  caminho(origem, argv[1], "dados-cache"); assert(!mkdir(origem, 0700));
  caminho(destino, argv[1], "android-cache");
  caminho(conta, argv[1], "contas"); assert(!mkdir(conta, 0700));
  gravar(conta, "ajustes.txt", "subtitle-language=eng");
  gravar(conta, "provedor.txt", "account-secret");
  gravar(origem, "a1b2c3d4.jpg", "image");
  gravar(origem, "a1b2c3d4a1b2c3d4.webp", "wide-image");
  gravar(origem, ".limpo-143", "1");
  gravar(origem, ".arte-negativa", "negative-art");
  gravar(origem, "11111111.png", "old");
  gravar(origem, "ajustes.txt", "do-not-delete");
  gravar(origem, "unknown.bin", "unknown");
  caminho(p, origem, "a1b2c3d4.jpg"); assert(!utime(p, &data));
  caminho(p, origem, "22222222.gif"); assert(!mkdir(p, 0700));
  caminho(p, conta, "provedor.txt");
  caminho(alias, origem, "33333333.jpeg"); assert(!symlink(p, alias));
  assert(!mkdir(destino, 0700));
  gravar(destino, "11111111.png", "new");
  caminho(alias, destino, "44444444.jpg"); assert(!symlink(p, alias));
  gravar(origem, "44444444.jpg", "retry-link");
  caminho(p, destino, "55555555.jpg"); assert(!mkdir(p, 0700));
  gravar(origem, "55555555.jpg", "retry-dir");

  assert(nv_cache_migrar_arte(origem, destino) == 5);
  igual(destino, "a1b2c3d4.jpg", "image");
  igual(destino, "a1b2c3d4a1b2c3d4.webp", "wide-image");
  igual(destino, ".limpo-143", "1");
  igual(destino, ".arte-negativa", "negative-art");
  igual(destino, "11111111.png", "new");
  assert(!existe(origem, "a1b2c3d4.jpg") && !existe(origem, "11111111.png"));
  caminho(p, destino, "a1b2c3d4.jpg"); assert(!stat(p, &st) && st.st_mtime == data.modtime);
  igual(origem, "ajustes.txt", "do-not-delete");
  igual(origem, "unknown.bin", "unknown");
  assert(existe(origem, "22222222.gif") && existe(origem, "33333333.jpeg"));
  igual(conta, "provedor.txt", "account-secret");
  assert(nv_cache_migrar_arte(origem, destino) == 0);

  // Colisoes nao sao sobrescritas; a proxima abertura retoma as pendencias.
  apagar(destino, "44444444.jpg");
  caminho(p, destino, "55555555.jpg"); assert(!rmdir(p));
  assert(nv_cache_migrar_arte(origem, destino) == 2);
  igual(destino, "44444444.jpg", "retry-link");
  igual(destino, "55555555.jpg", "retry-dir");
  assert(nv_cache_migrar_arte(destino, destino) == 0);

  // Alias de diretorio e destino invalido nunca movem ou apagam dados.
  caminho(alias, argv[1], "link-cache"); assert(!symlink(destino, alias));
  assert(nv_cache_migrar_arte(origem, alias) == -1);
  assert(nv_cache_migrar_arte(alias, destino) == 0);
  caminho(p, conta, "ajustes.txt"); assert(nv_cache_migrar_arte(origem, p) == -1);
  assert(nv_cache_migrar_arte(origem, "") == -1);

  // A poda continua funcionando na nova pasta e respeita imagens em uso.
  caminho(p, destino, "a1b2c3d4.jpg");
  assert(nv_cache_podar(destino, &total, 0, 0, 0, 1, protegido, p) == 4);
  igual(destino, "a1b2c3d4.jpg", "image");
  assert(total == 5);
  igual(destino, ".limpo-143", "1");

  // Android pode remover toda a cache. A proxima abertura recria-a sem contas.
  apagar(destino, "a1b2c3d4.jpg");
  apagar(destino, ".limpo-143"); apagar(destino, ".arte-negativa");
  assert(!rmdir(destino));
  assert(nv_cache_migrar_arte(origem, destino) == 0);
  assert(!stat(destino, &st) && S_ISDIR(st.st_mode));
  igual(conta, "ajustes.txt", "subtitle-language=eng");
  igual(conta, "provedor.txt", "account-secret");
  igual(origem, "ajustes.txt", "do-not-delete");
  puts("PASS: Android artwork migration, cache clearing, safe retry and pruning");
  return 0;
}
