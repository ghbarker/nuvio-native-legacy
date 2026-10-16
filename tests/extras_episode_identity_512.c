// #512: a primeira importacao nao pode gravar a numeracao do Trakt no
// catalogo de outra fonte. Inclui verbatim os leitores e alvos de producao;
// js.c e vistoep.c sao reais. Somente conta, rede e publicacao de catalogo
// sao fronteiras tipadas. Nao pretende testar o mutex real de perfis.c.
#include "catalogo.h"
#include "episodioid.h"
#include "extras.h"
#include "perfisessao.h"
#include "proximo.h"
#include "vistoep.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "trakt-lease-decl.inc"

static NvPerfilSessao perfil = 7, sessaoPedido = 7;
static unsigned long long credencial = 3, historico = 9;
static unsigned long long credencialPedido = 3;
static char idPedido[24] = "tt512", origemPedido[96] = "addon:A";
static int gerPedida = 1, historias;
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t travaCredTeste = PTHREAD_MUTEX_INITIALIZER;
static int lease, trocarNaEntrada;
#include "extras-state.inc"

uint64_t perfis_sessao_geracao(void) { return perfil; }
int perfis_sessao_atual(uint64_t s) { return s == perfil; }
int perfis_sessao_travar(uint64_t s) { return s == perfil; }
void perfis_sessao_destravar(void) { }
unsigned long long trakt_credencial_geracao(void) { assert(!lease); return credencial; }
unsigned long long cat_historico_geracao(void) { return historico; }
int cat_historico_definir_se_geracao(const char *id, const char *tipo, int visto,
                                    unsigned long long h) {
  assert(!strcmp(id, idPedido) && !strcmp(tipo, "series"));
  assert(lease && h == historico); (void)visto; historias++; return 1;
}
int trakt_historico_aplicar_se_atual(uint64_t s, unsigned long long h,
                                    unsigned long long c, int (*aplicar)(void *), void *dados) {
  int r;
  assert(!lease); pthread_mutex_lock(&travaCredTeste);
  if (trocarNaEntrada) { credencial++; trocarNaEntrada = 0; }
  if (s != perfil || h != historico || c != credencial) {
    pthread_mutex_unlock(&travaCredTeste); return 0;
  }
  lease = 1; r = aplicar(dados); lease = 0;
  pthread_mutex_unlock(&travaCredTeste); return r;
}

static CatEp fonte[96];
static int nFonte, copias, falhaCopia, trocaToken, trocaPerfil;
static unsigned token = 1;
static unsigned versaoFonte = 1;
unsigned cat_versao_episodios(void) { return versaoFonte; }
unsigned desc_episodios_escolha_pronta(const char *id, const char *origem, uint64_t s) {
  return s == perfil && !strcmp(id, "tt512") && !strcmp(origem, "addon:A") ? token : 0;
}
int cat_copiar_episodios_fonte(const char *id, const char *origem, uint64_t s,
                              CatEp **saida, int *qtd) {
  assert(!strcmp(id, "tt512") && !strcmp(origem, "addon:A") && s == perfil);
  *saida = NULL; *qtd = 0; copias++;
  if (falhaCopia) return 0;
  if (nFonte) { *saida = malloc((size_t)nFonte * sizeof **saida); assert(*saida);
    memcpy(*saida, fonte, (size_t)nFonte * sizeof **saida); }
  *qtd = nFonte;
  if (trocaToken) token++;
  if (trocaPerfil) perfil++;
  return 1;
}
#include "extras-import.inc"
#include "extras-read.inc"

