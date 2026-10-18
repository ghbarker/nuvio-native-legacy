// Real guide search; deterministic EPG and clock, without a window or network.
#include "../src/guia.c"
#include <assert.h>
static Uint32 tick, custo;
static int chamadas, paginasPorCanal=1, semResultado;
static unsigned revisao;
Uint32 SDL_GetTicks(void) { return tick; }
unsigned epg_revisao(void) { return revisao; }
int epg_canal_estado(const char *id) { (void)id;return EPG_PRONTO; }
int epg_match_canal(const char *id,const char *epgId,const char *nome) { (void)id;(void)epgId;(void)nome;return 0; }
int xtream_e_id(const char *id) { (void)id;return 0; }
void xtepg_querer(const char *id) { (void)id; }
int xtepg_faixa_desde(const char *id,time_t de,time_t ate,int pular,EpgProg *out,int cap) {
  (void)id;(void)de;(void)ate;(void)pular;(void)out;(void)cap;assert(0);return 0;
}
int epg_faixa_desde(int canal,time_t de,time_t ate,int pular,EpgProg *out,int cap) {
  (void)ate;chamadas++;tick+=custo;
  int n=paginasPorCanal==1?1:(pular<24*(paginasPorCanal-1)?24:1);
  if(n>cap)n=cap;
  for(int i=0;i<n;i++) out[i]=(EpgProg){.ini=de+(canal%2?300:-100),.fim=de+600,
      .titulo=semResultado?"Other programme":"Jaguars"};
  return n;
}
static void semear(int n) {
  assert(canaisReservar(n));memset(canais,0,(size_t)n*sizeof *canais);nCanais=n;
  for(int i=0;i<n;i++) { canais[i].epg=i;snprintf(canais[i].nome,sizeof canais[i].nome,"Channel %d",i+1); }
  strcpy(canais[0].nome,"Jaguars channel");canaisRevisao++;chamadas=0;tick=0;
}
static void concluir(void) { int passos=0;while(buscaPendente.ativa){assert(++passos<1000);buscaPasso();} }
int main(void) {
  // No scan in submit; yielding after one slow page keeps the UI available.
  semear(10);custo=5;buscaFazer(" Jaguars ");
  assert(buscaNC==1 && buscaNP==0 && chamadas==0 && buscaPendente.ativa);
  buscaPasso();assert(chamadas==1 && buscaNP==0 && buscaPendente.ativa);
  concluir();assert(buscaNP==10);
  for(int i=0;i<5;i++)assert(buscaProg[i].ini<=buscaPendente.de);
  for(int i=5;i<10;i++)assert(buscaProg[i].ini>buscaPendente.de);
  // Pagination yields within a channel, preserving the positional cursor.
  semear(1);paginasPorCanal=2;buscaFazer("Jaguars");buscaPasso();
  assert(buscaPendente.vistos==24 && buscaPendente.canal==0 && buscaNP==0);
  concluir();assert(buscaNP==25 && chamadas==2);paginasPorCanal=1;
  // A coarse/frozen clock still yields before scanning a large provider.
  semear(21202);custo=0;semResultado=1;buscaFazer("Jaguars");buscaPasso();
  assert(chamadas==256 && buscaPendente.ativa && buscaNP==0);
  buscaFechar();int antes=chamadas;buscaPasso();assert(chamadas==antes);
  // Starting a new query discards the old staged programmes.
  semResultado=0;buscaFazer("Jaguars");buscaPasso();assert(buscaNP==40);
  buscaFazer("missing");assert(buscaNC==0 && buscaNP==0 && buscaPendente.n==0);
  buscaFechar();
  // Channel-number lookup does not scan programme schedules.
  chamadas=0;buscaFazer("2");assert(buscaNC==G_BUSCA_CANAIS && buscaCanal[0]==1 && !buscaPendente.ativa && !chamadas);
  // Restarting the saved query must not overwrite its own input buffer.
  buscaFazer("Jaguars");buscaFazer(buscaTexto);assert(!strcmp(buscaTexto,"Jaguars") && buscaNC==1);
  free(canais);free(ordemCanais);
  puts("PASS: bounded search, pagination, cancellation, replacement, ordering, result cap and channel numbers");
}
