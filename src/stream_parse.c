#include "streams.h"
#include "badges.h"
#include "js.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <math.h>

// Addons tambem mandam numeros como strings. Evita conversao indefinida de
// infinito ou expoentes enormes a int/long, preservando tamanho desconhecido.
static long tamanhoMB(double valor) {
  return isfinite(valor) && valor > 0 && valor < (double)LONG_MAX ? (long)valor : 0;
}

// Find only a direct property of this exact object. JSON strings and nested
// objects are skipped; proxyHeaders/request cannot masquerade as videoSize.
static const char *valorRaiz(const char *p, const char *fim, const char *nome) {
  int depth = 0;
  size_t n = strlen(nome);
  if (!p || p >= fim || *p != '{') return NULL;
  while (p < fim) {
    if (*p == '"') {
      const char *ini = ++p;
      while (p < fim && *p != '"') {
        if (*p == '\\' && p + 1 < fim) p++;
        p++;
      }
      if (p >= fim) return NULL;
      const char *apos = p + 1;
      while (apos < fim && isspace((unsigned char)*apos)) apos++;
      if (depth == 1 && (size_t)(p - ini) == n && !memcmp(ini, nome, n) && apos < fim && *apos == ':') {
        apos++; while (apos < fim && isspace((unsigned char)*apos)) apos++;
        return apos < fim ? apos : NULL;
      }
    } else if (*p == '{' || *p == '[') depth++;
    else if (*p == '}' || *p == ']') { if (--depth == 0) break; }
    p++;
  }
  return NULL;
}

static const char *objetoRaiz(const char *p, const char *fim, const char *nome) {
  const char *v = valorRaiz(p, fim, nome), *f;
  if (!v || *v != '{' || !(f = js_fim(v)) || f <= v || f > fim || f[-1] != '}') return NULL;
  return v;
}

