/* Include the complete production player; only host service/render boundaries
 * are supplied below. Static state is seeded to avoid opening real media. */
#include PLAYER_SOURCE
#include <assert.h>
#include <stdio.h>
#include "player-boundaries.inc"
#ifdef HAVE_RENDER
#include "player-render.inc"
#endif

static void key(SDL_Keycode code) {
  SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;
  player_evento(&e);
}

static void seed(void) {
  snprintf(fixtureItem.imdb,sizeof fixtureItem.imdb,"ttfixture");
  snprintf(fixtureItem.tipo,sizeof fixtureItem.tipo,"series");
  fixtureEpisodes[0]=(CatEp){.temporada=1,.episodio=1};
  fixtureEpisodes[1]=(CatEp){.temporada=1,.episodio=2};
  fixtureEpisodes[2]=(CatEp){.temporada=1,.episodio=3};
  fixtureEpisodeCount=3;fixtureReady=1;fixtureAutoNext=fixtureExtras=fixtureReconnect=0;fixtureHasURL=1;
  aberto=1;saindo=0;idx=0;temFixo=0;canalSessao=0;
  epT=1;epE=2;visivel=1;soBarra=0;barraFoco=0;skipFoco=0;
  comVideo=1;esperandoFonte=0;erroFonte=0;scrubbing=0;
  botao=PLR_PLAY;pedProxT=pedProxE=0;
#ifdef HAVE_EPISODE_BUTTONS
  trocaEpisodio=episodioPedidoMarcado=confirmacaoTransporteRetida=0;pedRecarregar=recarregarPedidoMarcado=0;fixtureManualClears=0;
#endif
}

