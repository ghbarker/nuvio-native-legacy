// Provider XMLTV: bounded XML window, independent of expanded gzip size.
#define XMLTV_JANELA (256 * 1024)
#define XMLTV_PEDACO (64 * 1024)

typedef struct {
  char *buf;
  size_t n;
  const char *pular;
  int raiz, fim, grandes, perdas, programas;
  EpgGrade *grade;
  char (*filtro)[64];
  int nFiltro;
} XmltvFluxo;

static int xmltvTag(const char *p, const char *nome) {
  size_t n = strlen(nome);
  return !strncmp(p, nome, n) &&
    (isspace((unsigned char)p[n]) || p[n] == '>' || p[n] == '/');
}

static char *xmltvFimTag(char *p) {
  char aspas = 0;
  for (; *p; p++) {
    if (aspas) { if (*p == aspas) aspas = 0; }
    else if (*p == '\'' || *p == '"') aspas = *p;
    else if (*p == '>') return p;
  }
  return NULL;
}

// Drain complete records. A large description cannot grow this window: skip
// that record and continue at its closing tag, preserving subsequent channels.
static int xmltvDrenar(XmltvFluxo *x, int final) {
  char *p = x->buf, *fim = p + x->n;
  while (p < fim) {
    if (x->pular) {
      char *e = strstr(p, x->pular);
      if (!e) {
        size_t guardar = strlen(x->pular) - 1;
        if ((size_t)(fim - p) > guardar) p = fim - guardar;
        break;
      }
      p = e + strlen(x->pular); x->pular = NULL;
      continue;
    }
    char *a = strchr(p, '<');
    if (!a) { p = fim; break; }
    p = a;
    // Retain a possible split tag name, comment or CDATA opening.
    if (!final && fim - p < 12) break;
    if (!strncmp(p, "<!--", 4) || !strncmp(p, "<![CDATA[", 9) || !strncmp(p, "<?", 2)) {
      x->pular = !strncmp(p, "<!--", 4) ? "-->" :
                  !strncmp(p, "<?", 2) ? "?>" : "]]>";
      p += !strncmp(p, "<!--", 4) ? 4 : !strncmp(p, "<?", 2) ? 2 : 9;
      continue;
    }
    if (xmltvTag(p, "<tv")) {
      char *e = xmltvFimTag(p);
      if (!e) break;
      if (x->raiz || x->fim) return 0;
      x->raiz = 1; x->fim = e > p && e[-1] == '/'; p = e + 1;
      continue;
    }
    if (xmltvTag(p, "</tv")) {
      char *e = xmltvFimTag(p);
      if (!e) break;
      if (!x->raiz || x->fim) return 0;
      x->fim = 1; p = e + 1;
      continue;
    }
    const char *fecho = xmltvTag(p, "<channel") ? "</channel>" :
                         xmltvTag(p, "<programme") ? "</programme>" : NULL;
    if (fecho && x->raiz && !x->fim) {
      char *e = strstr(p, fecho);
      if (!e) {
        if ((size_t)(fim - p) == XMLTV_JANELA) {
          x->grandes++; x->pular = fecho;
          p += strlen(fecho); // The opening cannot also be the closing tag.
          continue;
        }
        break;
      }
      e += strlen(fecho);
      if (x->grade) {
        char salvo = *e; *e = 0;
        // The existing parser searches for a space after the element name.
        p[strlen(fecho) - 2] = ' ';
        x->programas += wProcessarF(x->grade, p, x->filtro, x->nFiltro, &x->perdas);
        *e = salvo;
      }
      p = e;
      continue;
    }
    char *e = xmltvFimTag(p);
    if (!e) break;
    p = e + 1;
  }
  x->n = (size_t)(fim - p);
  memmove(x->buf, p, x->n); x->buf[x->n] = 0;
  return x->n < XMLTV_JANELA;
}

static int xmltvAdicionar(XmltvFluxo *x, const char *p, size_t n) {
  while (n) {
    size_t k = XMLTV_JANELA - x->n;
    if (k > n) k = n;
    memcpy(x->buf + x->n, p, k); x->n += k; x->buf[x->n] = 0;
    p += k; n -= k;
    if (!xmltvDrenar(x, 0)) return 0;
  }
  return 1;
}

// NULL grade validates without publishing partial results from corrupt gzip.
// Network and cache keep the original compressed bytes; both use this reader.
static int xmltvExtra(const char *body, long n, EpgGrade *grade,
                      char (*filtro)[64], int nFiltro, int *programas, const char **erro) {
  XmltvFluxo x = {0};
  z_stream z = {0};
  int gzip = body && n >= 2 && (unsigned char)body[0] == 0x1f && (unsigned char)body[1] == 0x8b;
  int iniciou = 0, ok = 0;
  long total = 0;
  const char *falha = "XMLTV sem raiz ou fechamento tv";
  if (programas) *programas = 0;
  if (erro) *erro = NULL;
  if (!body || n <= 0) goto fim;
  x.grade = grade; x.filtro = filtro; x.nFiltro = nFiltro;
  x.buf = malloc(XMLTV_JANELA + 1);
  if (!x.buf) { falha = "sem memoria para janela XMLTV"; goto fim; }
  x.buf[0] = 0;
  if (gzip) {
    if (inflateInit2(&z, 31) != Z_OK) { falha = "gzip invalido"; goto fim; }
    iniciou = 1; z.next_in = (Bytef *)body; z.avail_in = (uInt)n;
  }
  for (long pos = 0;;) {
    char pedaco[XMLTV_PEDACO];
    const char *p;
    size_t k;
    int terminou = 0;
    if (!cargaAtual()) { falha = "carga cancelada"; goto fim; }
    if (gzip) {
      z.next_out = (Bytef *)pedaco; z.avail_out = sizeof pedaco;
      int r = inflate(&z, Z_NO_FLUSH);
      if (r != Z_OK && r != Z_STREAM_END) { falha = "gzip invalido ou incompleto"; goto fim; }
      k = sizeof pedaco - z.avail_out; p = pedaco; terminou = r == Z_STREAM_END;
    } else {
      k = (size_t)(n - pos); if (k > XMLTV_PEDACO) k = XMLTV_PEDACO;
      p = body + pos; pos += (long)k; terminou = pos == n;
    }
    total += (long)k;
    if (total > EPG_EXTRA_TETO_FLUXO) { falha = "XMLTV acima do limite de processamento"; goto fim; }
    if (memchr(p, 0, k)) { falha = "XMLTV com bytes NUL (codificacao nao suportada)"; goto fim; }
    if (!xmltvAdicionar(&x, p, k)) { falha = "tag XMLTV acima da janela"; goto fim; }
    if (terminou) break;
  }
  if (!xmltvDrenar(&x, 1)) goto fim;
  ok = x.raiz && x.fim && !x.pular;
  if (ok && grade) {
    if (programas) *programas = x.programas;
    printf("[epg] XMLTV em fluxo: %ld KB expandidos, %d programas, %d registros grandes ignorados, %d descartados por memoria\n",
           total / 1024, x.programas, x.grandes, x.perdas);
    fflush(stdout);
  }
fim:
  if (iniciou) inflateEnd(&z);
  free(x.buf);
  if (!ok && erro) *erro = falha;
  return ok;
}
