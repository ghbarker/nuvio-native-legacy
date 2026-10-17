#include "stub_perfisessao.h"
#include "../src/epg.c"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *pasta;
static const char *resposta;
static long tamanho;
static int pedidos;

char *dados_caminho(char *out, unsigned cap, const char *nome) {
  int n = snprintf(out, cap, "%s/%s", pasta, nome);
  return n >= 0 && (unsigned)n < cap ? out : NULL;
}
char *dados_ler(const char *nome) {
  char cam[1024]; assert(dados_caminho(cam, sizeof cam, nome));
  FILE *f = fopen(cam, "rb"); if (!f) return NULL;
  assert(!fseek(f, 0, SEEK_END)); long n = ftell(f); assert(n >= 0); rewind(f);
  char *s = malloc((size_t)n + 1); assert(s);
  assert(fread(s, 1, (size_t)n, f) == (size_t)n); s[n] = 0; fclose(f); return s;
}
int dados_gravar_leve(const char *nome, const char *s) {
  char cam[1024]; assert(dados_caminho(cam, sizeof cam, nome));
  FILE *f = fopen(cam, "wb"); assert(f);
  assert(fwrite(s, 1, strlen(s), f) == strlen(s)); return fclose(f) == 0;
}
int dados_apagar(const char *nome) {
  char cam[1024]; assert(dados_caminho(cam, sizeof cam, nome)); return unlink(cam) == 0;
}
void dados_fs_travar(void) {}
void dados_fs_liberar(void) {}
void dados_marcar_sujo(int leve) { (void)leve; }
char *rede_baixar_bin(const char *u, int s, long *n) { (void)u; (void)s; (void)n; return NULL; }
char *rede_baixar_bin_medido_controle(const char *u, int s, const char *const *h,
                                    const RedeControle *c, long *n, RedeMedida *m) {
  (void)u; (void)h; assert(s == 60); assert(c->max_bytes > 0); pedidos++;
  memset(m, 0, sizeof *m); m->status = resposta ? 200 : 503;
  if (!resposta) return NULL;
  char *b = malloc((size_t)tamanho); assert(b);
  memcpy(b, resposta, (size_t)tamanho); *n = tamanho; return b;
}

static const char *cab = "<?xml version=\"1.0\"?><tv><channel\n id=\"news\"><display-name>News HD</display-name></channel>";
static const char *prog = "<programme\n channel=\"news\" start=\"20990101000000 +0000\" stop=\"20990101010000 +0000\"><title>News &amp; weather</title></programme></tv>";

