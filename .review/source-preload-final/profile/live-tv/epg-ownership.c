#include "support.h"
#include "dados.h"
#include "epg.c"
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int block, entered, release_request, private_calls;
static char xmlA[600], xmlB[600];
char *rede_baixar_bin(const char *url, int sec, long *n) { (void)url; (void)sec; (void)n; return NULL; }
char *rede_baixar_bin_medido_controle(const char *url, int sec, const char *const *h,
  const RedeControle *ctl, long *n, RedeMedida *med) {
  (void)sec; (void)h; (void)ctl;
  if (med) { memset(med, 0, sizeof *med); med->status = 200; }
  if (strstr(url, "epgshare01")) return NULL;
  pthread_mutex_lock(&m); private_calls++;
  if (block) { entered = 1; pthread_cond_broadcast(&cv); while (!release_request) pthread_cond_wait(&cv, &m); }
  pthread_mutex_unlock(&m);
  const char *xml = strstr(url, "a.example") ? xmlA : xmlB;
  *n = (long)strlen(xml); return strdup(xml);
}
static void make_xml(char *out, const char *id, const char *title) {
  char start[40], end[40]; time_t a = time(NULL) - 60, b = a + 3600;
  strftime(start, sizeof start, "%Y%m%d%H%M%S +0000", gmtime(&a));
  strftime(end, sizeof end, "%Y%m%d%H%M%S +0000", gmtime(&b));
  snprintf(out, 600, "<tv><channel id=\"%s\"><display-name>%s</display-name></channel><programme channel=\"%s\" start=\"%s\" stop=\"%s\"><title>%s</title></programme></tv>", id, id, id, start, end, title);
}
static void wait_entered(void) {
  pthread_mutex_lock(&m); while (!entered) pthread_cond_wait(&cv, &m); pthread_mutex_unlock(&m);
}
static void release(void) {
  pthread_mutex_lock(&m); release_request = 1; block = 0; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
}
static void wait_done(void) {
  for (int i = 0; i < 2000 && !atomic_load_explicit(&pendPronto, memory_order_acquire); i++) usleep(1000);
  assert(atomic_load_explicit(&pendPronto, memory_order_acquire));
}
static void drain(void) {
  for (int i = 0; i < 2000; i++) { epg_passo(); if (!fioVivo && epg_estado() == EPG_PRONTO) return; usleep(1000); }
  assert(!"EPG did not finish");
}
int main(void) {
  fixture_init(); make_xml(xmlA, "Channel.A", "Private A programme"); make_xml(xmlB, "Channel.B", "Private B programme");
  epg_abrir_sessao(1); epg_fonte_extra("https://a.example/xmltv?private_A");
  block = 1; epg_iniciar(); wait_entered(); uint64_t old = carga.sessao;
  perfis_definir_ativo(2); epg_abrir_sessao(2);
  assert(epg_estado() == EPG_PARADO && P.nCanais == 0);
  epg_fonte_extra("https://b.example/xmltv?private_B"); epg_iniciar(); assert(reiniciarPend);
  release(); wait_done(); assert(!perfis_sessao_atual(old));
  char *file = dados_ler("epg-extra-p1.bin"); assert(file && strstr(file, "Private A programme")); free(file);
  assert(!dados_ler("epg-extra-p2.bin"));
  drain(); assert(epg_match_id("Channel.A") < 0 && epg_match_id("Channel.B") >= 0);
  file = dados_ler("epg-extra-p2.bin"); assert(file && strstr(file, "Private B programme") && !strstr(file, "Private A programme")); free(file);
  puts("PASS: late A EPG writes only A cache; B publishes only its captured source");
  perfis_definir_ativo(1); epg_abrir_sessao(1); epg_fonte_extra("https://a.example/xmltv?private_A");
  int before = private_calls; epg_iniciar(); drain(); assert(private_calls == before); assert(epg_match_id("Channel.A") >= 0 && epg_match_id("Channel.B") < 0);
  puts("PASS: returning to A reuses its own disk cache without another request");
  // A response that finished before a switch still belongs to the old opening.
  dados_apagar("epg-extra-p1.ts"); block = 1; entered = release_request = 0; epg_iniciar(); wait_entered(); old = carga.sessao;
  perfis_definir_ativo(2); epg_abrir_sessao(2); perfis_definir_ativo(1); epg_abrir_sessao(1);
  release(); wait_done(); epg_passo(); assert(!perfis_sessao_atual(old)); assert(P.nCanais == 0 && epg_estado() == EPG_PARADO);
  puts("PASS: A -> B -> A cannot revive a previous opening’s EPG worker");
  epg_teste_limpar(); fixture_cleanup();
}
