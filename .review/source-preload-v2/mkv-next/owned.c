#define assrender_preaquecer watchedPrewarm
#define assrender_limpar_fontes watchedClear
#define assrender_adicionar_fonte watchedFont
#define dados_gravar_leve watchedCacheWrite
#include "mkvass.c"
#undef assrender_preaquecer
#undef assrender_limpar_fontes
#undef assrender_adicionar_fonte
#undef dados_gravar_leve
#include <assert.h>

void assrender_preaquecer(void);
void assrender_limpar_fontes(void);
int assrender_adicionar_fonte(const char *, const void *, size_t);
int dados_gravar_leve(const char *, const char *);
static int renderOperations;
static int activeCacheWrites, totalCacheWrites;
int watchedCacheWrite(const char *name, const char *body) {
  assert(__atomic_add_fetch(&activeCacheWrites, 1, __ATOMIC_ACQ_REL) == 1);
  __atomic_add_fetch(&totalCacheWrites, 1, __ATOMIC_RELAXED);
  usleep(10000); // A reproducible slow filesystem overlap, outside owner locks.
  int ok = dados_gravar_leve(name, body);
  __atomic_sub_fetch(&activeCacheWrites, 1, __ATOMIC_ACQ_REL);
  return ok;
}
void watchedPrewarm(void) {
  __atomic_add_fetch(&renderOperations, 1, __ATOMIC_RELAXED); assrender_preaquecer();
}
void watchedClear(void) {
  __atomic_add_fetch(&renderOperations, 1, __ATOMIC_RELAXED); assrender_limpar_fontes();
}
int watchedFont(const char *name, const void *data, size_t length) {
  __atomic_add_fetch(&renderOperations, 1, __ATOMIC_RELAXED); return assrender_adicionar_fonte(name, data, length);
}

static const char *currentUrl = "https://example.test/current.mkv";
static const char *nextUrl = "https://example.test/next.mkv";
static unsigned char *files[2];
static long lengths[2];
static pthread_mutex_t transportTrava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t transportCond = PTHREAD_COND_INITIALIZER;
static int requests[2], blocked, entered;
static Off blockAt;

char *rede_baixar(const char *url, int s) { (void)url; (void)s; return NULL; }
char *rede_baixar_bin(const char *url, int s, long *n) { (void)url; (void)s; (void)n; return NULL; }
const char *rede_url_publica(const char *url, char *dst, unsigned tam) {
  (void)url; snprintf(dst, tam, "local"); return dst;
}
long rede_corte_host(const char *url) { (void)url; return 0; }
int rede_resto_recusado(void) { return 0; }
char *rede_baixar_trecho_st(const char *url, int prazo, long long ini, long long fim,
                          long *tam, int *status, int *erro, char *final, unsigned finalTam) {
  int index = !strcmp(url, currentUrl) ? 0 : 1;
  char *data;
  assert(!strcmp(url, index ? nextUrl : currentUrl));
  (void)prazo;
  pthread_mutex_lock(&transportTrava);
  requests[index]++;
  if (index && blocked && ini == blockAt) {
    entered = 1; pthread_cond_broadcast(&transportCond);
    while (blocked) pthread_cond_wait(&transportCond, &transportTrava);
  }
  pthread_mutex_unlock(&transportTrava);
  if (status) *status = 206;
  if (erro) *erro = 0;
  if (final && finalTam) snprintf(final, finalTam, "%s", url);
  *tam = 0;
  if (ini >= lengths[index]) return NULL;
  if (fim >= lengths[index]) fim = lengths[index] - 1;
  *tam = fim - ini + 1;
  data = calloc(1, (size_t)*tam + 1); assert(data);
  memcpy(data, files[index] + ini, (size_t)*tam);
  return data;
}