// Publicacao de notas/trailers e workers de rede sao vizinhos, nao a politica
// de admissao exercitada aqui. Os dois workers permanentes ja estao presentes.
static int notaTrakt, votosTrakt, nComent, nRel, nTemps, nCol, nTrailer, fichaDur, nEstudio;
static int agTemp, agEp, notas[EX_NFONTES];
static char colNome[80], colCapa[160], colFundo[160], colSinopse[600];
static char fichaStatus[32], fichaPaises[160], fichaCert[12], fichaLanc[16], fichaIdiomaOrig[16];
static char agStatus[80], agDataProx[80], agDataUlt[80], agNomeEp[160];
static char mdbChave[80], idEmCurso[24], origemEmCurso[96];
static int seriePedido = 1, serieEmCurso, fioVivo, fiosProntos = 1;
static long tmdbPedido, tmdbEmCurso;
static pthread_t fio;
static pthread_cond_t cvTrabalho = PTHREAD_COND_INITIALIZER;
int trakt_ativo(void) { return 1; }
const char *desc_chave_tmdb(void) { return ""; }
int ajustes_mdblist_ligado(void) { return 0; }
static void *lacoParte(void *p) { (void)p; assert(!"worker inesperado"); return NULL; }
static void *buscar(void *p) { (void)p; assert(!"worker inesperado"); return NULL; }
#include "extras-reset.inc"
#include "extras-request.inc"

static CatItem item;
static int idx, retomarFonteTvdb, retomarFonteT, retomarFonteE;
static unsigned retomarFonteToken, retomarFonteVersao;
static NvPerfilSessao retomarFonteSessao;
static int retomarFonteConcluido, retomarFonteProxT, retomarFonteProxE;
enum { SEC_EPISODIOS = 4 };
static struct { int fileira, coluna; } foco;
static int pedReproduzir, pedDoInicio;
static int aberto = 1, saindo, avisos;
static char aviso[160];
enum { ILHA_INFO = 0 };
const char *i18n(const char *s) { return s; }
void ilha_avisar(const char *chave, int tipo, const char *icone,
                  const char *texto, unsigned ms, int tecla) {
  assert(!strcmp(chave, "det-ep-identidade") && tipo == ILHA_INFO && !icone && ms && !tecla);
  snprintf(aviso, sizeof aviso, "%s", texto); avisos++;
}
const CatItem *cat_item(int i) { (void)i; return &item; }
const CatEp *cat_episodio(int i, int e) { (void)i; return e >= 0 && e < nFonte ? &fonte[e] : NULL; }
int cat_n_episodios(int i) { (void)i; return nFonte; }
static int epAbsoluto(int coluna) { return coluna; }
int ajustes_cw_concluido(void) { return 90; }
#include "detail-target.inc"
#include "detail-ready.inc"
#include "detail-play.inc"
#include "detail-start.inc"
int detail_aberto(void) { return aberto; }
int detail_indice(void) { return idx; }
int cat_id_stream(int i, int t, int e, char *dst, unsigned n) {
  (void)i; snprintf(dst, n, "%s:%d:%d", item.imdb, t, e); return 1;
}
#include "app-target.inc"
static int cwTocarT, cwTocarE, cwTocarExplicito, playerT, playerE;
static char cwTocarId[64], cwTocarOrigem[96];
static NvPerfilSessao cwTocarSessao;
void player_definir_episodio(int t, int e) { playerT = t; playerE = e; }
static char alvoDaLista[64];
int stream_lista_do_alvo(const char *id) { return !strcmp(id, alvoDaLista); }
#include "app-play.inc"