// Idiomas de faixas sao metadados positivos do addon.
// Valida cada valor antes de usa-lo: JSON cortado, objetos muito
// fundos e chaves em cabecalhos nunca viram disponibilidade de audio/legenda.
static const char *faixasEspacos(const char *p, const char *fim) {
  while (p < fim && isspace((unsigned char)*p)) p++;
  return p;
}
static unsigned faixasHex4(const char *p) {
  unsigned n = 0;
  for (int k = 0; k < 4; k++) {
    unsigned char c = (unsigned char)p[k];
    if (!isxdigit(c)) return UINT_MAX;
    n = n * 16 + (c <= '9' ? (unsigned)(c - '0') : (unsigned)(tolower(c) - 'a' + 10));
  }
  return n;
}
static const char *faixasFimTexto(const char *p, const char *fim) {
  if (p >= fim || *p++ != '"') return NULL;
  while (p < fim) {
    unsigned char c = (unsigned char)*p++;
    if (c == '"') return p;
    if (c < 0x20) return NULL;
    if (c != '\\') continue;
    if (p >= fim) return NULL;
    c = (unsigned char)*p++;
    if (strchr("\"\\/bfnrt", c)) continue;
    if (c != 'u' || fim - p < 4) return NULL;
    unsigned cp = faixasHex4(p);
    if (cp == UINT_MAX) return NULL;
    p += 4;
    if (cp >= 0xDC00 && cp <= 0xDFFF) return NULL;
    if (cp >= 0xD800 && cp <= 0xDBFF) {
      if (fim - p < 6 || p[0] != '\\' || p[1] != 'u') return NULL;
      unsigned lo = faixasHex4(p + 2);
      if (lo < 0xDC00 || lo > 0xDFFF) return NULL;
      p += 6;
    }
  }
  return NULL;
}
static const char *faixasFimValor(const char *p, const char *fim, unsigned prof) {
  if (p >= fim || prof > 16) return NULL;
  if (*p == '"') return faixasFimTexto(p, fim);
  if (*p == '{' || *p == '[') {
    int objeto = *p == '{';
    char fecha = objeto ? '}' : ']';
    p = faixasEspacos(p + 1, fim);
    if (p < fim && *p == fecha) return p + 1;
    for (;;) {
      if (objeto) {
        p = faixasFimTexto(p, fim);
        if (!p) return NULL;
        p = faixasEspacos(p, fim);
        if (p >= fim || *p != ':') return NULL;
        p = faixasEspacos(p + 1, fim);
      }
      p = faixasFimValor(p, fim, prof + 1);
      if (!p) return NULL;
      p = faixasEspacos(p, fim);
      if (p >= fim) return NULL;
      if (*p == fecha) return p + 1;
      if (*p != ',') return NULL;
      p = faixasEspacos(p + 1, fim);
    }
  }
  static const char *const literais[] = {"null", "true", "false"};
  for (unsigned k = 0; k < sizeof literais / sizeof *literais; k++) {
    size_t n = strlen(literais[k]);
    if ((size_t)(fim - p) >= n && !memcmp(p, literais[k], n)) return p + n;
  }
  // Numeros sao ignorados como idiomas, mas precisam de sintaxe JSON valida.
  if (*p == '-') p++;
  if (p >= fim || !isdigit((unsigned char)*p)) return NULL;
  if (*p == '0') p++;
  else while (p < fim && isdigit((unsigned char)*p)) p++;
  if (p < fim && *p == '.') {
    p++;
    if (p >= fim || !isdigit((unsigned char)*p)) return NULL;
    while (p < fim && isdigit((unsigned char)*p)) p++;
  }
  if (p < fim && (*p == 'e' || *p == 'E')) {
    p++;
    if (p < fim && (*p == '+' || *p == '-')) p++;
    if (p >= fim || !isdigit((unsigned char)*p)) return NULL;
    while (p < fim && isdigit((unsigned char)*p)) p++;
  }
  return p;
}
static int faixasUtf8(const unsigned char *p) {
  while (*p) {
    unsigned c = *p++, cp, n;
    if (c < 0x80) continue;
    if (c >= 0xC2 && c <= 0xDF) { cp = c & 31; n = 1; }
    else if (c >= 0xE0 && c <= 0xEF) { cp = c & 15; n = 2; }
    else if (c >= 0xF0 && c <= 0xF4) { cp = c & 7; n = 3; }
    else return 0;
    unsigned minimo = n == 1 ? 0x80 : n == 2 ? 0x800 : 0x10000;
    for (unsigned k = 0; k < n; k++) {
      if ((*p & 0xC0) != 0x80) return 0;
      cp = (cp << 6) | (*p++ & 63);
    }
    if (cp < minimo || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) ||
        (cp >= 0x80 && cp <= 0x9F) || cp == 0x2028 || cp == 0x2029) return 0;
  }
  return 1;
}
static int faixasTexto(const char *p, const char *fim, char *dst, unsigned tam) {
  const char *f = faixasFimTexto(p, fim);
  char json[1024];
  if (!f || !tam || (size_t)(f - p) >= sizeof json || (size_t)(f - p - 2) >= tam) return 0;
  // O decodificador geral converte alguns controles em espacos. Aqui nao
  // aceitar "en\\u0000..." nem valores com quebras que parecam cabecalhos.
  for (const char *q = p + 1; q < f - 1; q++) if (*q == '\\') {
    q++;
    if (strchr("bfnrt", *q)) return 0;
    if (*q == 'u') {
      unsigned cp = faixasHex4(q + 1);
      if (cp < 0x20 || cp == 0x7F) return 0;
      q += 4;
    }
  }
  memcpy(json, p, (size_t)(f - p)); json[f - p] = 0;
  return js_cadeia(json, dst, tam) && faixasUtf8((const unsigned char *)dst);
}
static void faixaIdiomaAdicionar(char *dst, unsigned tam, const char *ini, const char *fim) {
  char idioma[128];
  while (ini < fim && isspace((unsigned char)*ini)) ini++;
  while (fim > ini && isspace((unsigned char)fim[-1])) fim--;
  size_t n = (size_t)(fim - ini);
  if (!n || n >= sizeof idioma) return;
  int letra = 0;
  for (const char *p = ini; p < fim; p++) {
    unsigned char c = (unsigned char)*p;
    if (c >= 0x80 || isalpha(c)) letra = 1;
    else if (!isdigit(c) && c != ' ' && c != '-' && c != '_' && c != '(' && c != ')' && c != '\'' && c != '.') return;
  }
  if (!letra) return;
  memcpy(idioma, ini, n); idioma[n] = 0;
  static const char *const vazios[] = {
    "und", "unknown", "null", "none", "undefined", "unavailable", "unreported",
    "not reported", "not specified", "n.a.", "zxx", "mul", "multi", "multilingual",
    "multiple", "multiple languages", "auto", "original"
  };
  for (unsigned k = 0; k < sizeof vazios / sizeof *vazios; k++)
    if (!strcasecmp(idioma, vazios[k])) return;
  for (const char *p = dst; *p;) {
    const char *e = strchr(p, ',');
    if (!e) e = p + strlen(p);
    if ((size_t)(e - p) == n && !strncasecmp(p, idioma, n)) return;
    p = *e ? e + 1 : e;
  }
  size_t usado = strlen(dst);
  if (usado + n + (usado ? 1 : 0) >= tam) return;
  if (usado) dst[usado++] = ',';
  memcpy(dst + usado, idioma, n + 1);
}
static void faixasValorLer(const char *v, const char *fim, char *dst, unsigned tam, unsigned prof, int faixa) {
  if (!v || v >= fim || prof > 3) return;
  if (*v == '"') {
    char texto[1024];
    if (!faixasTexto(v, fim, texto, sizeof texto)) return;
    const char *p = texto;
    for (const char *q = p;; q++) if (!*q || *q == ',' || *q == '|' || *q == ';') {
      faixaIdiomaAdicionar(dst, tam, p, q);
      if (!*q) break;
      p = q + 1;
    }
  } else if (*v == '[') {
    const char *q = faixasEspacos(v + 1, fim);
    while (q < fim && *q != ']') {
      const char *f = faixasFimValor(q, fim, 0);
      if (!f) return;
      // Lista aninhada nao tem significado de faixa: nao a achatar.
      if (*q == '"' || *q == '{') faixasValorLer(q, f, dst, tam, prof + 1, faixa);
      q = faixasEspacos(f, fim);
      if (q < fim && *q == ',') q = faixasEspacos(q + 1, fim);
      else break;
    }
  } else if (*v == '{') {
    // language/lang pertencem a faixa; code/name sao aceitos apenas dentro
    // de um objeto declarado como idioma. O nome/codec de uma faixa nao e
    // seu idioma: {name:"Stereo",code:"AAC"} nao informa idioma nenhum.
    static const char *const chaves[] = {"language", "lang", "languageCode", "language_code", "code", "iso639_1", "iso639_2", "name"};
    unsigned n = faixa ? 4 : sizeof chaves / sizeof *chaves;
    for (unsigned k = 0; k < n; k++) {
      const char *p = valorRaiz(v, fim, chaves[k]);
      if (p && (*p == '"' || *p == '{')) {
        faixasValorLer(p, fim, dst, tam, prof + 1, 0);
        return;
      }
    }
  }
}
static void faixasCampo(const char *obj, const char *fim, const char *chave, char *dst, unsigned tam, int faixa) {
  const char *v = valorRaiz(obj, fim, chave), *f;
  if (!v || !(f = faixasFimValor(v, fim, 0))) return;
  const char *apos = faixasEspacos(f, fim);
  if (apos >= fim || (*apos != ',' && *apos != '}')) return;
  faixasValorLer(v, f, dst, tam, 0, faixa);
}
static void metadadosFaixas(const char *obj, const char *fim, Stream *s) {
  const char *cr = objetoRaiz(obj, fim, "clientResolve");
  const char *st = cr ? objetoRaiz(cr, js_fim(cr), "stream") : NULL;
  const char *raw = st ? objetoRaiz(st, js_fim(st), "raw") : NULL;
  const char *parsed = raw ? objetoRaiz(raw, js_fim(raw), "parsed") : NULL;
  if (!parsed && st) parsed = objetoRaiz(st, js_fim(st), "parsed");
  if (!parsed && cr) parsed = objetoRaiz(cr, js_fim(cr), "parsed");
  if (!raw) raw = objetoRaiz(obj, fim, "raw");
  if (!parsed && raw) parsed = objetoRaiz(raw, js_fim(raw), "parsed");
  const char *parsedRaiz = objetoRaiz(obj, fim, "parsed");
  const char *data = objetoRaiz(obj, fim, "streamData");
  const char *parsedFile = data ? objetoRaiz(data, js_fim(data), "parsedFile") : NULL;
  const char *objetos[] = {parsedFile, parsed, parsedRaiz, raw, st, cr, obj};
  static const char *const audio[] = {"audioLanguages", "audio_languages", "audioTracks", "audio_tracks"};
  static const char *const subs[] = {"subtitleLanguages", "subtitle_languages", "subtitleTracks", "subtitle_tracks"};
  for (unsigned k = 0; k < sizeof objetos / sizeof *objetos; k++) if (objetos[k]) {
    const char *f = js_fim(objetos[k]);
    for (unsigned j = 0; j < sizeof audio / sizeof *audio; j++)
      faixasCampo(objetos[k], f, audio[j], s->audioIdiomas, sizeof s->audioIdiomas, j >= 2);
    for (unsigned j = 0; j < sizeof subs / sizeof *subs; j++)
      faixasCampo(objetos[k], f, subs[j], s->legendaIdiomas, sizeof s->legendaIdiomas, j >= 2);
    const char *tracks = objetoRaiz(objetos[k], f, "tracks");
    if (tracks) {
      const char *tf = js_fim(tracks);
      faixasCampo(tracks, tf, "audio", s->audioIdiomas, sizeof s->audioIdiomas, 1);
      faixasCampo(tracks, tf, "subtitles", s->legendaIdiomas, sizeof s->legendaIdiomas, 1);
      faixasCampo(tracks, tf, "subtitle", s->legendaIdiomas, sizeof s->legendaIdiomas, 1);
    }
  }
  // AIOStreams separa explicitamente languages (audio) de subtitles no
  // ParsedFile. So estes caminhos documentados autorizam o nome generico.
  // https://github.com/Viren070/AIOStreams/blob/main/packages/core/src/db/schemas.ts
  if (parsedFile) {
    faixasCampo(parsedFile, js_fim(parsedFile), "languages", s->audioIdiomas, sizeof s->audioIdiomas, 0);
    faixasCampo(parsedFile, js_fim(parsedFile), "subtitles", s->legendaIdiomas, sizeof s->legendaIdiomas, 0);
  }
  if (parsed) {
    faixasCampo(parsed, js_fim(parsed), "languages", s->audioIdiomas, sizeof s->audioIdiomas, 0);
    faixasCampo(parsed, js_fim(parsed), "subtitles", s->legendaIdiomas, sizeof s->legendaIdiomas, 0);
  }
  // Stremio stream.subtitles sao legendas anexadas a ESTA fonte. Nao buscar
  // no objeto de resposta/titulo, nem afirmar que estas faixas sao embutidas.
  faixasCampo(obj, fim, "subtitles", s->legendaIdiomas, sizeof s->legendaIdiomas, 1);
}