static void load(const char *path, int index) {
  FILE *f = fopen(path, "rb"); assert(f);
  assert(!fseek(f, 0, SEEK_END)); lengths[index] = ftell(f); rewind(f);
  files[index] = malloc((size_t)lengths[index]); assert(files[index]);
  assert(fread(files[index], 1, (size_t)lengths[index], f) == (size_t)lengths[index]); fclose(f);
}
static int choose(const char *const *languages, int n, const void *context) {
  assert(context && !strcmp(context, "eng"));
  assert(n == 1 && !strcmp(languages[0], "eng"));
  return 0;
}
static int ordinaryChoices;
static int chooseNone(const char *const *languages, int n) {
  assert(n == 1 && !strcmp(languages[0], "eng"));
  ordinaryChoices++;
  return -1;
}
static int count(int index) {
  int n; pthread_mutex_lock(&transportTrava); n = requests[index]; pthread_mutex_unlock(&transportTrava); return n;
}
static void waitEntered(void) {
  struct timespec deadline; assert(!clock_gettime(CLOCK_REALTIME, &deadline)); deadline.tv_sec += 5;
  pthread_mutex_lock(&transportTrava);
  while (!entered) assert(!pthread_cond_timedwait(&transportCond, &transportTrava, &deadline));
  pthread_mutex_unlock(&transportTrava);
}
static void currentText(void) {
  char text[256];
  assert(legenda_texto(2.0, 0, text, sizeof text));
  assert(strstr(text, "Current only") && !strstr(text, "Next only"));
}
static void drain(MkvassPreparo *s) {
  int live = 1;
  for (int i = 0; i < 1000 && live; i++) {
    pthread_mutex_lock(&s->trava); live = s->vivos; pthread_mutex_unlock(&s->trava);
    if (live) usleep(5000);
  }
  assert(!live);
}
static MkvassPreparo *prepare(void) {
  char language[16] = "eng";
  MkvassPreparo *s = mkvass_preparo_criar(nextUrl, choose, language, sizeof language, 0.0);
  assert(s); strcpy(language, "changed");
  for (int i = 0; i < 1500 && mkvass_preparo_fase(s) == 1; i++) usleep(5000);
  assert(mkvass_preparo_fase(s) == 2);
  pthread_mutex_lock(&s->trava);
  assert(s->nColhidos == 1 && s->vivos == 1 && s->prebusca && s->prebuscaOrdinal == 0);
  pthread_mutex_unlock(&s->trava);
  return s;
}
static void *cacheWriter(void *owner) {
  for (int i = 0; i < 4; i++) gravarSidecar(owner, 1, 0);
  return NULL;
}
static void cacheRace(void) {
  MkvassPreparo states[2] = {0};
  Fio writers[2] = {0}, read = {0};
  FonteMkv fonts[2] = {0};
  Ponto point = { .colhido = 1 };
  const char *bodies[] = { "Owner A", "Owner B" };
  unsigned char bytes[] = {'A', 'B'};
  pthread_t threads[2];
  for (int i = 0; i < 2; i++) {
    assert(!pthread_mutex_init(&states[i].trava, NULL)); states[i].geracao = 1;
    writers[i].dono = &states[i]; writers[i].g = 1;
    writers[i].corpo = (char *)bodies[i]; writers[i].corpoTam = strlen(bodies[i]);
    writers[i].fontesCompletas = 1; writers[i].nFontes = 1; writers[i].fontes = &fonts[i];
    writers[i].nPontos = writers[i].nColhidos = 1; writers[i].pontos = &point;
    fonts[i].nome = "fixture.ttf"; fonts[i].dados = &bytes[i]; fonts[i].tam = 1;
    strcpy(writers[i].sidecar, "ownership-race.ass");
    strcpy(writers[i].sidecarFontes, "ownership-race.ass.fonts");
    assert(!pthread_create(&threads[i], NULL, cacheWriter, &writers[i]));
  }
  for (int i = 0; i < 2; i++) assert(!pthread_join(threads[i], NULL));
  strcpy(read.sidecar, "ownership-race.ass"); strcpy(read.sidecarFontes, "ownership-race.ass.fonts");
  int validFonts = 0;
  char *cache = lerSidecar(&read, &validFonts);
  assert(cache && sidecarCompleto(cache) && validFonts && read.nFontes == 1);
  int lastOwner = strstr(cache, "Owner A") ? 0 : 1;
  assert(strstr(cache, bodies[lastOwner]) && read.fontes[0].dados[0] == bytes[lastOwner]);
  int writesBeforeCleanup = totalCacheWrites;
  states[0].geracao = 2; // stale cancelled owner cleanup may still save a partial
  gravarSidecar(&writers[0], 0, 1);
  char *after = lerSidecar(&read, &validFonts);
  assert(after && !strcmp(cache, after) && validFonts && totalCacheWrites == writesBeforeCleanup);
  free(cache); free(after); liberarFontes(&read);
  for (int i = 0; i < 2; i++) pthread_mutex_destroy(&states[i].trava);
  puts("PASS simultaneous same-URL owners serialize real font/body files; late cancelled partial cannot downgrade complete cache");
}