static ExProgressoPedido dono(void) {
  ExProgressoPedido p = {0};
  snprintf(p.id, sizeof p.id, "%s", idPedido);
  snprintf(p.origem, sizeof p.origem, "%s", origemPedido);
  p.sessao = sessaoPedido; p.geracao = gerPedida;
  p.credencial = credencial; p.historico = historico;
  return p;
}
static void guardar(const char *json) {
  ExProgressoPedido p = dono(); credencialPedido = credencial; progressoGuardar(&p, strdup(json));
}
static void ids(const char *json) { ExProgressoPedido p = dono(); progressoGuardarIds(&p, json); }
static void ep(int i, int t, int e, int tvdb) {
  assert(i < (int)(sizeof fonte / sizeof fonte[0]));
  fonte[i] = (CatEp){0}; fonte[i].temporada = t; fonte[i].episodio = e; fonte[i].tvdbEp = tvdb;
  if (nFonte <= i) nFonte = i + 1;
}
static void limpar(void) {
  progressoLimpar(); idsTraktLimpar(); vistoep_esquecer(); vistoep_lapides(NULL, NULL);
  perfil = sessaoPedido = 7; credencial = credencialPedido = 3; historico = 9; gerPedida = 1;
  snprintf(idPedido, sizeof idPedido, "tt512"); snprintf(origemPedido, sizeof origemPedido, "addon:A");
  token = versaoFonte = 1; historias = copias = falhaCopia = trocaToken = trocaPerfil = trocarNaEntrada = 0;
  nFonte = 0; memset(fonte, 0, sizeof fonte); memset(vistos, 0, sizeof vistos);
  progressoPronto = progressoAgregadoPronto = proximoT = proximoE = 0;
  epsExibidos = epsVistos = epsBase = epsRev = 0;
  memset(&item, 0, sizeof item); snprintf(item.imdb, sizeof item.imdb, "tt512");
  snprintf(item.origem, sizeof item.origem, "addon:A"); foco.fileira = foco.coluna = 0;
  snprintf(item.tipo, sizeof item.tipo, "series");
  retomarFonteToken = 0; pedReproduzir = pedDoInicio = avisos = saindo = 0;
  cwTocarT = cwTocarE = cwTocarExplicito = playerT = playerE = 0;
  seriePedido = fiosProntos = 1; alvoDaLista[0] = 0;
  aberto = 1; aviso[0] = 0;
}
static const char *PROGRESSO =
  "{\"aired\":2,\"completed\":1,\"seasons\":["
    "{\"number\":1,\"episodes\":[{\"number\":1,\"completed\":false}]},"
    "{\"number\":2,\"episodes\":[{\"number\":1,\"completed\":true,"
      "\"last_watched_at\":\"2026-10-01T12:00:00Z\"}]}],"
    "\"next_episode\":{\"season\":1,\"number\":1}}";
static const char *IDENTIDADES =
  "[{\"number\":1,\"episodes\":[{\"number\":1,\"ids\":{\"tvdb\":101}}]},"
   "{\"number\":2,\"episodes\":[{\"number\":1,\"ids\":{\"tvdb\":201}}]}]";

static void primeiroJoin(void) {
  int v, n, t = 0, e = 0;
  limpar(); ep(0, 1, 1, 101); ep(1, 1, 13, 201); token = 0;
  vistoep_definir("tt512", 1, 9, 1);
  guardar(PROGRESSO); extras_atualizar_episodios();
  assert(extras_progresso_serie(&v, &n) && v == 1 && n == 2);
  assert(!extras_progresso_pronto() && !extras_proximo_episodio(&t, &e));
  assert(vistoep_total("tt512") == 1 && vistoep_estado("tt512", 1, 9) == 1 && copias == 0);
  token = 1; extras_atualizar_episodios();
  assert(!extras_progresso_pronto() && vistoep_estado("tt512", 2, 1) == -1);
  for (int i = 0; i < 15; i++) extras_atualizar_episodios();
  assert(copias == 1); token++; extras_atualizar_episodios(); assert(copias == 2);
  versaoFonte++; extras_atualizar_episodios(); assert(copias == 3);
  ids(IDENTIDADES); extras_atualizar_episodios();
  assert(extras_progresso_pronto() && !progressoPendente);
  assert(vistoep_estado("tt512", 1, 13) == 1 && vistoep_estado("tt512", 1, 1) == 0);
  assert(vistoep_estado("tt512", 2, 1) == -1 && vistoep_estado("tt512", 1, 9) == 1);
  assert(extras_proximo_episodio(&t, &e) && t == 1 && e == 1 && historias == 1);
  puts("ok  primeiro join aguarda fonte final sem atrasar agregados ou apagar marcas locais");
}

static void colisaoMesmoNumero(void) {
  limpar(); ep(0, 1, 1, 201); ep(1, 1, 13, 101); ids(IDENTIDADES);
  guardar(PROGRESSO); extras_atualizar_episodios();
  assert(vistoep_estado("tt512", 1, 1) == 1 && vistoep_estado("tt512", 1, 13) == 0);
  assert(proximoT == 1 && proximoE == 13 && vistoep_estado("tt512", 2, 1) == -1);
  puts("ok  colisao de pares iguais usa TVDB e nao presume que T1E1 e o mesmo episodio");
}