// Decimal integers only, parsed without double rounding. Numeric strings
// are accepted for compatibility, but fractions/exponents/overflow are
// unavailable for fit. Legacy display size remains separate below.
static uint64_t bytesExatos(const char *obj, const char *fim) {
  const char *bh = valorRaiz(obj, fim, "behaviorHints"), *p, *bf;
  uint64_t valor = 0;
  int quoted;
  if (!bh || *bh != '{' || !(bf = js_fim(bh)) || bf > fim) return 0;
  p = valorRaiz(bh, bf, "videoSize");
  if (!p) return 0;
  quoted = *p == '"'; if (quoted) p++;
  if (p >= bf || !isdigit((unsigned char)*p)) return 0;
  if (!quoted && *p == '0' && p + 1 < bf && isdigit((unsigned char)p[1])) return 0;
  while (p < bf && isdigit((unsigned char)*p)) {
    unsigned d = (unsigned)(*p++ - '0');
    if (valor > (STREAMFIT_BYTES_MAX - d) / 10) return 0;
    valor = valor * 10 + d;
  }
  if (quoted) { if (p >= bf || *p++ != '"') return 0; }
  while (p < bf && isspace((unsigned char)*p)) p++;
  return p < bf && (*p == ',' || *p == '}') ? valor : 0;
}