#ifdef HAVE_EPISODE_BUTTONS
static int checks;
#define CHECK(v) do {assert(v);checks++;}while(0)
static void episode_buttons(void) {
  PlrAcao a[PLR_NBTNS];
  const int order[]={PLR_ANTERIOR,PLR_PLAY,PLR_PROXIMO,PLR_RECARREGAR,PLR_CC,PLR_AUDIO,PLR_ASPECTO,PLR_FONTES,PLR_EPISODIOS};
  seed();CHECK(PLR_ANTERIOR==6&&PLR_PROXIMO==7&&PLR_NBTNS==9);
  CHECK(transporteAcoes(a)==9);
  for(int i=0;i<9;i++)CHECK(a[i].id==order[i]&&a[i].habilitada);
  key(SDLK_LEFT);CHECK(botao==PLR_ANTERIOR);key(SDLK_LEFT);CHECK(botao==PLR_ANTERIOR);
  for(int i=1;i<9;i++){key(SDLK_RIGHT);CHECK(botao==order[i]);}
  key(SDLK_RIGHT);CHECK(botao==PLR_EPISODIOS);
  epE=1;botao=PLR_PLAY;CHECK(!transporteHabilitado(PLR_ANTERIOR));key(SDLK_LEFT);CHECK(botao==PLR_PLAY);
  CHECK(!ativarBotao(PLR_ANTERIOR,0)&&!trocaEpisodio);
  epE=3;botao=PLR_PLAY;CHECK(!transporteHabilitado(PLR_PROXIMO));key(SDLK_RIGHT);CHECK(botao==PLR_RECARREGAR);
  CHECK(!ativarBotao(PLR_PROXIMO,0)&&!trocaEpisodio);
  fixtureEpisodeCount=0;CHECK(!transporteHabilitado(PLR_ANTERIOR)&&!transporteHabilitado(PLR_PROXIMO));
  CHECK(!ativarBotao(PLR_PROXIMO,0));
  seed();fixtureReady=0;CHECK(!transporteHabilitado(PLR_ANTERIOR)&&!transporteHabilitado(PLR_PROXIMO));
  CHECK(!ativarBotao(PLR_PROXIMO,0));fixtureReady=1;
  CHECK(!ativarBotao(PLR_PROXIMO,1)&&!trocaEpisodio);
  CHECK(!fixtureAutoNext&&ativarBotao(PLR_PROXIMO,0));CHECK(fixtureManualClears==1);
  CHECK(pedProxT==1&&pedProxE==3&&epT==1&&epE==2);
  player_episodio_pedido_liberar();CHECK(trocaEpisodio); // cannot drop a queued request
  int t=0,e=0;CHECK(player_pediu_proximo(&t,&e)&&t==1&&e==3);CHECK(!player_pediu_proximo(&t,&e));
  fixtureClock+=1000;CHECK(!ativarBotao(PLR_ANTERIOR,0)); // app has not acknowledged transition
  player_episodio_pedido_liberar();CHECK(!trocaEpisodio);
  CHECK(ativarBotao(PLR_ANTERIOR,0));CHECK(player_pediu_proximo(&t,&e)&&t==1&&e==1);CHECK(fixtureManualClears==2);
  player_episodio_pedido_liberar();CHECK(!ativarBotao(PLR_PROXIMO,0)); // immediate second tap remains debounced
  fixtureClock+=400;CHECK(ativarBotao(PLR_PROXIMO,0));CHECK(player_pediu_proximo(&t,&e)&&e==3);
  seed();epT=epE=0;snprintf(fixtureItem.tipo,sizeof fixtureItem.tipo,"movie");CHECK(transporteAcoes(a)==6);
  const int movieOrder[]={PLR_PLAY,PLR_RECARREGAR,PLR_CC,PLR_AUDIO,PLR_ASPECTO,PLR_FONTES};for(int i=0;i<6;i++)CHECK(a[i].id==movieOrder[i]&&a[i].habilitada);
  CHECK(!ativarBotao(PLR_ANTERIOR,0)&&!ativarBotao(PLR_PROXIMO,0));
  seed();canalSessao=1;CHECK(!transporteSerie());CHECK(!transporteHabilitado(PLR_ANTERIOR)&&!transporteHabilitado(PLR_PROXIMO));
  key(SDLK_DOWN);CHECK(pedGuia==1&&!pedProxT&&!pedProxE); // unchanged live guide action
  seed();fixtureClock+=400;ponteiroAtivarBotao(PLR_PROXIMO,0);CHECK(player_transporte_pedido_pendente());
  CHECK(player_pediu_proximo(&t,&e)&&t==1&&e==3);player_episodio_pedido_liberar();
  seed();fixtureClock+=400;fixtureModal=1;ponteiroAtivarBotao(PLR_PROXIMO,0);CHECK(!player_transporte_pedido_pendente());fixtureModal=0;
  seed();epE=1;ponteiroBotaoInativo(PLR_ANTERIOR,0);CHECK(!player_transporte_pedido_pendente());
  seed();saindo=1;CHECK(!ativarBotao(PLR_PROXIMO,0));saindo=0;aberto=0;CHECK(!ativarBotao(PLR_PROXIMO,0));
  seed();fixtureClock+=400;CHECK(ativarBotao(PLR_PROXIMO,0));CHECK(player_pediu_proximo(&t,&e));player_episodio_pedido_liberar();
  botao=PLR_PLAY;int pauses=fixturePauseCalls;SDL_Event held={0};held.type=SDL_KEYDOWN;held.key.keysym.sym=SDLK_RETURN;held.key.repeat=1;player_evento(&held);CHECK(fixturePauseCalls==pauses);
  held.key.repeat=0;player_evento(&held);CHECK(fixturePauseCalls==pauses+1);
  seed();fixtureClock+=400;CHECK(ativarBotao(PLR_RECARREGAR,0));CHECK(player_transporte_pedido_pendente());CHECK(player_pediu_recarregar());CHECK(!player_pediu_recarregar());CHECK(!ativarBotao(PLR_RECARREGAR,1));CHECK(!ativarBotao(PLR_RECARREGAR,0));fixtureClock+=400;CHECK(ativarBotao(PLR_RECARREGAR,0));CHECK(player_pediu_recarregar());
  for(int mode=0;mode<3;mode++){
    seed();fixtureClock+=400;CHECK(ativarBotao(PLR_PROXIMO,0));CHECK(player_pediu_proximo(&t,&e));player_episodio_pedido_liberar();botao=PLR_PLAY;
    if(mode==1)barraFoco=1;if(mode==2)visivel=0;
    pauses=fixturePauseCalls;held.type=SDL_KEYDOWN;held.key.repeat=1;player_evento(&held);CHECK(fixturePauseCalls==pauses);
    held.type=SDL_KEYUP;held.key.repeat=0;player_evento(&held);CHECK(!confirmacaoTransporteRetida);
    visivel=1;barraFoco=0;key(SDLK_RETURN);CHECK(fixturePauseCalls==pauses+1);
  }
  seed();fixtureReconnect=1;CHECK(!transporteHabilitado(PLR_RECARREGAR)&&!ativarBotao(PLR_RECARREGAR,0));fixtureReconnect=0;fixtureHasURL=0;CHECK(!transporteHabilitado(PLR_RECARREGAR));
  seed();fixtureClock+=400;CHECK(ativarBotao(PLR_RECARREGAR,0));CHECK(!ativarBotao(PLR_ANTERIOR,0)&&!ativarBotao(PLR_PROXIMO,0));
  CHECK(!player_regra_concluiu(120,2400,-1));int arrived=0;CHECK(!player_regra_visto(0,120,120,0,&arrived)&&arrived);
  printf("PASS %d actual transport/episode activation checks with automatic Next Off\n",checks);
}
#endif

int main(void) {
  int t=-1,e=-1;
  assert(PLR_PLAY==0 && PLR_CC==1 && PLR_AUDIO==2 && PLR_ASPECTO==3 && PLR_FONTES==4 && PLR_EPISODIOS==5);
  pedProxT=1;pedProxE=2;
  assert(player_pediu_proximo(&t,&e) && t==1 && e==2);
  assert(!player_pediu_proximo(&t,&e));
  pedProxT=2;pedProxE=1;
  assert(player_pediu_proximo(NULL,NULL));
  assert(!player_pediu_proximo(NULL,NULL));
  seed();
#ifndef HAVE_EPISODE_BUTTONS
  for(int i=1;i<PLR_NBTNS;i++) { key(SDLK_RIGHT);assert(botao==i); }
  key(SDLK_RIGHT);assert(botao==PLR_EPISODIOS);
  for(int i=PLR_EPISODIOS-1;i>=0;i--) { key(SDLK_LEFT);assert(botao==i); }
  key(SDLK_LEFT);assert(botao==PLR_PLAY);
#endif
#ifdef HAVE_EPISODE_BUTTONS
  episode_buttons();
#endif
#ifdef HAVE_RENDER
  render_matrix();
#endif
  puts("PASS actual player mailbox is consumed once and existing transport IDs stay stable");
  return 0;
}
