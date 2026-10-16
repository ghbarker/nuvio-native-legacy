// Actual player.c handoff bodies; Android/media operations are explicit doubles.
#include "playerexterno.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct { char titulo[256]; } CatItem;
static CatItem titulo = { "Title \xf0\x9f\x8e\xac" };
static PlayerExterno externo;
static double retomarFonteSeg, retomarSeg;
static char prebuscaUrl[4096];
static int semRetomada, comVideo, esperandoFonte, saindo, pediuSair, canal;
static uint64_t perfil;
static char cab[2048], abriuUrl[4096], abriuCab[2048];
static PlayerExternoFonte capturada;
static int eventos[32], nEventos, iEvento, abrirInterno, avisos, cancelou;
static uint64_t tokens[32], cancelToken;
static int pedirAceita, abrirAceita, pediu, lancou, passos[64], nPassos, hostOcupado;
static void passo(int n) { assert(nPassos < 64); passos[nPassos++] = n; }
static const char *i18n(const char *s) { return s; }
static const CatItem *item(void) { return &titulo; }
static int ehCanal(void) { return canal; }
static uint64_t nv_perfil_sessao_ler(void) { return perfil; }
static int nv_perfil_sessao_atual(uint64_t p) { return p == perfil; }
static const char *video_cabecalhos(void) { return cab; }
static int android_player_externo_ocupado(void) { return hostOcupado; }
static void video_definir_cabecalhos(const char *c) { snprintf(cab,sizeof cab,"%s",c); }
static void proximopreparo_cancelar(void) { passo(1); }
static void player_precarregar_cancelar(unsigned n) { assert(n==0); passo(2); }
static void mkvass_parar(void) { assert(!prebuscaUrl[0]); passo(4); }
static void video_parar(void) { passo(5); }
static void mkvass_video_aberto(int v) { assert(v==0); passo(6); }
static void android_player_externo_cancelar(uint64_t t) { cancelou++; cancelToken=t; }
static void android_player_externo_aviso(const char *s) { assert(s&&*s); avisos++; }
static int android_player_externo_pedir(uint64_t t,int modo,const char *u,const char *c,
                                      const char *nome,int64_t pos,const char *const r[7]) {
  assert(t==externo.token&&modo>=1&&modo<=2);
  for(int i=0;i<7;i++) assert(r[i]&&*r[i]);
  assert(!strcmp(nome,titulo.titulo));
  snprintf(capturada.url,sizeof capturada.url,"%s",u);
  snprintf(capturada.cabecalhos,sizeof capturada.cabecalhos,"%s",c);
  capturada.posicaoMs=pos; pediu++; passo(7); return pedirAceita;
}
static int android_player_externo_abrir(uint64_t t) { assert(t==externo.token); lancou++; passo(8); return abrirAceita; }
static int android_player_externo_evento(uint64_t *t,int *e) {
  if(iEvento==nEventos)return 0;
  *t=tokens[iEvento];*e=eventos[iEvento++];return 1;
}
static void player_definir_fonte(const char *u) {
  assert(externo.decisao==PEX_INTERNO&&!playerexterno_ativo(&externo));
  abrirInterno++; snprintf(abriuUrl,sizeof abriuUrl,"%s",u);snprintf(abriuCab,sizeof abriuCab,"%s",cab);
}
#include "playerexterno_native.inc"

