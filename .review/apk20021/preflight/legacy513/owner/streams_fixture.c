/* Complete actual Sources UI entry/event/list implementation, renderer idle. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include STREAMS_SOURCE
void fixture_queue(const char*,const char*);
void fixture_old_response(void);
void fixture_queued_dispatched(const char*);
#ifdef NV_TOUCH_UI
float nv_layout_w=1080.0f,nv_layout_h=1920.0f;
int layout_modo_mobile(void){return 1;}
#endif
static int failures,checks;
static void verify(int good,const char *what){checks++;printf("%s %s\n",good?"PASS":"FAIL",what);if(!good)failures++;}
Uint32 SDL_GetTicks(void){return 1000;}
unsigned selospacote_versao(void){return 1;}
int ajustes_selos_coloridos(void){return 0;}
void selospacote_colorido(int c){assert(!c);}
int selospacote_ativo(void){return -1;}
int selospacote_embutidos_ok(void){return 0;}
uint64_t streamfit_agora_ms(void){return 1000;}
void streamfit_foto(StreamfitFoto *out,uint64_t now){(void)now;memset(out,0,sizeof *out);}
StreamfitClasse streamfit_classificar(const StreamfitFoto *foto,const char *url,uint64_t bytes,double duration,StreamfitResultado *out){(void)foto;(void)url;(void)bytes;(void)duration;memset(out,0,sizeof *out);return SF_DESCONHECIDA;}
float gfx_escala_ui(void){return 1;}
int player_aberto(void){return 0;}
void ondever_apps_atualizar(void){}
int ajustes_fonte_ordem_addon(void){return 1;}
int ajustes_fonte_texto_addon(void){return 1;}
int ajustes_fonte_escopo(void){return 0;}
int ajustes_fonte_regex_modo(void){return 0;}
int ajustes_fonte_usar_outros(void){return 1;}
unsigned fonteregra_versao(void){return 1;}
int fonteregra_grupo(const FonteRegraCfg *cfg,const char *name,int plugin,const char *text){(void)cfg;(void)name;(void)plugin;(void)text;return 0;}
uint64_t badges_bit(const char *id){(void)id;return 0;}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 Stream old={0},next={0};int chosen=-1;SDL_Event event={0};event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_RETURN;
 snprintf(old.url,sizeof old.url,"https://example.invalid/S01E01.mkv");snprintf(old.provedor,sizeof old.provedor,"Fixture addon");old.altura=1080;old.foraCache=1;
 next=old;snprintf(next.url,sizeof next.url,"https://example.invalid/S01E02.mkv");
 stream_definir_alvo("tt1234567:1:1");stream_definir_lista(&old,1);
 verify(stream_n()==1&&stream_lista_do_alvo("tt1234567:1:1"),"old episode list has exact old target");
 fixture_queue("tt1234567:1:1","tt1234567:1:2");
 verify(stream_n()==1&&!stream_lista_do_alvo("tt1234567:1:2"),"actual addon queue retains stale list before opening Sources");
 stream_folha_abrir();
 verify(stream_n()==0&&stream_item(0)==NULL&&stream_folha_aberta(),"opening Sources discards stale links without closing or waiting");
 stream_folha_evento(&event);
 verify(!stream_folha_escolheu(&chosen),"OK cannot select a previous episode link while new search waits");
 fixture_old_response();
 verify(stream_n()==0&&stream_item(0)==NULL,"old worker late response cannot repopulate new episode sheet");
 fixture_queued_dispatched("tt1234567:1:2");stream_definir_lista(&next,1);
 verify(stream_n()==1&&stream_lista_do_alvo("tt1234567:1:2"),"correct new episode sources publish to requested target");
 /* Old baseline may have selected and closed the stale sheet. Restore only
  * its visibility for the independent new-target selection assertion.
  */
 if(!stream_folha_aberta()) stream_folha_abrir();
 stream_folha_evento(&event);
 verify(stream_folha_escolheu(&chosen)&&chosen==0&&!strcmp(stream_item(chosen)->url,next.url),"OK selects the correct new episode link after it arrives");
 unsigned generation=stream_lista_geracao();stream_folha_abrir();
 verify(stream_n()==1&&stream_lista_geracao()==generation&&stream_lista_do_alvo("tt1234567:1:2"),"opening same-target Sources preserves list and generation");
 /* A replacement between key handling and consumption must not rebind the
  * selected raw index, even when the replacement has the same target. */
 Stream replacement=next;snprintf(replacement.url,sizeof replacement.url,"https://example.invalid/S01E02-refreshed.mkv");
 stream_folha_evento(&event);stream_definir_lista(&replacement,1);
 verify(!stream_folha_escolheu(&chosen)&&stream_folha_aberta(),"same-target list replacement rejects old index and reopens Sources");
 verify(!stream_folha_escolheu(&chosen),"rejected old choice cannot be consumed twice");
 stream_folha_evento(&event);
 verify(stream_folha_escolheu(&chosen)&&!strcmp(stream_item(chosen)->url,replacement.url),"confirming refreshed source selects the refreshed URL");
 /* A normal late addon response appends: raw indexes and generation remain
  * stable, so a valid click stays valid. */
 stream_folha_abrir();stream_folha_evento(&event);generation=stream_lista_geracao();
 stream_lista_acrescentar(&next,1,1);
 verify(stream_lista_geracao()==generation&&stream_n()==2,"partial addon append preserves existing list generation");
 verify(stream_folha_escolheu(&chosen)&&chosen==0&&!strcmp(stream_item(chosen)->url,replacement.url),"partial addon append preserves the actual clicked source");
 /* Target changes after the input event, before the app consumes it. */
 stream_folha_abrir();stream_folha_evento(&event);stream_definir_alvo("tt1234567:2:1");
 verify(!stream_folha_escolheu(&chosen)&&stream_folha_aberta()&&stream_n()==0,"target changed after OK cannot consume previous episode link");
 stream_definir_lista(&next,1);stream_folha_evento(&event);
 verify(stream_folha_escolheu(&chosen)&&stream_lista_do_alvo("tt1234567:2:1"),"same sheet can choose a later-season link when its source arrives");
 /* Existing fixture/legacy callers can open an untagged list; no target was
  * requested so there is no conflicting ownership to invent. */
 stream_definir_alvo("");stream_definir_lista(&old,1);stream_folha_abrir();stream_folha_evento(&event);
 verify(stream_folha_escolheu(&chosen)&&!strcmp(stream_item(chosen)->url,old.url),"untagged legacy source lists preserve existing explicit selection");
 stream_definir_alvo("kitsu:41370:1");stream_definir_lista(&old,1);stream_definir_alvo("kitsu:41370:2");stream_folha_abrir();
 verify(stream_n()==0&&stream_folha_aberta(),"normalized non-IMDb episode target also clears stale sources");
 stream_definir_lista(&next,1);stream_folha_evento(&event);
 verify(stream_folha_escolheu(&chosen)&&stream_lista_do_alvo("kitsu:41370:2"),"normalized non-IMDb target can select its correct source");
 aberta=0;stream_definir_lista(NULL,0);
 printf("RESULT checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
