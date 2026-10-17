// Exercise the real Settings controls against the account registry/backends.
#include "text.h"
#include "gfx.h"
static void recorteTeste(float x,float y,float w,float h){(void)x;(void)y;(void)w;(void)h;}
static void semRecorteTeste(void){}
static int recargasTeste;
static void recarregarTeste(int limpar){(void)limpar;recargasTeste++;}
#define guia_recarregar recarregarTeste
#define gfx_recorte recorteTeste
#define gfx_sem_recorte semRecorteTeste
#include "../src/ajustes.c"
#undef guia_recarregar
#undef gfx_recorte
#undef gfx_sem_recorte
#include <assert.h>
void teclado_teste_texto(const char *t);
void teclado_teste_foco(int f,int c);
static void key(SDL_Keycode k) {
  SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=k;ajustes_evento(&e);
}
static void abrir(int op){focarOpcao(op);key(SDLK_RETURN);}
static void tocar(int op) {
  for(int i=0;i<AJ_N_TELA;i++)if(TELA[i].tipo==IT_OPC && TELA[i].op==op) {
    assert(visivel(i) && focavel(i));ajPonteiroItem(i,0);
    assert(focoOp==op);key(SDLK_RETURN);return;
  }
  assert(!"Settings row missing");
}
static void salvarTeclado(const char *url) {
  assert(teclado_aberto());teclado_teste_texto(url);teclado_teste_foco(0,0);
  SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_DOWN;
  for(int i=0;i<20;i++)teclado_evento(&e);
  e.key.keysym.sym=SDLK_RIGHT;teclado_evento(&e);teclado_evento(&e);
  e.key.keysym.sym=SDLK_RETURN;teclado_evento(&e);assert(!teclado_aberto());
  ajustes_atualizar(0.016f,SDL_GetTicks());
}
static void salvarUrl(int op,const char *url) { abrir(op);salvarTeclado(url); }
static void adicionar(int op,const char *url) {
  int antes=iptv_contas(NULL,0),selecionadaAntes=iptv_conta_indice();
  tocar(op);assert(uxEditor && !teclado_aberto());
  assert(iptv_contas(NULL,0)==antes && iptv_conta_indice()==selecionadaAntes);
  ajIptvToqueEscolher(nValores(op)-1,0);
  assert(!uxEditor && teclado_aberto() && !teclado_texto()[0]);
  assert(iptv_contas(NULL,0)==antes && iptv_conta_indice()==selecionadaAntes);
  salvarTeclado(url);assert(iptv_contas(NULL,0)==antes+1);
}
static void cancelarAdicao(int op) {
  int antes=iptv_contas(NULL,0),selecionadaAntes=iptv_conta_indice();
  tocar(op);ajIptvToqueEscolher(nValores(op)-1,0);
  assert(teclado_aberto() && !teclado_texto()[0]);
  SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_ESCAPE;teclado_evento(&e);
  assert(!teclado_aberto());ajustes_atualizar(0.016f,SDL_GetTicks());
  assert(iptv_contas(NULL,0)==antes && iptv_conta_indice()==selecionadaAntes);
}
static IptvConta selecionada(void){IptvConta c;assert(iptv_conta_selecionada(&c));return c;}
int main(int argc,char **argv) {
  setvbuf(stdout,NULL,_IONBF,0);
  assert(getenv("NUVIO_DADOS"));assert(SDL_Init(SDL_INIT_TIMER)==0);
  dados_iniciar(getenv("NUVIO_DADOS"));ajustes_dir(getenv("NUVIO_DADOS"));
  if(argc>1 && !strcmp(argv[1],"legacy")) {
    m3u_fonte_selecionar(0);
    assert(m3u_definir_lista("https://legacy.invalid/list"));
    assert(m3u_definir_epg("https://legacy.invalid/epg.xml"));
  }
  ajustes_iniciar();
  layout_modo_definir(1);layout_tela_definir(1080,1920);gfx_opacidade_grupo=0;
  if(argc>1) {
    assert(dados_gravar("aviso-legal.txt","1\n"));
    if(!strcmp(argv[1],"legacy")) {
      assert(iptv_contas(NULL,0)==1 && selecionada().id==0);
      focarOpcao(AJ_IPTV_CONTA);
      adicionar(AJ_IPTV_ADD_M3U,"https://new.invalid/list");
      assert(iptv_contas(NULL,0)==2 && selecionada().tipo==IPTV_M3U && selecionada().id!=0);
      assert(!m3u_epg()[0]);
      salvarUrl(AJ_M3U_EPG,"https://new.invalid/epg.xml");
    } else assert(!strcmp(argv[1],"reload"));
    assert(iptv_contas(NULL,0)==2 && selecionada().id!=0);
    assert(!strcmp(m3u_lista(),"https://new.invalid/list"));
    assert(!strcmp(m3u_epg(),"https://new.invalid/epg.xml"));
    abrir(AJ_IPTV_CONTA);key(SDLK_UP);key(SDLK_RETURN);
    assert(selecionada().id==0);
    assert(!strcmp(m3u_lista(),"https://legacy.invalid/list"));
    assert(!strcmp(m3u_epg(),"https://legacy.invalid/epg.xml"));
    abrir(AJ_IPTV_CONTA);key(SDLK_DOWN);key(SDLK_RETURN);
    puts("PASS: mobile Settings Add preserves the migrated M3U playlist and both independent URL/EPG pairs survive process restart");
    SDL_Quit();return 0;
  }
  assert(dados_gravar("aviso-legal.txt","1\n"));
  assert(gravar());assert(!iptv_contas(NULL,0));assert(!ajOpcaoDisponivel(AJ_IPTV_CONTA));
  assert(ajOpcaoDisponivel(AJ_IPTV_ADD_XTREAM));assert(!ajOpcaoDisponivel(AJ_XTREAM_SERVIDOR));
  cancelarAdicao(AJ_IPTV_ADD_XTREAM);cancelarAdicao(AJ_IPTV_ADD_M3U);cancelarAdicao(AJ_IPTV_ADD_STALKER);
  adicionar(AJ_IPTV_ADD_XTREAM,"https://one.invalid");assert(iptv_contas(NULL,0)==1);
  IptvConta um=selecionada();assert(um.tipo==IPTV_XTREAM);
  assert(ajOpcaoDisponivel(AJ_XTREAM_SERVIDOR) && !ajOpcaoDisponivel(AJ_M3U_URL));
  assert(strstr(xtream_servidor_curto(),"one.invalid"));xtream_definir_usuario("one");xtream_definir_senha("fixture");
  adicionar(AJ_IPTV_ADD_XTREAM,"https://two.invalid");IptvConta dois=selecionada();assert(dois.id!=um.id);
  assert(!xtream_configurado());
  assert(strstr(xtream_servidor_curto(),"two.invalid"));xtream_definir_usuario("two");xtream_definir_senha("fixture");
  assert(xtream_fonte_configurado(um.id) && xtream_fonte_configurado(dois.id));
  assert(nValores(AJ_IPTV_CONTA)==2);
  abrir(AJ_IPTV_CONTA);assert(uxEditor && uxPendente==1);
  key(SDLK_UP);assert(uxPendente==0 && selecionada().id==dois.id);
  key(SDLK_ESCAPE);assert(!uxEditor && selecionada().id==dois.id);
  abrir(AJ_IPTV_CONTA);key(SDLK_UP);key(SDLK_RETURN);
  assert(!uxEditor && selecionada().id==um.id && valor[AJ_IPTV_CONTA]==0);
  assert(!strcmp(xtream_usuario(),"one"));
  char texto[160];uxValorTexto(AJ_IPTV_CONTA,1,texto,sizeof texto);assert(!strcmp(texto,dois.nome));
  puts("PASS: real Settings add two independent Xtream accounts; preview and Cancel never select; Confirm persists");
  adicionar(AJ_IPTV_ADD_M3U,"https://three.invalid/list");IptvConta tres=selecionada();assert(tres.tipo==IPTV_M3U);
  assert(ajOpcaoDisponivel(AJ_M3U_URL) && !ajOpcaoDisponivel(AJ_XTREAM_SERVIDOR));
  assert(!strcmp(m3u_lista(),"https://three.invalid/list"));
  salvarUrl(AJ_M3U_EPG,"https://three.invalid/epg.xml");
  adicionar(AJ_IPTV_ADD_M3U,"https://four.invalid/list");IptvConta quatro=selecionada();
  assert(quatro.tipo==IPTV_M3U && quatro.id!=tres.id);
  assert(!m3u_epg()[0]);
  salvarUrl(AJ_M3U_EPG,"https://four.invalid/epg.xml");
  tocar(AJ_IPTV_ADD_M3U);assert(nValores(AJ_IPTV_ADD_M3U)==3);
  uxValorTexto(AJ_IPTV_ADD_M3U,0,texto,sizeof texto);assert(!strcmp(texto,tres.nome));
  uxValorTexto(AJ_IPTV_ADD_M3U,1,texto,sizeof texto);assert(!strcmp(texto,quatro.nome));
  ajPontEdValor(0,0);ajToqueConfirmar(0,0);
  assert(!uxEditor && !teclado_aberto() && selecionada().id==tres.id);
  assert(!strcmp(m3u_lista(),"https://three.invalid/list"));
  assert(!strcmp(m3u_epg(),"https://three.invalid/epg.xml"));
  abrir(AJ_IPTV_CONTA);key(SDLK_DOWN);key(SDLK_RETURN);
  assert(selecionada().id==quatro.id);
  assert(!strcmp(m3u_lista(),"https://four.invalid/list"));
  assert(!strcmp(m3u_epg(),"https://four.invalid/epg.xml"));
  abrir(AJ_IPTV_REMOVER);key(SDLK_RETURN);
  assert(selecionada().id==tres.id && !m3u_fonte_configurado(quatro.id));
  assert(!strcmp(m3u_lista(),"https://three.invalid/list"));
  puts("PASS: real Settings adds two M3U playlists; keyboard saves independent playlist/EPG URLs; switching and deleting preserve the other account");
  int antesRecargas=recargasTeste;
  abrir(AJ_IPTV_ATIVA);assert(!selecionada().ativa && valor[AJ_IPTV_ATIVA]==1);
  assert(recargasTeste==antesRecargas+1);
  abrir(AJ_IPTV_ATIVA);assert(selecionada().ativa);
  assert(iptv_conta_renomear("My playlist"));ajIptvEspelhar();assert(!strcmp(textoLeitura(AJ_IPTV_NOME),"My playlist"));
  assert(!ajOpcaoDisponivel(AJ_XTREAM_LIMPAR) && !ajOpcaoDisponivel(AJ_M3U_LIMPAR));
  assert(pedeConfirmacao(AJ_IPTV_REMOVER));
  abrir(AJ_IPTV_REMOVER);assert(iptv_contas(NULL,0)==3);
  key(SDLK_RETURN);assert(iptv_contas(NULL,0)==2 && !m3u_fonte_configurado(tres.id));
  assert(xtream_fonte_configurado(um.id) && xtream_fonte_configurado(dois.id));
  assert(recargasTeste==antesRecargas+3);
  puts("PASS: account type selects the correct fields; enable/name/removal affect only the selected account and deletion requires confirmation");
  cancelarAdicao(AJ_IPTV_ADD_XTREAM);cancelarAdicao(AJ_IPTV_ADD_M3U);cancelarAdicao(AJ_IPTV_ADD_STALKER);
  adicionar(AJ_IPTV_ADD_STALKER,"https://portal.invalid");IptvConta portal=selecionada();
  assert(portal.tipo==IPTV_STALKER && strstr(stalker_portal_curto(),"portal.invalid"));
  assert(!stalker_configurado());salvarUrl(AJ_STALKER_MAC,"00:1a:79:00:00:01");assert(stalker_configurado());
  cancelarAdicao(AJ_IPTV_ADD_STALKER);assert(stalker_fonte_configurado(portal.id));
  tocar(AJ_IPTV_ADD_XTREAM);assert(nValores(AJ_IPTV_ADD_XTREAM)==3);
  ajPontEdValor(0,0);ajToqueConfirmar(0,0);assert(selecionada().id==um.id && !teclado_aberto());
  assert(!strcmp(xtream_usuario(),"one"));
  int antes=iptv_contas(NULL,0);tocar(AJ_IPTV_ADD_M3U);ajPontEdValor(nValores(AJ_IPTV_ADD_M3U)-1,0);key(SDLK_RETURN);
  salvarTeclado("ftp://invalid.example/list");assert(iptv_contas(NULL,0)==antes && selecionada().id==um.id);
  tocar(AJ_IPTV_ADD_M3U);ajIptvToqueEscolher(nValores(AJ_IPTV_ADD_M3U)-1,0);
  perfis_invalidar_sessao();salvarTeclado("https://stale.invalid/list");
  assert(iptv_contas(NULL,0)==antes && selecionada().id==um.id);
  tocar(AJ_IPTV_ADD_XTREAM);perfis_invalidar_sessao();key(SDLK_RETURN);
  assert(!uxEditor && !teclado_aberto() && selecionada().id==um.id);
  while(iptv_contas(NULL,0)<IPTV_MAX_CONTAS)assert(iptv_conta_adicionar(IPTV_M3U));
  ajIptvEspelhar();
  assert(ajOpcaoDisponivel(AJ_IPTV_ADD_XTREAM) && ajOpcaoDisponivel(AJ_IPTV_ADD_M3U) && ajOpcaoDisponivel(AJ_IPTV_ADD_STALKER));
  assert(nValores(AJ_IPTV_ADD_XTREAM)==2 && nValores(AJ_IPTV_ADD_STALKER)==1);
  tocar(AJ_IPTV_ADD_XTREAM);ajIptvToqueEscolher(0,0);
  assert(selecionada().id==um.id && !teclado_aberto() && iptv_contas(NULL,0)==IPTV_MAX_CONTAS);
  puts("PASS: provider-type lists contain saved accounts plus Add; Add opens a blank URL popup; Cancel and invalid M3U URLs do not create accounts; existing-account selection preserves credentials");
  SDL_Quit();return 0;
}