static void iniciar(int modo) {
  memset(&externo,0,sizeof externo);perfil=41;
  playerexterno_iniciar(&externo,modo,perfil);
  retomarFonteSeg=-1;retomarSeg=37.5;semRetomada=canal=0;comVideo=1;esperandoFonte=1;
  saindo=pediuSair=0;abrirInterno=avisos=cancelou=pediu=lancou=nPassos=nEventos=iEvento=0;
  pedirAceita=abrirAceita=1;cancelToken=0;hostOcupado=0;
  snprintf(cab,sizeof cab,"Authorization: Bearer fixture\nReferer: https://catalog.invalid/");
  memset(&capturada,0,sizeof capturada);abriuUrl[0]=abriuCab[0]=0;
  snprintf(prebuscaUrl,sizeof prebuscaUrl,"https://old.pending.invalid/subtitle.mkv");
}
static void evento(uint64_t t,int e) { assert(nEventos<32);tokens[nEventos]=t;eventos[nEventos++]=e; }
static void recursosAntes(int final) {
  const int esperados[]={1,2,4,5,6};
  assert(nPassos==6);for(int i=0;i<5;i++)assert(passos[i]==esperados[i]);assert(passos[5]==final);
  assert(!prebuscaUrl[0]);
  assert(!comVideo&&!esperandoFonte);
}
int main(void) {
  const char *url="https://stream.invalid/video.mkv?sig=a%2Bb%26c";
  iniciar(PEX_INTERNO);assert(!externoPedir(url)&&!pediu&&!nPassos&&comVideo);
  hostOcupado=1;assert(externoPedir(url)&&pediuSair&&avisos==1&&!pediu&&!nPassos);
  puts("ok external native: default internal keeps existing pipeline");

  iniciar(PEX_PERGUNTAR);assert(externoPedir(url));assert(pediu==1&&nPassos==1&&passos[0]==7&&comVideo);
  assert(capturada.posicaoMs==37500&&!strcmp(capturada.url,url));
  snprintf(cab,sizeof cab,"new mutable headers");
  evento(externo.token,PEX_EV_INTERNO);assert(externoPasso());
  assert(abrirInterno==1&&!strcmp(abriuUrl,url)&&!strcmp(abriuCab,capturada.cabecalhos));
  assert(!playerexterno_ativo(&externo)&&!saindo&&!externoPedir(url)&&pediu==1);
  puts("ok external native: ask internal restores owned source once without releasing warmup");

  iniciar(PEX_PERGUNTAR);assert(externoPedir(url));nPassos=0;
  uint64_t t=externo.token;evento(t,PEX_EV_EXTERNO);assert(externoPasso());recursosAntes(8);assert(lancou==1&&!abrirInterno);
  evento(t,PEX_EV_EXTERNO);evento(t,PEX_EV_ABRIU);evento(t,PEX_EV_ABRIU);evento(t,PEX_EV_INTERNO);
  assert(externoPasso()&&externo.fase==PEX_ABERTO&&lancou==1&&!abrirInterno&&!pediuSair);
  evento(t,PEX_EV_VOLTOU);assert(externoPasso()&&pediuSair&&saindo&&!abrirInterno);
  assert(cancelou==1&&cancelToken==t&&!playerexterno_ativo(&externo));
  puts("ok external native: external releases both preloads before launch and return exits without internal playback");

  iniciar(PEX_EXTERNO);assert(externoPedir(url));recursosAntes(7);
  assert(externoPedir("https://other.invalid/new")&&pediu==1&&!strcmp(capturada.url,url));
  t=externo.token;evento(t-1,PEX_EV_CANCELADO);assert(externoPasso()&&playerexterno_ativo(&externo));
  perfil++;assert(externoPasso()&&pediuSair&&cancelToken==t&&!abrirInterno);
  puts("ok external native: late sources and stale callbacks cannot replace owner or cross profile");

  iniciar(PEX_EXTERNO);assert(externoPedir(url));snprintf(cab,sizeof cab,"changed");
  evento(externo.token,PEX_EV_INTERNO);assert(externoPasso()&&abrirInterno==1&&!pediuSair);
  assert(!strcmp(abriuCab,capturada.cabecalhos));
  iniciar(PEX_EXTERNO);pedirAceita=0;assert(externoPedir(url)&&pediuSair&&avisos==1&&!abrirInterno);
  iniciar(PEX_PERGUNTAR);assert(externoPedir(url));abrirAceita=0;evento(externo.token,PEX_EV_EXTERNO);
  assert(externoPasso()&&pediuSair&&avisos==1&&!abrirInterno);
  puts("ok external native: explicit fallback resumes internal, bridge refusal never silently falls back");

  iniciar(PEX_EXTERNO);retomarFonteSeg=12.25;assert(externoPedir(url)&&capturada.posicaoMs==12250);
  iniciar(PEX_EXTERNO);semRetomada=1;assert(externoPedir(url)&&!capturada.posicaoMs);
  iniciar(PEX_EXTERNO);canal=1;assert(externoPedir(url)&&!capturada.posicaoMs);
  iniciar(PEX_EXTERNO);assert(externoPedir("file:///not-shared")&&pediuSair&&!pediu&&avisos==1);
  iniciar(PEX_EXTERNO);perfil++;assert(externoPedir(url)&&pediuSair&&!pediu&&!nPassos);
  puts("ok external native: resume uses actual seconds and invalid media or profile refuses before dispatch");
  return 0;
}