static int contem(const char *s, const char *termo) {
  for (; *s; s++) if (!strncasecmp(s, termo, strlen(termo))) return 1;
  return 0;
}

// "FHD"/"Full HD" como ROTULO SOLTO do formatador (#402): AIOStreams e afins
// escrevem "FHD | REMUX | SDR" ou "FHD \u2022 REMUX". Aceita so se:
//  - os dois lados sao inicio/fim de linha, espaco, '|', '\u2022' ou '\u00b7'
//    (nunca '.', '_', '-', '/', '[' — isso e nome de arquivo ou URL);
//  - e o primeiro rotulo da linha, ou encosta num '|' / bullet (pulando
//    espaco). "The FHD Story" e frase, nao rotulo;
//  - o texto nao tem "://".
// Nao o "hd" solto do nv_res_do_texto ("DTS-HD" viraria 720).
static int ehSepFhd(const char *a, const char *ini, int antes) {
  const unsigned char *u = (const unsigned char *)a;
  if (antes) {
    if (a == ini || u[-1] == '\n' || u[-1] == ' ' || u[-1] == '\t' || u[-1] == '|') return 1;
    if (a - ini >= 3 && u[-3] == 0xE2 && u[-2] == 0x80 && u[-1] == 0xA2) return 1;
    return a - ini >= 2 && u[-2] == 0xC2 && u[-1] == 0xB7;
  }
  return !*u || *u == '\n' || *u == ' ' || *u == '\t' || *u == '|' ||
         (u[0] == 0xE2 && u[1] == 0x80 && u[2] == 0xA2) || (u[0] == 0xC2 && u[1] == 0xB7);
}
// Prefixo da linha feito so de SIMBOLO/emoji e espaco, e curto (ate 2 simbolos):
// "\u23f3 FHD", "\u26a1 FHD". So os blocos de simbolo do UTF-8 contam — E2 80..AF xx
// (U+2000..U+2BFF: setas, relogios, raios, estrelas; de U+2C00 em diante ha letras,
// Glagolitico e Georgiano; U+2E00..U+2FFF tem pontuacao e radicais, recusados por
// seguranca) e F0 9F xx xx (emoji), com EF B8 8F
// (seletor de variacao) junto. Letra de outro alfabeto (Cirilico D0/D1, CJK
// E3..E9) e palavra, nao rotulo: "Фильм FHD" e "我的 FHD" continuam frase.
static int prefixoSoSimbolos(const char *ini, const char *fim) {
  const unsigned char *u = (const unsigned char *)ini, *f = (const unsigned char *)fim;
  int simbolos = 0;
  while (u < f) {
    if (*u == ' ' || *u == '\t') { u++; continue; }
    if (f - u >= 3 && u[0] == 0xEF && u[1] == 0xB8 && u[2] == 0x8F) { u += 3; continue; }
    if (f - u >= 3 && u[0] == 0xE2 && u[1] >= 0x80 && u[1] <= 0xAF && (u[2] & 0xC0) == 0x80) u += 3;
    else if (f - u >= 4 && u[0] == 0xF0 && u[1] == 0x9F && (u[2] & 0xC0) == 0x80 &&
             (u[3] & 0xC0) == 0x80) u += 4;
    else return 0;
    if (++simbolos > 2) return 0;
  }
  return simbolos > 0;
}
static int ehRotuloSep(const unsigned char *u) {   // '|' ou bullet comecando em u
  return *u == '|' || (u[0] == 0xE2 && u[1] == 0x80 && u[2] == 0xA2) ||
         (u[0] == 0xC2 && u[1] == 0xB7);
}
static int fhdNoNome(const char *s) {
  static const char *const t[] = {"fhd", "fullhd", "full hd", "full-hd"};
  if (strstr(s, "://")) return 0;
  for (const char *p = s; *p; p++)
    for (size_t i = 0; i < sizeof t / sizeof t[0]; i++) {
      size_t n = strlen(t[i]);
      if (strncasecmp(p, t[i], n) || !ehSepFhd(p, s, 1) || !ehSepFhd(p + n, s, 0)) continue;
      const char *a = p, *d = p + n;
      while (a > s && (a[-1] == ' ' || a[-1] == '\t')) a--;
      while (*d == ' ' || *d == '\t') d++;
      if (a == s || a[-1] == '\n' || ehRotuloSep((const unsigned char *)d)) return 1;
      if (a[-1] == '|' ||
          (a - s >= 3 && (unsigned char)a[-1] == 0xA2 && (unsigned char)a[-2] == 0x80 &&
           (unsigned char)a[-3] == 0xE2) ||
          (a - s >= 2 && (unsigned char)a[-1] == 0xB7 && (unsigned char)a[-2] == 0xC2)) return 1;
      // So simbolos (emoji, bytes >= 0x80) e espaco antes na linha: "\u23f3 FHD" do
      // AIOStreams e rotulo; uma palavra ASCII antes ("The FHD") continua frase.
      { const char *ls = a;
        while (ls > s && ls[-1] != '\n') ls--;
        if (prefixoSoSimbolos(ls, a)) return 1; }
    }
  return 0;
}