static void donoETicket(void) {
  for (int qual = 0; qual < 7; qual++) {
    limpar(); ep(0, 1, 13, 201); ids(IDENTIDADES); guardar(PROGRESSO);
    switch (qual) {
      case 0: gerPedida++; break; case 1: perfil++; break; case 2: credencial++; break;
      case 3: historico++; break; case 4: strcpy(origemPedido, "addon:B"); break;
      case 5: trocaToken = 1; break; case 6: falhaCopia = 1; break;
    }
    extras_atualizar_episodios();
    assert(vistoep_n() == 0 && !progressoPronto && !historias && progressoPendente);
  }
  limpar(); ep(0, 1, 13, 201); ids(IDENTIDADES); guardar(PROGRESSO); trocaPerfil = 1;
  extras_atualizar_episodios(); assert(vistoep_n() == 0 && !progressoPronto);
  puts("ok  pedido origem perfil credencial historico e ticket antigos nunca publicam");
}

static void ambiguidades(void) {
  limpar(); ep(0, 1, 1, 201); ep(1, 1, 13, 201); ids(IDENTIDADES);
  guardar(PROGRESSO); extras_atualizar_episodios();
  assert(vistoep_n() == 0 && !proximoT && !proximoE);
  limpar(); ep(0, 1, 1, 201);
  ids("[{\"number\":1,\"episodes\":[{\"number\":1,\"ids\":{\"tvdb\":201}},"
      "{\"number\":2,\"ids\":{\"tvdb\":201}}]}]");
  guardar(PROGRESSO); extras_atualizar_episodios(); assert(vistoep_n() == 0 && !proximoE);
  limpar(); ep(0, 1, 1, 201);
  ids("[{\"number\":2,\"episodes\":[{\"number\":1,\"ids\":{\"tvdb\":201}},"
      "{\"number\":1,\"ids\":{\"tvdb\":202}}]}]");
  guardar(PROGRESSO); extras_atualizar_episodios(); assert(vistoep_n() == 0 && !proximoE);
  limpar(); ep(0, 1, 13, 101); ep(1, 1, 13, 201); ids(IDENTIDADES);
  guardar(PROGRESSO); extras_atualizar_episodios(); assert(vistoep_n() == 0 && !proximoE);
  puts("ok  identidades duplicadas ou conflitantes nao caem na numeracao crua");
}

static void legadoECache(void) {
  limpar(); ep(0, 2, 1, 0); guardar(PROGRESSO); extras_atualizar_episodios();
  assert(progressoPronto && vistoep_estado("tt512", 2, 1) == 1 && !idsTraktProntos);
  limpar(); ep(0, 1, 13, 201); ids(IDENTIDADES);
  // A identidade do Trakt pode ser reutilizada ao mudar a fonte selecionada;
  // a fonte final continua sendo consultada novamente pelo ticket corrente.
  gerPedida++; guardar(PROGRESSO); extras_atualizar_episodios();
  assert(progressoPronto && vistoep_estado("tt512", 1, 13) == 1);
  puts("ok  fonte final sem IDs conserva legado e cache de IDs evita nova espera");
}

static int lapide(const char *id, int t, int e, long long ms) {
  assert(!strcmp(id, "tt512"));
  assert(lease && pthread_mutex_trylock(&travaCredTeste) == EBUSY);
  (void)ms; return t == 1 && e == 13;
}
static void manualPrevalece(void) {
  limpar(); ep(0, 1, 1, 101); ep(1, 1, 13, 201); ids(IDENTIDADES);
  vistoep_definir("tt512", 1, 13, 0); vistoep_lapides(lapide, NULL);
  guardar(PROGRESSO); extras_atualizar_episodios();
  assert(vistoep_estado("tt512", 1, 13) == 0 && !extras_ep_visto(1, 13));
  assert(epsVistos == 0 && vistoep_estado("tt512", 2, 1) == -1);
  vistoep_lapides(NULL, NULL);
  puts("ok  lapide local e timestamp continuam passando pelo mapa real");
}

