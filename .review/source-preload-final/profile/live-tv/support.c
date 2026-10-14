#include "support.h"
#include "dados.h"
#include <dirent.h>
static char dir[256];
void fixture_init(void) {
  snprintf(dir, sizeof dir, "/tmp/nuvio-live-tv-ownership.XXXXXX");
  assert(mkdtemp(dir));
  perfis_definir_ativo(1);
}
const char *fixture_dir(void) { return dir; }
const char *dados_dir(void) { return dir; }
char *dados_caminho(char *out, unsigned tam, const char *nome) {
  int n = snprintf(out, tam, "%s/%s", dir, nome);
  return n >= 0 && (unsigned)n < tam ? out : NULL;
}
char *dados_ler(const char *name) {
  char path[600], *out; long n;
  assert(dados_caminho(path, sizeof path, name));
  FILE *f = fopen(path, "rb"); if (!f) return NULL;
  assert(!fseek(f, 0, SEEK_END)); n = ftell(f); assert(n >= 0);
  rewind(f); out = malloc((size_t)n + 1); assert(out);
  assert(fread(out, 1, (size_t)n, f) == (size_t)n); out[n] = 0;
  fclose(f); return out;
}
int dados_gravar(const char *name, const char *text) {
  char path[600]; assert(dados_caminho(path, sizeof path, name));
  FILE *f = fopen(path, "wb"); assert(f);
  assert(fwrite(text, 1, strlen(text), f) == strlen(text));
  assert(!fclose(f)); return 1;
}
int dados_gravar_leve(const char *name, const char *text) { return dados_gravar(name, text); }
int dados_apagar(const char *name) {
  char path[600]; assert(dados_caminho(path, sizeof path, name)); return unlink(path) == 0;
}
void fixture_cleanup(void) {
  DIR *d = opendir(dir); struct dirent *e; assert(d);
  while ((e = readdir(d))) if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) dados_apagar(e->d_name);
  closedir(d); assert(!rmdir(dir));
}
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
static pthread_mutex_t fs = PTHREAD_MUTEX_INITIALIZER;
void dados_fs_travar(void) { pthread_mutex_lock(&fs); }
void dados_fs_liberar(void) { pthread_mutex_unlock(&fs); }
void dados_marcar_sujo(int leve) { (void)leve; }
