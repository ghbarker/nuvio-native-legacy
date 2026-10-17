// Exercise the real Settings controls against the account registry/backends.
#include "text.h"
#include "gfx.h"
static void recorteTeste(float x,float y,float w,float h){(void)x;(void)y;(void)w;(void)h;}
static void semRecorteTeste(void){}
#define gfx_recorte recorteTeste
#define gfx_sem_recorte semRecorteTeste
#include "../src/ajustes.c"
#undef gfx_recorte
#undef gfx_sem_recorte
#include <assert.h>
static void key(SDL_Keycode k) {
  SDL_Event e;SDL_zero(e);e.type=SDL_KEYDOWN;e.key.keysym.sym=k;ajustes_evento(&e);
}
static void abrir(int op){focarOpcao(op);key(SDLK_RETURN);}
static IptvConta selecionada(void){IptvConta c;assert(iptv_conta_selecionada(&c));return c;}
int main(void) {
  assert(getenv("NUVIO_DADOS"));assert(SDL_Init(SDL_INIT_TIMER)==0);
  dados_iniciar(getenv("NUVIO_DADOS"));ajustes_dir(getenv("NUVIO_DADOS"));ajustes_iniciar();
  layout_modo_definir(1);layout_tela_definir(1080,1920);gfx_opacidade_grupo=0;
  assert(gravar());assert(!iptv_contas(NULL,0));assert(!ajOpcaoDisponivel(AJ_IPTV_CONTA));
  assert(ajOpcaoDisponivel(AJ_IPTV_ADD_XTREAM));assert(!ajOpcaoDisponivel(AJ_XTREAM_SERVIDOR));
  abrir(AJ_IPTV_ADD_XTREAM);assert(iptv_contas(NULL,0)==1);
  IptvConta um=selecionada();assert(um.tipo==IPTV_XTREAM);
  assert(ajOpcaoDisponivel(AJ_XTREAM_SERVIDOR) && !ajOpcaoDisponivel(AJ_M3U_URL));
  xtream_definir_servidor("https://one.invalid");xtream_definir_usuario("one");xtream_definir_senha("fixture");
  abrir(AJ_IPTV_ADD_XTREAM);IptvConta dois=selecionada();assert(dois.id!=um.id);
  assert(!xtream_configurado());
  xtream_definir_servidor("https://two.invalid");xtream_definir_usuario("two");xtream_definir_senha("fixture");
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
  abrir(AJ_IPTV_ADD_M3U);IptvConta tres=selecionada();assert(tres.tipo==IPTV_M3U);
  assert(ajOpcaoDisponivel(AJ_M3U_URL) && !ajOpcaoDisponivel(AJ_XTREAM_SERVIDOR));
  assert(m3u_definir_lista("https://three.invalid/list"));
  abrir(AJ_IPTV_ATIVA);assert(!selecionada().ativa && valor[AJ_IPTV_ATIVA]==1);
  abrir(AJ_IPTV_ATIVA);assert(selecionada().ativa);
  assert(iptv_conta_renomear("My playlist"));ajIptvEspelhar();assert(!strcmp(textoLeitura(AJ_IPTV_NOME),"My playlist"));
  assert(!ajOpcaoDisponivel(AJ_XTREAM_LIMPAR) && !ajOpcaoDisponivel(AJ_M3U_LIMPAR));
  assert(pedeConfirmacao(AJ_IPTV_REMOVER));
  abrir(AJ_IPTV_REMOVER);assert(iptv_contas(NULL,0)==3);
  key(SDLK_RETURN);assert(iptv_contas(NULL,0)==2 && !m3u_fonte_configurado(tres.id));
  assert(xtream_fonte_configurado(um.id) && xtream_fonte_configurado(dois.id));
  puts("PASS: account type selects the correct fields; enable/name/removal affect only the selected account and deletion requires confirmation");
  SDL_Quit();return 0;
}