static void temporadasLongas(void) {
  char json[10000]; size_t u; limpar();
  u = (size_t)snprintf(json, sizeof json, "[{\"number\":2,\"episodes\":[");
  for (int i = 1; i <= 65; i++)
    u += (size_t)snprintf(json + u, sizeof json - u, "%s{\"number\":%d,\"ids\":{\"tvdb\":%d}}", i > 1 ? "," : "", i, 200 + i);
  snprintf(json + u, sizeof json - u, "]}]"); ids(json);
  assert(idsTraktProntos && nIdsTrakt == 65 && progressoTvdb(2, 65, 0) == 265);
  ep(0, 1, 65, 265);
  guardar("{\"aired\":65,\"completed\":1,\"seasons\":[{\"number\":2,\"episodes\":[{\"number\":65,\"completed\":true}]}]}");
  extras_atualizar_episodios(); assert(vistoep_estado("tt512", 1, 65) == 1);
  puts("ok  join nao herda teto de trinta episodios das notas");
}

static void idsAusentesEConflitantes(void) {
  limpar(); ep(0, 1, 1, 201);
  ids("[{\"number\":1,\"ids\":{\"tvdb\":201},\"episodes\":[{\"number\":1},"
      "{\"number\":2,\"tvdb_id\":201,\"ids\":{\"tvdb\":202}},"
      "{\"number\":3,\"ids\":{\"tvdb\":0}},"
      "{\"number\":4,\"ids\":{\"tvdb\":2147483648}}]}]");
  assert(idsTraktProntos && nIdsTrakt == 0);
  guardar(PROGRESSO); extras_atualizar_episodios(); assert(vistoep_n() == 0 && !proximoE);
  puts("ok  IDs de serie invalidos ou conflitantes nao identificam episodios");
}

static void cacheNaoCruzaConta(void) {
  for (int qual = 0; qual < 3; qual++) {
    limpar(); ep(0, 1, 13, 201); ids(IDENTIDADES);
    if (qual == 0) { perfil++; sessaoPedido = perfil; }
    else if (qual == 1) credencial++;
    else snprintf(idsDono.id, sizeof idsDono.id, "ttOutro");
    guardar(PROGRESSO); extras_atualizar_episodios();
    assert(vistoep_n() == 0 && progressoPendente && !progressoPronto);
  }
  // ID expresso conflitante com seasons/full nao pode escolher o par da fonte.
  limpar(); ep(0, 1, 13, 201); ids(IDENTIDADES);
  guardar("{\"aired\":1,\"completed\":1,\"seasons\":[{\"number\":2,\"episodes\":["
    "{\"number\":1,\"ids\":{\"tvdb\":999},\"completed\":true}]}],"
    "\"next_episode\":{\"season\":2,\"number\":1,\"ids\":{\"tvdb\":999}}}");
  extras_atualizar_episodios(); assert(vistoep_n() == 0 && !proximoE);
  puts("ok  cache pertence ao titulo perfil e credencial e ID expresso conflitante adia");
}

static void retomarFinalEFoco(void) {
  int t, e, origem;
  limpar(); ep(0, 1, 1, 101); ep(1, 1, 13, 201);
  item.progresso = 25; item.temporada = 2; item.episodio = 1; item.tvdbEp = 201;
  token = 0; assert(!episodioAlvo(&t, &e, &origem));
  // Nem Play nem "do inicio" podem entregar o pedido ao roteador que teria
  // um fallback numerico. O pedido foi consumido: nao reaparece ao chegar meta.
  pedReproduzir = pedDoInicio = 1;
  assert(detail_pediu_reproduzir() && detail_pediu_do_inicio());
  assert(!admitirReproducaoDetalhe());
  assert(!pedReproduzir && !pedDoInicio);
  assert(avisos == 1 && !strcmp(aviso, "Carregando…"));
  token = 1; assert(episodioAlvo(&t, &e, &origem) && t == 1 && e == 13 && origem == 2);
  assert(!detail_pediu_reproduzir() && !detail_pediu_do_inicio());
  foco.fileira = SEC_EPISODIOS; foco.coluna = 0;
  assert(episodioAlvo(&t, &e, &origem) && t == 1 && e == 1 && origem == 1);
  foco.coluna = 1;
  assert(episodioAlvo(&t, &e, &origem) && t == 1 && e == 13 && origem == 2);
  assert(copias == 1); // Resultados de desenho sao copias possuidas por ticket.
  puts("ok  retomar refina TVDB na fonte final e episodio escolhido continua prioritario");
}