static void comprimir(z_stream *z, const char *s, size_t n) {
  z->next_in = (Bytef *)s; z->avail_in = (uInt)n;
  while (z->avail_in) assert(deflate(z, Z_NO_FLUSH) == Z_OK && z->avail_out);
}
static char *grande(long *n) {
  // Generate 160 MiB without allocating the expanded fixture. A long XML
  // comment crosses every input boundary, then a real programme must survive.
  const size_t cap = 2 * 1024 * 1024;
  char *out = malloc(cap), bloco[65536]; assert(out); memset(bloco, 'x', sizeof bloco);
  z_stream z = {0}; assert(deflateInit2(&z, 1, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) == Z_OK);
  z.next_out = (Bytef *)out; z.avail_out = (uInt)cap;
  comprimir(&z, cab, strlen(cab)); comprimir(&z, "<!--", 4);
  for (int i = 0; i < 2560; i++) comprimir(&z, bloco, sizeof bloco);
  comprimir(&z, "-->", 3); comprimir(&z, prog, strlen(prog));
  assert(deflate(&z, Z_FINISH) == Z_STREAM_END); *n = (long)z.total_out;
  deflateEnd(&z); return out;
}
static void conferir(char *body, long n) {
  const char *erro = NULL; int count = -1;
  assert(xmltvExtra(body, n, NULL, NULL, 0, NULL, &erro) && !erro);
  assert(wAbrir(&W));
  char filtro[1][64] = {"news"};
  assert(xmltvExtra(body, n, &W, filtro, 1, &count, NULL) && count == 1);
  wFechar(&W); assert(W.nCanais == 1 && W.nEvs == 1);
  assert(!strcmp(W.arena + W.evs[0].tit, "News & weather"));
  free(W.evs); free(W.arena); memset(&W, 0, sizeof W);
}
int main(int argc, char **argv) {
  assert(argc == 2); pasta = argv[1]; assert(!mkdir(pasta, 0700)); carga.perfil = 1;
  char *body = grande(&tamanho); conferir(body, tamanho);
  puts("PASS: gzip above the previous 128 MiB expanded limit, split comments/tags and filters");
  const char *erro = NULL;
  assert(!xmltvExtra(body, tamanho - 6, NULL, NULL, 0, NULL, &erro));
  assert(erro && strstr(erro, "gzip"));
  assert(!xmltvExtra("#EXTM3U\n", 8, NULL, NULL, 0, NULL, &erro));
  assert(!xmltvExtra("<html>Login</html>", 18, NULL, NULL, 0, NULL, &erro));
  assert(xmltvExtra("<tv/>", 5, NULL, NULL, 0, NULL, &erro));
  assert(!xmltvExtra("<tv>", 4, NULL, NULL, 0, NULL, &erro));
  char simples[1024]; snprintf(simples, sizeof simples, "%s%s", cab, prog);
  conferir(simples, (long)strlen(simples));
  // Split element opening/closing at every possible byte boundary.
  for (size_t split = 1; split < strlen(simples); split++) {
    XmltvFluxo x = {0}; x.buf = calloc(XMLTV_JANELA + 1, 1); assert(x.buf);
    assert(xmltvAdicionar(&x, simples, split));
    assert(xmltvAdicionar(&x, simples + split, strlen(simples) - split));
    assert(xmltvDrenar(&x, 1) && x.raiz && x.fim && !x.pular); free(x.buf);
  }
  puts("PASS: plain XML, empty XMLTV, malformed/truncated responses and every split boundary");
  // Keep a single oversized record bounded, then recover the next programme.
  size_t grandeN = strlen(cab) + XMLTV_JANELA * 2 + strlen(prog) + 128;
  char *registro = malloc(grandeN); assert(registro);
  size_t off = (size_t)sprintf(registro, "%s<programme channel=\"news\"><desc>", cab);
  memset(registro + off, 'x', XMLTV_JANELA * 2); off += XMLTV_JANELA * 2;
  off += (size_t)sprintf(registro + off, "</desc></programme>%s", prog);
  conferir(registro, (long)off); free(registro);

  resposta = body;
  long n = 0; char *cache = obterExtraNome("https://guide.invalid/one", &n, 1);
  assert(cache && n == tamanho && pedidos == 1); free(cache);
  // Simulate a cold restart: no published grade; only the on-disk gzip remains.
  memset(&P, 0, sizeof P); resposta = NULL;
  cache = obterExtraNome("https://guide.invalid/one", &n, 1);
  assert(cache && pedidos == 1); conferir(cache, n); free(cache);
  assert(dados_apagar("epg-extra-m3u-p1.ts"));
  // HTTP 200 with a bad body also falls back, without overwriting valid gzip.
  resposta = "<html>Login</html>"; long antigo = tamanho; tamanho = (long)strlen(resposta);
  cache = obterExtraNome("https://guide.invalid/one", &n, 1);
  assert(cache && n == antigo && pedidos == 2); free(cache);
  cache = obterExtraNome("https://guide.invalid/different", &n, 1);
  assert(!cache && pedidos == 3); // A different URL must not borrow the old cache.
  carga.perfil = 2; resposta = NULL;
  cache = obterExtraNome("https://guide.invalid/one", &n, 1);
  assert(!cache && pedidos == 4); // Nor may a different profile.
  carga.perfil = 1; carga.cancelada = 1;
  assert(!xmltvExtra(body, antigo, NULL, NULL, 0, NULL, &erro));
  assert(erro && strstr(erro, "cancelada"));
  free(body);
  puts("PASS: cold restart cache, invalid-refresh recovery, URL/profile ownership and cancellation");
  return 0;
}
