#include "esportes.h"
#include "js.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const EspLiga liga = {"nba", "NBA", ""};
static const char *jogo = "{\"events\":[{\"id\":\"10\",\"date\":\"2026-10-19T20:00Z\",\"name\":\"Boston Celtics at Orlando Magic\",\"competitions\":[{\"id\":\"10\",\"date\":\"2026-10-19T20:00Z\",\"status\":{\"type\":{\"state\":\"in\",\"shortDetail\":\"Q2\"}},\"venue\":{\"fullName\":\"Arena\"},\"competitors\":[{\"score\":\"80\",\"team\":{\"id\":\"1\",\"displayName\":\"Boston Celtics\",\"shortDisplayName\":\"Celtics\",\"abbreviation\":\"BOS\"}},{\"score\":\"70\",\"team\":{\"id\":\"2\",\"displayName\":\"Orlando Magic\",\"shortDisplayName\":\"Magic\",\"abbreviation\":\"ORL\"}}],\"broadcasts\":[{\"names\":[\"ESPN\",\"ABC\"]}]}]}]}";
int main(void) {
  EspEvento e[5]; EspLiga ls[3];
  assert(esportes_modalidades_n() == 16);
  assert(!esportes_modalidade(-1)); assert(!esportes_modalidade(16));
  assert(esportes_ligas_ler("{\"leagues\":[{\"slug\":\"eng.1\",\"name\":\"Premier League\"},{\"slug\":\"../bad\",\"name\":\"Bad\"},{\"slug\":\"nba\",\"name\":\"NBA\"}]}",ls,3)==2);
  assert(!strcmp(ls[1].id,"nba"));
  memset(e,0,sizeof e); assert(esportes_eventos_ler(jogo,"basketball",&liga,e,5)==1);
  assert(e[0].nParticipantes==2 && e[0].estado==1);
  assert(!strcmp(e[0].participantes[0].pontos,"80"));
  assert(!strcmp(e[0].local,"Arena")); assert(e[0].nEmissoras==2);
  time_t t=(time_t)(js_ms_iso("2026-10-19T20:00Z")/1000);
  assert(esportes_corresponde(e,"NBA: Celtics vs Magic",t-1800,t+10800)==100);
  assert(esportes_corresponde(e,"BOS @ ORL",t,t+10800)==100);
  assert(!esportes_corresponde(e,"Celtics vs Cavaliers",t,t+10800));
  assert(!esportes_corresponde(e,"NBA Basketball",t,t+10800));
  assert(!esportes_corresponde(e,"Celtics vs Magic",t-86400,t-80000));
  assert(!esportes_corresponde(e,"Celtics vs Magic Highlights",t,t+10800));
  assert(!esportes_corresponde(e,"BOSTON ORLANDO",t,t+10800));
  assert(!esportes_corresponde(e,"BOSON ORLANDO",t,t+10800));
  assert(esportes_emissora(e,"US | ESPN HD"));
  assert(!esportes_emissora(e,"ESPNExtra"));
  assert(!esportes_corresponde(e,"ESPN NBA",t,t+10800));
  e[0].estado=3; assert(!esportes_corresponde(e,"Celtics vs Magic",t,t+10800));
  /* Do not inherit a participant ID/name/date as missing event identity. */
  assert(!esportes_eventos_ler("{\"events\":[{\"competitions\":[{\"competitors\":[{\"team\":{\"id\":\"wrong\",\"name\":\"wrong\",\"date\":\"2026-10-19T20:00Z\"}}]}]}]}","basketball",&liga,e,5));
  const char *tennis="{\"events\":[{\"id\":\"tour\",\"name\":\"Tournament\",\"date\":\"2026-10-19T10:00Z\",\"groupings\":[{\"competitions\":[{\"id\":\"match\",\"date\":\"2026-10-19T20:00Z\",\"competitors\":[{\"id\":\"7\",\"linescores\":[{\"value\":6},{\"value\":7}],\"athlete\":{\"displayName\":\"Player One\"}},{\"athlete\":{\"id\":\"8\",\"displayName\":\"Player Two\"}}]}]}]}]}";
  assert(esportes_eventos_ler(tennis,"tennis",&liga,e,5)==1);
  assert(!strcmp(e[0].id,"tour:match") && e[0].inicio==t);
  assert(e[0].nParticipantes==2 && strstr(e[0].nome,"Player One"));
  assert(!strcmp(e[0].participantes[0].id,"7"));
  assert(!strcmp(e[0].participantes[0].pontos,"6 7"));
  const char *race="{\"events\":[{\"id\":\"race\",\"name\":\"United States Grand Prix\",\"date\":\"2026-10-19T20:00Z\",\"competitions\":[{\"id\":\"p1\",\"date\":\"2026-10-19T20:00Z\",\"type\":{\"abbreviation\":\"FP1\"}},{\"id\":\"p2\",\"date\":\"2026-10-20T20:00Z\",\"type\":{\"abbreviation\":\"FP2\"}}]}]}";
  assert(esportes_eventos_ler(race,"racing",&liga,e,5)==2);
  assert(!strcmp(e[1].id,"race:p2") && !strcmp(e[1].sessao,"FP2"));
  assert(esportes_corresponde(e,"F1 United States Grand Prix FP1",t,t+7200));
  assert(!esportes_corresponde(e,"F1 United States Grand Prix FP2",t,t+7200));
  assert(!esportes_corresponde(e,"F1 Italian Grand Prix",t,t+7200));
  assert(esportes_eventos_ler(race,"racing",&liga,e,1)==1);
  assert(esportes_eventos_ler("{\"events\":[]}","soccer",&liga,e,5)==0);
  puts("sports parsing and event matching: passed"); return 0;
}