static void retomarAmbiguo(void) {
  int t, e, origem;
  limpar(); ep(0, 1, 13, 201); ep(1, 1, 14, 201);
  item.progresso = 25; item.temporada = 2; item.episodio = 1; item.tvdbEp = 201;
  assert(!episodioAlvo(&t, &e, &origem));
  pedReproduzir = 1; assert(detail_pediu_reproduzir() && !admitirReproducaoDetalhe());
  assert(!strcmp(aviso, "Escolha a temporada e o episódio e aperte OK"));
  limpar(); ep(0, 1, 13, 201); item.progresso = 25;
  item.temporada = 2; item.episodio = 1; item.tvdbEp = 201; trocaToken = 1;
  assert(!episodioAlvo(&t, &e, &origem));
  limpar(); ep(0, 1, 13, 202); item.progresso = 25;
  item.temporada = 2; item.episodio = 1; item.tvdbEp = 201;
  assert(!episodioAlvo(&t, &e, &origem));
  foco.fileira = SEC_EPISODIOS;
  assert(episodioAlvo(&t, &e, &origem) && t == 1 && e == 13 && origem == 1);
  pedReproduzir = pedDoInicio = 1;
  assert(detail_pediu_reproduzir() && detail_pediu_do_inicio());
  item.tvdbEp = 0; foco.fileira = 0;
  assert(episodioAlvo(&t, &e, &origem) && t == 2 && e == 1 && origem == 2);
  puts("ok  retomar pendente ausente ambiguo ou republicado aguarda prova sem bloquear escolha explicita");
}

static void primeiroDoCatalogo(void) {
  int t, e, origem;
  limpar(); ep(0, 1, 13, 201); ep(1, 1, 14, 202); progressoPronto = 1;
  vistoep_definir("tt512", 1, 13, 1);
  assert(episodioAlvo(&t, &e, &origem) && t == 1 && e == 14 && origem == 3);
  puts("ok  primeiro nao visto usa episodios da fonte e nao temporadas do Trakt");
}

static void rotaSemChute(void) {
  char alvo[64]; CatItem outro;
  limpar(); ep(0, 1, 13, 201); item.temporada = 2; item.episodio = 1;
  item.tvdbEp = 201; item.progresso = 25; token = 0;
  proximoT = 9; proximoE = 9; progressoPronto = 1;
  assert(detail_ep_aguardando()); idDoAlvo(&item, alvo, sizeof alvo); assert(!alvo[0]);
  outro = item; snprintf(outro.imdb, sizeof outro.imdb, "ttOutro");
  idDoAlvo(&outro, alvo, sizeof alvo); assert(!strcmp(alvo, "ttOutro"));
  aberto = 0; idDoAlvo(&item, alvo, sizeof alvo); assert(!strcmp(alvo, "tt512"));
  aberto = 1; foco.fileira = SEC_EPISODIOS;
  assert(!detail_ep_aguardando()); idDoAlvo(&item, alvo, sizeof alvo); assert(!strcmp(alvo, "tt512:1:13"));
  // Sem ID de episodio e sem lista, o pedido legado continua chegando ao app.
  limpar(); pedReproduzir = pedDoInicio = 1;
  assert(detail_pediu_reproduzir() && detail_pediu_do_inicio());
  puts("ok  roteador nao busca nem prepara par desconhecido e nao bloqueia outro titulo ou legado");
}

