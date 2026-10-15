/* Actual addon queue and late-result handling; no network worker launched. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include ADDONS_SOURCE
void fixture_queue(const char *old,const char *next) {
 nAddon=1;fioVivo=1;progLigado=1;atomic_store(&estado,ADD_BUSCANDO);
 snprintf(alvoId,sizeof alvoId,"%s",old);snprintf(alvoTipo,sizeof alvoTipo,"series");
 stream_definir_alvo(next);
 addons_buscar(next,"series");
 assert(!strcmp(pendId,next));
}
void fixture_old_response(void) {
 assert(pendId[0]);
 progN=1;progFila[0].idx=0;progFila[0].n=1;progFila[0].a=calloc(1,sizeof(Stream));assert(progFila[0].a);
 snprintf(progFila[0].a->url,sizeof progFila[0].a->url,"https://example.invalid/old-late.mkv");
 progDrenar();assert(progN==0);
}
void fixture_queued_dispatched(const char *next) {
 assert(!strcmp(pendId,next));
 /* Model worker completion without creating/joining an actual network thread.
  * Actual addons_estado performs these queue bookkeeping writes before calling
  * addons_buscar(next), which starts the next worker. Publication below uses
  * the actual Stream API, as the next worker's response would.
  */
 fioVivo=0;progLigado=0;pendId[0]=0;snprintf(alvoId,sizeof alvoId,"%s",next);stream_definir_alvo(next);
}
void ondever_pedir(const char *id,int serie,long tmdb){assert(id&&serie&&!tmdb);}
void debrid_nova_busca(void){}
