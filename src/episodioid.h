// Identidade de um episodio entre numeracoes diferentes. Sem rede ou estado.
#ifndef NV_EPISODIOID_H
#define NV_EPISODIOID_H
#include "catalogo.h"
#include "js.h"
#include <limits.h>
#include <string.h>

typedef struct { int temporada, episodio, tvdbEp; } EpisodioId;

// Apenas chaves da raiz: o TVDB da serie nunca identifica seu episodio.
static inline const char *episodioid_valor(const char *p, const char *fim, const char *chave) {
  int nivel = 0;
  size_t n = strlen(chave);
  if (!p) return NULL;
  if (!fim) fim = p + strlen(p);
  for (; p < fim; p++) {
    if (*p == '"') {
      const char *a = ++p;
      while (p < fim && *p != '"') { if (*p == '\\' && p + 1 < fim) p++; p++; }
      if (p >= fim) return NULL;
      if (nivel == 1 && (size_t)(p - a) == n && !memcmp(a, chave, n)) {
        const char *v = p + 1;
        while (v < fim && (unsigned char)*v <= ' ') v++;
        if (v >= fim || *v != ':') continue;
        v++;
        while (v < fim && (unsigned char)*v <= ' ') v++;
        return v < fim ? v : NULL;
      }
    } else if (*p == '{' || *p == '[') nivel++;
    else if ((*p == '}' || *p == ']') && --nivel == 0) break;
  }
  return NULL;
}

static inline int episodioid_numero(const char *p, const char *fim) {
  int n = 0;
  if (!p || !fim || p >= fim || *p < '1' || *p > '9') return 0;
  while (p < fim && *p >= '0' && *p <= '9') {
    int d = *p++ - '0';
    if (n > (INT_MAX - d) / 10) return 0;
    n = n * 10 + d;
  }
  while (p < fim && (unsigned char)*p <= ' ') p++;
  return p == fim || *p == ',' || *p == '}' || *p == ']' ? n : 0;
}

// Season zero is explicit metadata, not an absent/invalid provider ID.
// Keep episodioid_numero strictly positive for episode and TVDB identities.
static inline int episodioid_temporada(const char *p, const char *fim) {
  int n;
  if (!p || !fim || p >= fim) return -1;
  if (*p != '0') {
    n = episodioid_numero(p, fim);
    return n > 0 ? n : -1;
  }
  p++;
  while (p < fim && (unsigned char)*p <= ' ') p++;
  return p == fim || *p == ',' || *p == '}' || *p == ']' ? 0 : -1;
}

// Stremio/Cinemeta: tvdb_id. Trakt: ids.tvdb, dentro do episodio.
static inline int episodioid_tvdb(const char *ep, const char *fim) {
  const char *p, *ids;
  int direto = 0, aninhado = 0;
  if (!ep) return 0;
  if (!fim) fim = ep + strlen(ep);
  p = episodioid_valor(ep, fim, "tvdb_id");
  if (p && !(direto = episodioid_numero(p, fim))) return 0;
  ids = episodioid_valor(ep, fim, "ids");
  if (ids && *ids == '{') {
    const char *fi = js_fim(ids);
    if (!fi || fi > fim) return 0;
    p = episodioid_valor(ids, fi, "tvdb");
    if (p && !(aninhado = episodioid_numero(p, fi))) return 0;
  }
  if (direto && aninhado && direto != aninhado) return 0;
  return direto ? direto : aninhado;
}

// A correspondencia tem de ser unica nos dois sentidos: ID e par da fonte.
static inline int episodioid_procurar(const CatEp *eps, int n, int tvdb, EpisodioId *saida) {
  EpisodioId achado = {0};
  if (!eps || tvdb <= 0 || !saida) return 0;
  for (int i = 0; i < n; i++) if (eps[i].tvdbEp == tvdb) {
    if (eps[i].temporada < 0 || eps[i].episodio <= 0) return 0;
    if (achado.tvdbEp && (achado.temporada != eps[i].temporada || achado.episodio != eps[i].episodio)) return 0;
    achado = (EpisodioId){ eps[i].temporada, eps[i].episodio, tvdb };
  }
  if (!achado.tvdbEp) return 0;
  for (int i = 0; i < n; i++)
    if (eps[i].temporada == achado.temporada && eps[i].episodio == achado.episodio &&
        eps[i].tvdbEp > 0 && eps[i].tvdbEp != tvdb) return 0;
  *saida = achado;
  return 1;
}

static inline int episodioid_meta(const char *corpo, const char *titulo, int tvdb, EpisodioId *saida) {
  const char *m, *fim, *v, *videos;
  EpisodioId achado = {0};
  char id[64] = "", tipo[12] = "";
  if (!corpo || !titulo || tvdb <= 0 || !saida) return 0;
  m = episodioid_valor(corpo, NULL, "meta");
  if (!m || *m != '{') return 0;
  fim = js_fim(m);
  js_texto_raiz_em(m, fim, "id", id, sizeof id);
  js_texto_raiz_em(m, fim, "type", tipo, sizeof tipo);
  if ((id[0] && strcmp(id, titulo)) || (tipo[0] && strcmp(tipo, "series"))) return 0;
  videos = episodioid_valor(m, fim, "videos");
  if (!videos || *videos != '[') return 0;
  for (v = js_raiz_array(videos); v; v = js_prox(js_fim(v))) {
    const char *f = js_fim(v);
    EpisodioId e;
    if (episodioid_tvdb(v, f) != tvdb) continue;
    e.temporada = episodioid_temporada(episodioid_valor(v, f, "season"), f);
    e.episodio = episodioid_numero(episodioid_valor(v, f, "episode"), f);
    e.tvdbEp = tvdb;
    if (e.temporada < 0 || !e.episodio) return 0;
    if (achado.tvdbEp && (achado.temporada != e.temporada || achado.episodio != e.episodio)) return 0;
    achado = e;
  }
  if (!achado.tvdbEp) return 0;
  for (v = js_raiz_array(videos); v; v = js_prox(js_fim(v))) {
    const char *f = js_fim(v);
    int outro = episodioid_tvdb(v, f);
    if (outro > 0 && outro != tvdb &&
        episodioid_temporada(episodioid_valor(v, f, "season"), f) == achado.temporada &&
        episodioid_numero(episodioid_valor(v, f, "episode"), f) == achado.episodio) return 0;
  }
  *saida = achado;
  return 1;
}
#endif