static void destinosDoRoteador(void) {
  int t, e;
  limpar(); ep(0, 1, 13, 201); ep(1, 1, 14, 202);
  item.temporada = 2; item.episodio = 1; item.tvdbEp = 201; item.progresso = 25;
  cwTocarT = 2; cwTocarE = 1; token = 0;
  assert(!admitirReproducaoDetalhe() && !cwTocarT && !cwTocarE);
  token = 1; cwTocarT = 2; cwTocarE = 1;
  assert(admitirReproducaoDetalhe()); episodioPedidoDetalhe(&t, &e);
  assert(t == 1 && e == 13); episodioDoDetalhe();
  assert(playerT == 1 && playerE == 13 && !cwTocarT && !cwTocarExplicito);
  item.progresso = 95; cwTocarT = 2; cwTocarE = 2;
  assert(admitirReproducaoDetalhe()); episodioPedidoDetalhe(&t, &e);
  assert(t == 1 && e == 14); episodioDoDetalhe(); assert(playerT == 1 && playerE == 14);
  // Agenda/retained-video choices carry their explicit pair, independent of CW.
  token = 0; assert(!gravarCwExplicito("ttOutro", 4, 7));
  assert(gravarCwExplicito("tt512", 4, 7));
  assert(admitirReproducaoDetalhe()); episodioDoDetalhe();
  assert(playerT == 4 && playerE == 7 && !cwTocarT && !cwTocarExplicito);
  gravarCwExplicito("tt512", 4, 7); perfil++;
  assert(!admitirReproducaoDetalhe() && !cwTocarExplicito);
  perfil--; gravarCwExplicito("tt512", 4, 7); strcpy(cwTocarId, "ttOutro");
  assert(!admitirReproducaoDetalhe() && !cwTocarExplicito);
  // A fresh manual focus cannot inherit the old automatic CW override.
  token = 1; foco.fileira = SEC_EPISODIOS; foco.coluna = 0;
  assert(admitirReproducaoDetalhe()); episodioDoDetalhe(); assert(playerT == 1 && playerE == 13);
  limpar(); cwTocarT = 2; cwTocarE = 8;
  assert(admitirReproducaoDetalhe()); episodioDoDetalhe(); assert(playerT == 2 && playerE == 8);
  puts("ok  preparo e player usam o mesmo alvo final preservando Agenda foco e CW sem ID");
}

static void refreshReiniciaUmaVez(void) {
  ExProgressoPedido antigo;
  limpar(); ep(0, 1, 13, 201); guardar(PROGRESSO); antigo = dono();
  extras_pedir_fonte("tt512", 1, 0, "addon:A"); assert(gerPedida == 1 && progressoPendente);
  trocarNaEntrada = 1; extras_atualizar_episodios();
  assert(credencial == 4 && !vistoep_n() && !progressoPronto);
  extras_pedir_fonte("tt512", 1, 0, "addon:A");
  assert(gerPedida == 2 && credencialPedido == 4 && !progressoPendente);
  progressoGuardar(&antigo, strdup(PROGRESSO)); assert(!progressoPendente);
  ids(IDENTIDADES); guardar(PROGRESSO); extras_atualizar_episodios();
  assert(progressoPronto && vistoep_estado("tt512", 1, 13) == 1 && vistoep_estado("tt512", 2, 1) == -1);
  extras_pedir_fonte("tt512", 1, 0, "addon:A"); assert(gerPedida == 2);
  puts("ok  refresh apos validacao bloqueia conta antiga e reinicia exatamente um pedido real");
}

static void folhaNaoHerdaFonte(void) {
  limpar(); ep(0, 1, 13, 201); item.tvdbEp = 201;
  item.temporada = 2; item.episodio = 1; item.progresso = 25; token = 0;
  strcpy(alvoDaLista, "ttAnterior:1:1"); assert(!fonteDoDetalheAtual(0, 0));
  strcpy(alvoDaLista, "tt512:1:13"); assert(fonteDoDetalheAtual(1, 13));
  assert(!fonteDoDetalheAtual(1, 14));
  token = 1; assert(fonteDoDetalheAtual(0, 0));
  aberto = 0; assert(!fonteDoDetalheAtual(1, 13));
  puts("ok  folha manual exige alvo atual e escolha explicita nao herda fonte anterior");
}

int main(void) {
  primeiroJoin(); colisaoMesmoNumero(); donoETicket(); ambiguidades(); legadoECache();
  manualPrevalece(); temporadasLongas(); idsAusentesEConflitantes(); cacheNaoCruzaConta(); retomarFinalEFoco();
  retomarAmbiguo(); primeiroDoCatalogo(); rotaSemChute(); destinosDoRoteador();
  refreshReiniciaUmaVez(); folhaNaoHerdaFonte(); limpar();
  puts("OK extras identidade: 16 grupos"); return 0;
}