int main(int argc, char **argv) {
  unsigned generation;
  char headerUrl[4096];
  long currentRequests;
  MkvassPreparo *future, *hold;
  assert(argc == 3); load(argv[1], 0); load(argv[2], 1); dados_iniciar(".");
  mkvass_iniciar_ordinal(currentUrl, 0);
  MkvassPreparo *current = reterAtivo();
  for (int i = 0; i < 1500; i++) {
    int collected = 0; mkvass_estatisticas(NULL, NULL, &collected, NULL);
    if (collected) break; usleep(5000);
  }
  currentText(); assert(mkvass_ocupado()); mkvass_video_aberto(1);
  generation = legenda_geracao(); currentRequests = count(0);
  int initialRenderOperations = __atomic_load_n(&renderOperations, __ATOMIC_RELAXED);
  strcpy(headerUrl, S.url);
  future = prepare();
  assert(S.videoAberto && !strcmp(S.url, headerUrl));
  assert(legenda_geracao() == generation && count(0) == currentRequests); currentText();
  assert(__atomic_load_n(&renderOperations, __ATOMIC_RELAXED) == initialRenderOperations);
  Fio synthetic = { .dono = future, .prebusca = 1 };
  assert(!foraDoTeto(&synthetic));
  assert(!mkvass_preparo_adotar(&future, nextUrl, 0.0)); assert(future); currentText();
  mkvass_video_aberto(0);
  assert(!mkvass_preparo_adotar(&future, currentUrl, 0.0));
  assert(!mkvass_preparo_adotar(&future, nextUrl, .5));
  assert(future && !strcmp(S.url, currentUrl)); currentText();
  mkvass_video_aberto(1);
  hold = reterEstado(future); mkvass_preparo_cancelar(&future); assert(!future); drain(hold);
  assert(__atomic_load_n(&hold->referencias, __ATOMIC_ACQUIRE) == 1); soltarEstado(hold);
  assert(legenda_geracao() == generation); currentText();
  puts("PASS current ASS continuity, copied language, bounded parser and shared throttle; URL/resume/active-video rejects preserve ownership");

  pthread_mutex_lock(&transportTrava); blocked = 1; entered = 0; blockAt = 0; pthread_mutex_unlock(&transportTrava);
  future = mkvass_preparo_criar(nextUrl, choose, "eng", 4, 0.0); assert(future);
  hold = reterEstado(future);
  waitEntered();
  long since = agoraMs(); mkvass_preparo_cancelar(&future);
  assert(!future && agoraMs() - since < 50); currentText();
  pthread_mutex_lock(&transportTrava); blocked = 0; pthread_cond_broadcast(&transportCond); pthread_mutex_unlock(&transportTrava);
  drain(hold); assert(hold->referencias == 1); soltarEstado(hold);
  puts("PASS cancel during blocked HTTP is nonblocking; detached worker retains owner until response returns");

  future = prepare(); hold = reterEstado(future);
  pthread_mutex_lock(&transportTrava); blocked = 1; entered = 0; blockAt = 777; pthread_mutex_unlock(&transportTrava);
  synthetic.dono = future; synthetic.g = future->geracao;
  snprintf(synthetic.url, sizeof synthetic.url, "%s", nextUrl);
  Job *job = submeter(&synthetic, 777, 32); assert(job && job->dono == future);
  waitEntered();
  since = agoraMs(); mkvass_preparo_cancelar(&future);
  assert(!future && agoraMs() - since < 50); currentText();
  pthread_mutex_lock(&transportTrava); blocked = 0; pthread_cond_broadcast(&transportCond); pthread_mutex_unlock(&transportTrava);
  long bytes = 0; assert(!colherJobDe(&synthetic, job, &bytes, 0) && bytes == 0);
  drain(hold); assert(hold->referencias == 1); soltarEstado(hold);
  puts("PASS pooled Range retains future owner; cancelled bytes discarded without current accounting/state mutation");

  future = prepare(); hold = reterEstado(future);
  int readyRequests = count(1);
  mkvass_video_aberto(0); mkvass_parar(); legenda_desligar();
  assert(mkvass_preparo_adotar(&future, nextUrl, 0.0) && !future);
  assert(!mkvass_preparo_adotar(&future, nextUrl, 0.0));
  assert(!strcmp(S.url, nextUrl) && mkvass_prebusca_fase() == 2);
  unsigned char *header = NULL; long headerN = 0;
  assert(mkvass_cabecalho(nextUrl, &header, &headerN) && headerN > 0); free(header);
  mkvass_iniciar_ordinal(nextUrl, 0);
  for (int i = 0; i < 1500; i++) {
    char text[256];
    if (legenda_texto(2.0, 0, text, sizeof text) && strstr(text, "Next only")) break;
    usleep(5000);
  }
  char text[256]; assert(legenda_texto(2.0, 0, text, sizeof text) && strstr(text, "Next only"));
  assert(count(1) == readyRequests);
  mkvass_video_aberto(1); mkvass_parar(); mkvass_video_aberto(0); drain(hold); soltarEstado(hold);
  drain(current); soltarEstado(current);
  puts("PASS consume-once Next adoption retains exact worker/header/body; first subtitle arrives without new Range");
  assert(mkvass_prebuscar(currentUrl, chooseNone, .5));
  for (int i = 0; i < 1500 && mkvass_prebusca_fase() == 1; i++) usleep(5000);
  assert(mkvass_prebusca_fase() == 2 && ordinaryChoices == 1);
  hold = reterAtivo(); drain(hold); soltarEstado(hold);
  puts("PASS later ordinary preparation uses its current callback, not the adopted future's captured language");
  cacheRace();
  free(files[0]); free(files[1]);
  puts("PASS owned MKV future preparation");
  return 0;
}
