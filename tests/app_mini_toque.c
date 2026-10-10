/* Actual app eligibility, including overlays that open after Mini drawing. */
#define main sidebar_fixture_main
#include "app_sidebar_toque.c"
#undef main
enum { BLOQ_MENU, BLOQ_AVISOS, BLOQ_STREAMS, BLOQ_FAIXAS, BLOQ_EPISODIOS,
 BLOQ_GUIA, BLOQ_SINTRO, BLOQ_NOVIDADES, BLOQ_TELEMETRIA, BLOQ_RECINTRO,
 BLOQ_UPDATE, BLOQ_LEMBRETE, BLOQ_CRASH, BLOQ_GLEM, BLOQ_ENVIO, BLOQ_PESSOAS,
 BLOQ_RECOMENDA, BLOQ_PIPINTRO, BLOQ_AJUSTES, BLOQ_AGENDA, BLOQ_DIAGLOCAL,
 BLOQ_OUTROS_N };
static int outros[BLOQ_OUTROS_N];
#define BLOQUEIO(fn, b) int fn(void) { return outros[b]; }
BLOQUEIO(avisos_aberto,BLOQ_AVISOS)
BLOQUEIO(stream_folha_aberta,BLOQ_STREAMS)
BLOQUEIO(faixas_aberta,BLOQ_FAIXAS)
BLOQUEIO(episodios_aberto,BLOQ_EPISODIOS)
BLOQUEIO(guia_overlay_aberta,BLOQ_GUIA)
BLOQUEIO(sintro_aberto,BLOQ_SINTRO)
BLOQUEIO(novidades_aberto,BLOQ_NOVIDADES)
BLOQUEIO(telemetria_aberto,BLOQ_TELEMETRIA)
BLOQUEIO(recintro_aberto,BLOQ_RECINTRO)
BLOQUEIO(atualizacao_aberta,BLOQ_UPDATE)
BLOQUEIO(agendaviso_aberto,BLOQ_LEMBRETE)
BLOQUEIO(avisos_cartao_aberto,BLOQ_CRASH)
BLOQUEIO(glem_cartao_aberto,BLOQ_GLEM)
BLOQUEIO(recenviar_aberto,BLOQ_ENVIO)
BLOQUEIO(pessoas_aberto,BLOQ_PESSOAS)
BLOQUEIO(recomenda_aberta,BLOQ_RECOMENDA)
BLOQUEIO(pipintro_aberto,BLOQ_PIPINTRO)
BLOQUEIO(agendaui_menu_aberto,BLOQ_AGENDA)
BLOQUEIO(diagnostico_apresentacao_aberta,BLOQ_DIAGLOCAL)
int ajustes_relogio_cabe(void) { return !outros[BLOQ_AJUSTES]; }
#define INTRO_INERTO(n) int novidades##n##_aberto(void) { return 0; }
INTRO_INERTO(11) INTRO_INERTO(12) INTRO_INERTO(13) INTRO_INERTO(131)
INTRO_INERTO(132) INTRO_INERTO(133) INTRO_INERTO(134) INTRO_INERTO(139)
INTRO_INERTO(1312) INTRO_INERTO(142) INTRO_INERTO(148) INTRO_INERTO(170) INTRO_INERTO(180)
static void inicio(Tela t) { reiniciar(t);mini=1;memset(outros,0,sizeof outros); }
int main(void) {
  const Tela pages[]={TELA_HOME,TELA_EXPLORAR,TELA_BUSCA,TELA_BIBLIOTECA,
    TELA_PERFIL,TELA_AJUSTES,TELA_DIAGNOSTICO,TELA_SOCIAL,TELA_ADDONS,
    TELA_AGENDA,TELA_LIVETV_DIAG,TELA_PLUGINS};
  int checks=0;
  for(unsigned p=0;p<sizeof pages/sizeof *pages;p++) {
    inicio(pages[p]);assert(miniToquePermitido());
    assert(playerToquePermitido());
    for(int b=0;b<TESTE_BLOQUEIOS_N;b++) {
      inicio(pages[p]);bloqueios[b]=1;assert(!miniToquePermitido());checks++;
      assert(!playerToquePermitido());
    }
    for(int b=0;b<BLOQ_OUTROS_N;b++) {
      inicio(pages[p]);outros[b]=1;
      if(b==BLOQ_MENU)menu=1;
      int expected=b==BLOQ_AJUSTES?pages[p]==TELA_AJUSTES:
        b==BLOQ_AGENDA?pages[p]==TELA_AGENDA:
        b==BLOQ_DIAGLOCAL?pages[p]==TELA_DIAGNOSTICO:1;
      assert(miniToquePermitido()==!expected);checks++;
      assert(playerToquePermitido()==!expected);
    }
    inicio(pages[p]);contexto=1;assert(!miniToquePermitido());
    inicio(pages[p]);player=1;assert(!miniToquePermitido());
    assert(playerToquePermitido());
    inicio(pages[p]);loginOk=0;assert(!miniToquePermitido());
    inicio(pages[p]);perfilOk=0;assert(!miniToquePermitido());
    /* Detail and See-all are regular browsing layers, not blocking modals. */
    inicio(pages[p]);detalhe=1;assert(miniToquePermitido());
    inicio(pages[p]);lista=1;assert(miniToquePermitido());
  }
  inicio(TELA_GUIA);assert(!miniToquePermitido());
  assert(playerToquePermitido());
  inicio(TELA_PLAYER);player=1;assert(playerToquePermitido());
  for(int b=0;b<TESTE_BLOQUEIOS_N;b++) {
    inicio(TELA_PLAYER);player=1;bloqueios[b]=1;assert(!playerToquePermitido());
  }
  for(int b=0;b<BLOQ_OUTROS_N;b++) {
    inicio(TELA_PLAYER);player=1;outros[b]=1;if(b==BLOQ_MENU)menu=1;
    int expected=b!=BLOQ_AJUSTES&&b!=BLOQ_AGENDA&&b!=BLOQ_DIAGLOCAL;
    assert(playerToquePermitido()==!expected);
  }
  printf("app_mini_toque: %d current overlay checks, browsing/detail/See-all and Guide priority PASS\n",checks);
}
