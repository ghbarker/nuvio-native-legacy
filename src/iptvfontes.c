#include "iptvfontes.h"
#include "dados.h"
#include "perfis.h"
#include "perfisessao.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <dirent.h>

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static IptvConta contas[IPTV_MAX_CONTAS];
static int nContas, selecionada, perfil = -1;
static unsigned proxima = 1;
static uint64_t sessao;

static const char *tipoNome(int tipo) {
  return tipo == IPTV_XTREAM ? "Xtream" : tipo == IPTV_M3U ? "M3U" : "Stalker";
}
static void nomeArquivo(char *out, unsigned n, int p) {
  snprintf(out, n, "livetv-contas-p%d.txt", p);
}
static int gravar(void) {
  char nome[64], corpo[8192];
  int w = snprintf(corpo, sizeof corpo, "next\t%u\nselected\t%d\n", proxima, selecionada);
  for (int i = 0; i < nContas; i++) {
    int k = snprintf(corpo + w, sizeof corpo - (size_t)w, "account\t%u\t%d\t%d\t%s\n",
                     contas[i].id, contas[i].tipo, contas[i].ativa, contas[i].nome);
    if (k < 0 || (size_t)k >= sizeof corpo - (size_t)w) return 0;
    w += k;
  }
  nomeArquivo(nome, sizeof nome, perfil);
  if (!nv_perfil_sessao_travar(sessao)) return 0;
  int ok = dados_gravar(nome, corpo);
  nv_perfil_sessao_destravar(); return ok;
}
static void carregar(void) {
  int p = perfis_ativo(); uint64_t s = nv_perfil_sessao_ler();
  if (perfil == p && sessao == s) return;
  perfil = p; sessao = s; nContas = selecionada = 0; proxima = 1;
  char nome[64]; nomeArquivo(nome, sizeof nome, p);
  char *b = dados_ler(nome);
  if (b) {
    for (char *l = b; *l;) {
      char *f = strchr(l, '\n'); if (f) *f++ = 0; else f = l + strlen(l);
      unsigned id, next; int tipo, ativa, idx; char rotulo[96];
      if (sscanf(l, "next\t%u", &next) == 1 && next && next < UINT_MAX) proxima = next;
      else if (sscanf(l, "selected\t%d", &idx) == 1) selecionada = idx;
      else if (nContas < IPTV_MAX_CONTAS &&
               sscanf(l, "account\t%u\t%d\t%d\t%95[^\n]", &id, &tipo, &ativa, rotulo) == 4 &&
               id < UINT_MAX && tipo >= IPTV_XTREAM && tipo <= IPTV_STALKER && !strchr(rotulo, '\t')) {
        int repetida = 0;
        for (int i = 0; i < nContas; i++) repetida |= contas[i].id == id && contas[i].tipo == tipo;
        if (!repetida) {
          contas[nContas] = (IptvConta){.id = id, .tipo = tipo, .ativa = !!ativa};
          snprintf(contas[nContas++].nome, 96, "%s", rotulo);
          if (id >= proxima) proxima = id + 1;
        }
      }
      l = f;
    }
    free(b);
  } else {
    // Legacy files stay in place. Their account-zero IDs remain valid.
    for (int tipo = IPTV_XTREAM; tipo <= IPTV_STALKER; tipo++) {
      const char *base = tipo == IPTV_XTREAM ? "xtream" : tipo == IPTV_M3U ? "m3u" : "stalker";
      snprintf(nome, sizeof nome, "%s-p%d.txt", base, p);
      char *antigo = dados_ler(nome);
      if (antigo && *antigo) {
        contas[nContas] = (IptvConta){.tipo = tipo, .ativa = 1};
        snprintf(contas[nContas++].nome, 96, "%s", tipoNome(tipo));
      }
      free(antigo);
    }
    // Persist once so removing the legacy account cannot reimport it later.
    if (nContas) gravar();
  }
  if (selecionada < 0 || selecionada >= nContas) selecionada = 0;
}
int iptv_contas(IptvConta *out, int max) {
  pthread_mutex_lock(&trava); carregar(); int n = nContas;
  if (out && max > 0) { if (n > max) n = max; memcpy(out, contas, sizeof *out * (size_t)n); }
  pthread_mutex_unlock(&trava); return n;
}
int iptv_conta_selecionada(IptvConta *out) {
  pthread_mutex_lock(&trava); carregar(); int ok = nContas > 0;
  if (ok && out) *out = contas[selecionada];
  pthread_mutex_unlock(&trava); return ok;
}
int iptv_conta_indice(void) {
  pthread_mutex_lock(&trava); carregar(); int i = selecionada; pthread_mutex_unlock(&trava); return i;
}
int iptv_conta_selecionar(int i) {
  pthread_mutex_lock(&trava); carregar(); int anterior = selecionada;
  int ok = i >= 0 && i < nContas;
  if (ok) { selecionada = i; if (!(ok = gravar())) selecionada = anterior; }
  pthread_mutex_unlock(&trava); return ok;
}
int iptv_conta_adicionar(int tipo) {
  pthread_mutex_lock(&trava); carregar();
  int ok = tipo >= IPTV_XTREAM && tipo <= IPTV_STALKER && nContas < IPTV_MAX_CONTAS && proxima < UINT_MAX;
  if (ok) {
    unsigned anterior = proxima; int sel = selecionada, numero = 1;
    for (int i = 0; i < nContas; i++) numero += contas[i].tipo == tipo;
    contas[nContas] = (IptvConta){.id = proxima++, .tipo = tipo, .ativa = 1};
    snprintf(contas[nContas].nome, 96, "%s %d", tipoNome(tipo), numero);
    selecionada = nContas++;
    if (!(ok = gravar())) { nContas--; selecionada = sel; proxima = anterior; }
  }
  pthread_mutex_unlock(&trava); return ok;
}
int iptv_conta_renomear(const char *nome) {
  if (!nome || !*nome || strlen(nome) >= 96) return 0;
  for (const unsigned char *p = (const unsigned char *)nome; *p; p++) if (*p < 32 || *p == 127) return 0;
  pthread_mutex_lock(&trava); carregar(); int ok = nContas > 0;
  if (ok) {
    char antigo[96]; memcpy(antigo, contas[selecionada].nome, 96);
    snprintf(contas[selecionada].nome, 96, "%s", nome);
    if (!(ok = gravar())) memcpy(contas[selecionada].nome, antigo, 96);
  }
  pthread_mutex_unlock(&trava); return ok;
}
int iptv_conta_ativar(int ativa) {
  pthread_mutex_lock(&trava); carregar(); int ok = nContas > 0;
  if (ok) {
    int anterior = contas[selecionada].ativa; contas[selecionada].ativa = !!ativa;
    if (!(ok = gravar())) contas[selecionada].ativa = anterior;
  }
  pthread_mutex_unlock(&trava); return ok;
}
int iptv_conta_remover(IptvConta *removida) {
  pthread_mutex_lock(&trava); carregar(); int ok = nContas > 0;
  if (ok) {
    IptvConta copia[IPTV_MAX_CONTAS]; memcpy(copia, contas, sizeof copia);
    int anterior = selecionada; IptvConta alvo = contas[selecionada];
    nContas--; memmove(contas + selecionada, contas + selecionada + 1, sizeof *contas * (size_t)(nContas - selecionada));
    if (selecionada >= nContas) selecionada = nContas ? nContas - 1 : 0;
    if (!(ok = gravar())) { nContas++; selecionada = anterior; memcpy(contas, copia, sizeof copia); }
    else {
      if(removida)*removida=alvo;
      static const char *ext[]={"bin","ts","id"};char nome[80];
      for(int i=0;i<3;i++) {
        snprintf(nome,sizeof nome,"epg-conta-%d-%08x-p%d.%s",alvo.tipo,alvo.id,perfil,ext[i]);
        dados_apagar(nome);
      }
    }
  }
  pthread_mutex_unlock(&trava); return ok;
}

void iptv_contas_esquecer_todas(void) {
  // Enumerate only files owned by native Live TV sources; this also removes
  // orphaned account caches if a process exited between metadata/file removal.
  DIR *dir = opendir(dados_dir());
  if (dir) {
    struct dirent *e;
    while ((e = readdir(dir))) {
      const char *n = e->d_name;
      if (!strncmp(n,"livetv-contas-p",14) || !strncmp(n,"xtream-p",8) ||
          !strncmp(n,"stalker-p",9) || !strncmp(n,"m3u-p",5) ||
          !strncmp(n,"m3u-cache-p",11) || !strncmp(n,"epg-conta-",10) ||
          !strncmp(n,"epg-extra-p",11) || !strncmp(n,"epg-extra-m3u-p",15)) dados_apagar(n);
    }
    closedir(dir);
  }
  pthread_mutex_lock(&trava); perfil = -1; nContas = selecionada = 0; proxima = 1;
  pthread_mutex_unlock(&trava);
}