// Token isolado reconhece .DV., DV/HDR e [DV], mas nunca DVD/DVDRip.
static int token(const char *s, const char *t) {
  size_t n = strlen(t);
  const char *p;
  for (p = s; *p; p++)
    if ((p == s || !isalnum((unsigned char)p[-1])) &&
        !strncasecmp(p, t, n) && !isalnum((unsigned char)p[n])) return 1;
  return 0;
}

// CABECALHOS EXIGIDOS PELO ADDON: behaviorHints.proxyHeaders.request.
//
// MEDIDO em 17/09 contra o addon de um relato: o CDN devolve 403 sem
// Referer/Origin/User-Agent e 200 com eles. O parser entrava em behaviorHints
// so para pegar bingeGroup, entao esses cabecalhos eram descartados e QUALQUER
// addon que dependa de Referer ficava sem tocar, nos dois alvos.
//
// A MAO, e nao com js_texto: os nomes das chaves aqui sao escolhidos pelo
// ADDON (sao nomes de cabecalho HTTP), entao nao ha lista de chaves conhecidas
// para procurar — e preciso varrer os pares que existirem. So profundidade 1
// de "request", que e onde a convencao do Stremio poe os cabecalhos.
//
// Saida no formato que rede.h aceita: uma linha "Nome: valor" por cabecalho.
static void lerProxyHeaders(const char *bh, const char *fim, char *dst, unsigned tam) {
  const char *ph, *rq, *q;
  unsigned u = 0;
  dst[0] = 0;
  ph = strstr(bh, "\"proxyHeaders\"");
  if (!ph || ph >= fim) return;
  rq = strstr(ph, "\"request\"");
  if (!rq || rq >= fim) return;
  q = strchr(rq, '{');
  if (!q || q >= fim) return;
  q++;
  // Varre pares "nome":"valor" ate a chave que fecha o objeto. Valor de
  // cabecalho e string, entao um '{' aqui e coisa que nao se esperava: encerra
  // em vez de tentar adivinhar.
  while (q < fim && *q && *q != '}' && *q != '{') {
    char nome[96], valor[320];
    unsigned k;
    const char *f;
    while (q < fim && *q && *q != '"' && *q != '}') q++;
    if (q >= fim || *q != '"') break;
    q++;
    f = q;
    while (f < fim && *f && *f != '"') f++;
    if (f >= fim || *f != '"') break;
    k = (unsigned)(f - q);
    if (k >= sizeof nome) k = sizeof nome - 1;
    memcpy(nome, q, k); nome[k] = 0;
    q = f + 1;
    while (q < fim && *q && *q != ':') q++;
    if (q >= fim || *q != ':') break;
    q++;
    while (q < fim && (*q == ' ' || *q == '\t')) q++;
    if (q >= fim || *q != '"') break;   // valor nao-string: nao e cabecalho
    q++;
    f = q;
    while (f < fim && *f && *f != '"') f++;
    if (f >= fim || *f != '"') break;
    k = (unsigned)(f - q);
    if (k >= sizeof valor) k = sizeof valor - 1;
    memcpy(valor, q, k); valor[k] = 0;
    q = f + 1;
    if (nome[0] && valor[0]) {
      int esc = snprintf(dst + u, tam - u, "%s%s: %s", u ? "\n" : "", nome, valor);
      if (esc < 0 || (unsigned)esc >= tam - u) { dst[u] = 0; break; }
      u += (unsigned)esc;
    }
    while (q < fim && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')) q++;
    if (q < fim && *q == ',') q++;
  }
}

// "FORA DE CACHE" DITO PELO ADDON. As marcas sao as que os addons mandam de
// verdade: "⏳" e o que o AIOStreams poe no nome de toda fonte nao cacheada
// (registros 1136, 2191, 2501: "⏳ UHD", "⏳ FHD"; a cacheada vem com "⚡"), o
// Torrentio escreve "[TB download]" / "[RD download]" (a cacheada e "[TB+]"),
// e outros usam "⬇" ou a palavra "uncached". "download]" e nao "download"
// solto: "WEB-DL"/"Download" aparecem em titulo de fonte cacheada.
int stream_texto_fora_de_cache(const char *t) {
  if (!t) return 0;
  return strstr(t, "\xe2\x8f\xb3") != NULL          // U+23F3 ⏳
      || strstr(t, "\xe2\xac\x87") != NULL          // U+2B07 ⬇
      || contem(t, " download]") || contem(t, "uncached") || contem(t, "not cached");
}

// "sources": ["tracker:udp://...", "dht:<hash>"] de um stream de torrent, uma
// entrada por linha em `dst`. So as que o servidor de streaming do Stremio
// entende (prefixo tracker: ou dht:); uma URL solta de tracker ganha o prefixo.
// A entrada que nao cabe inteira fica de fora (nunca cortada no meio: um
// tracker pela metade e endereco torto). \/ vira / (JSON escapa a barra).
static void lerFontesP2P(const char *ini, const char *fim, char *dst, unsigned tam) {
  const char *q = strstr(ini, "\"sources\"");
  unsigned u = 0;
  dst[0] = 0;
  if (!q || q >= fim) return;
  q += 9;
  while (q < fim && (*q == ' ' || *q == ':' || *q == '\n' || *q == '\t')) q++;
  if (q >= fim || *q != '[') return;
  q++;
  while (q < fim && *q != ']') {
    char e[300];
    unsigned k = 0;
    if (*q != '"') { q++; continue; }
    q++;
    while (q < fim && *q != '"' && k + 1 < sizeof e) {
      if (*q == '\\' && q + 1 < fim) q++;      // \/ -> /
      e[k++] = *q++;
    }
    e[k] = 0;
    while (q < fim && *q != '"') q++;          // entrada maior que o buffer: pula o resto
    if (q < fim) q++;
    if (!strncmp(e, "tracker:", 8) || !strncmp(e, "dht:", 4) ||
        !strncmp(e, "udp://", 6) || !strncmp(e, "http://", 7) || !strncmp(e, "https://", 8)) {
      char ent[320];
      int L = snprintf(ent, sizeof ent, "%s%s",
                       strncmp(e, "tracker:", 8) && strncmp(e, "dht:", 4) ? "tracker:" : "", e);
      if (L > 0 && u + (unsigned)L + 2 <= tam) {
        u += (unsigned)snprintf(dst + u, tam - u, "%s%s", u ? "\n" : "", ent);
      }
    }
  }
}

// SEMEADORES no texto do addon. Nao ha campo para isso no protocolo: cada
// addon escreve do seu jeito, sempre marcador + numero. Torrentio e
// MediaFusion usam "👤 12", Comet/Jackettio "👥 12", outros "Seeders: 12".
// Devolve 1 e preenche *qtd; limite de 9 digitos para nao estourar o int.
static int lerSemeadores(const char *t, int *qtd) {
  static const char *const marcas[] = { "\xF0\x9F\x91\xA4", "\xF0\x9F\x91\xA5", "\xF0\x9F\x8C\xB1",
                                        "Seeders", "seeders", "Seeds", "seeds" };
  for (size_t k = 0; k < sizeof marcas / sizeof *marcas; k++) {
    const char *p = strstr(t, marcas[k]);
    if (!p) continue;
    p += strlen(marcas[k]);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (!isdigit((unsigned char)*p)) continue;
    long v = 0; int d = 0;
    while (isdigit((unsigned char)*p) && d < 9) { v = v * 10 + (*p++ - '0'); d++; }
    *qtd = (int)v;
    return 1;
  }
  return 0;
}

int stream_extrair(const char *json, const char *provedor, Stream **saida) {
  const char *p, *fim;
  int n = 0, cap = 0;
  Stream *v = NULL;
  *saida = NULL;
  if (!json) return 0;
  p = js_array(json, json + strlen(json), "streams");
  while (p && *p == '{') {
    Stream s = {0};
    char titulo[2048] = "", texto[5000];
    fim = js_fim(p);
    if (!fim || fim <= p) break;
    js_texto(p, fim, "url", s.url, sizeof s.url);
    if (!s.url[0]) js_texto(p, fim, "externalUrl", s.url, sizeof s.url);
    s.fileIdx = -1;
    // Torrent puro: sem url, com infoHash (no topo ou dentro de clientResolve,
    // como o AIOStreams manda). A url nasce depois, no debrid.
    if (!s.url[0] || strncmp(s.url, "http", 4)) {
      const char *cr = strstr(p, "\"clientResolve\"");
      s.url[0] = 0;
      if (!js_texto(p, fim, "infoHash", s.infoHash, sizeof s.infoHash) && cr && cr < fim)
        js_texto(cr, fim, "infoHash", s.infoHash, sizeof s.infoHash);
      double indice = js_num(p, fim, "fileIdx", -1);
      if (isfinite(indice) && indice >= 0 && indice <= INT_MAX)
        s.fileIdx = (int)indice;
      if (s.infoHash[0]) lerFontesP2P(p, fim, s.fontes, sizeof s.fontes);
    }
    // Nao tocar URL cortada; sem url e sem hash nao ha o que tocar.
    if ((s.infoHash[0] || !strncmp(s.url, "http", 4)) &&
        strlen(s.url) < sizeof s.url - 1) {
      js_texto(p, fim, "name", s.rotulo, sizeof s.rotulo);
      // So o name da PROPRIA fonte, inteiro, conta para o FHD (#402): o
      // rotulo de 192 bytes pode cortar "...FHDx" em "...FHD", e o provedor
      // entra no lugar do name que falta mais abaixo.
      int fhd = 0;
      { char nome[1024];
        if (js_texto_linhas(p, fim, "name", nome, sizeof nome) && strlen(nome) < sizeof nome - 1)
          fhd = fhdNoNome(nome); }
      js_texto_linhas(p, fim, "description", s.descricao, sizeof s.descricao);
      js_texto_linhas(p, fim, "title", titulo, sizeof titulo);
      metadadosFaixas(p, fim, &s);
      js_texto(p, fim, "filename", s.arquivo, sizeof s.arquivo);
      // behaviorHints.bingeGroup — ANCORADO NO OBJETO, e nao procurado solto.
      //
      // Duas coisas separadas, e as duas custam uma linha:
      //
      // 1. `bh < fim` E O QUE IMPORTA DE VERDADE. strstr varre ate o fim do
      //    documento, nao ate o fim DESTE stream: sem a guarda, uma fonte que
      //    nao manda behaviorHints herdaria o bingeGroup da fonte SEGUINTE do
      //    array — e herdaria calada, com o sintoma sendo o app lembrar da
      //    fonte errada no episodio seguinte. Mesma guarda que o
      //    "clientResolve" logo acima ja usa, e pelo mesmo motivo.
      // 2. js_texto_raiz_em le so as chaves de PROFUNDIDADE 1 de
      //    behaviorHints. behaviorHints tem objeto dentro dele
      //    (proxyHeaders.request/response, com nomes de cabecalho que o addon
      //    escolhe), e a regra da casa para chave aninhada de Stremio esta em
      //    js.h: procurar solto pega a primeira ocorrencia, que nem sempre e a
      //    que se quer. Nas respostas que deu para inspecionar aqui o js_texto
      //    solto daria o mesmo resultado; ancorar e seguro de graca.
      { const char *bh = strstr(p, "\"behaviorHints\"");
        if (bh && bh < fim) {
          js_texto_raiz_em(bh, fim, "bingeGroup", s.bingeGroup, sizeof s.bingeGroup);
          js_texto_raiz_em(bh, fim, "videoHash", s.videoHash, sizeof s.videoHash);
          lerProxyHeaders(bh, fim, s.cabecalhos, sizeof s.cabecalhos);
        } }
      if (!s.descricao[0]) snprintf(s.descricao, sizeof s.descricao, "%s", titulo);
      if (!s.rotulo[0]) snprintf(s.rotulo, sizeof s.rotulo, "%s", provedor);
      snprintf(s.provedor, sizeof s.provedor, "%s", provedor);
      snprintf(texto, sizeof texto, "%s %s %s %s", s.rotulo, s.descricao, titulo, s.arquivo);
      for (char *q = texto; *q; q++) if (*q == '\n') *q = ' ';
      s.altura = contem(texto, "2160") || token(texto, "4k") || token(texto, "uhd") ? 2160 :
                 contem(texto, "1440") ? 1440 : contem(texto, "1080") ? 1080 :
                 contem(texto, "720") ? 720 : contem(texto, "480") ? 480 : 0;
      s.dolbyVision = token(texto, "dv") || token(texto, "dovi") ||
                      contem(texto, "dolby vision") || contem(texto, "dolbyvision");
      s.dolbyAtmos = token(texto, "atmos");
      s.badges = badges_detectar(texto);
      // "FHD"/"Full HD" (#402): formatadores tipo AIOStreams poem a resolucao
      // so como sigla no name ("FHD | REMUX | SDR"). Vale so na FALTA de
      // resolucao escrita em qualquer campo, e so do name: descricao e
      // filename trazem URL ("https://fhd...") e grupo de release ("x265-FHD").
      // E nunca ao lado de outro selo de resolucao que o badges_detectar ja deu.
      if (!s.altura && fhd &&
          !(s.badges & (badges_bit("r-4k") | badges_bit("r-1080") | badges_bit("r-720")))) {
        s.altura = 1080;
        s.badges |= badges_bit("r-1080");
      }
      s.mp4 = token(texto, "mp4") || contem(s.url, ".mp4");
      s.foraCache = stream_texto_fora_de_cache(texto);
      // AIOStreams: clientResolve.isCached e explicito; ausente nao e false.
      if (s.infoHash[0] && !s.url[0]) {
        const char *cr = valorRaiz(p, fim, "clientResolve");
        const char *v = cr ? valorRaiz(cr, fim, "isCached") : NULL;
        if (v && fim - v >= 5 && !memcmp(v, "false", 5) &&
            (v + 5 == fim || v[5] == ',' || v[5] == '}' || isspace((unsigned char)v[5])))
          s.foraCache = 1;
      }
      if (s.infoHash[0]) s.temSemeadores = lerSemeadores(texto, &s.semeadores);
      s.tamanhoBytes = bytesExatos(p, fim);
      double bytes = js_num(p, fim, "videoSize", 0);
      if (s.tamanhoBytes) s.tamanhoMB = tamanhoMB((double)s.tamanhoBytes / 1048576.0);
      else if (bytes > 0) s.tamanhoMB = tamanhoMB(bytes / (1024.0 * 1024.0));
      else {
        const char *u = strstr(texto, " GB");
        double escala = 1024;
        if (!u) { u = strstr(texto, " MB"); escala = 1; }
        if (u) {
          const char *ini = u;
          while (ini > texto && (isdigit((unsigned char)ini[-1]) || ini[-1] == '.')) ini--;
          if (ini < u) s.tamanhoMB = tamanhoMB(atof(ini) * escala);
        }
      }
      if (n == cap) {
        int nova = cap ? cap * 2 : 32;
        Stream *tmp = realloc(v, (size_t)nova * sizeof *tmp);
        if (!tmp) { free(v); return -1; }
        v = tmp; cap = nova;
      }
      v[n++] = s;
    }
    p = js_prox(fim);
  }
  *saida = v;
  return n;
}
