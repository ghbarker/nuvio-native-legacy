#ifndef NV_CACHEDISCO_H
#define NV_CACHEDISCO_H
#ifndef __EMSCRIPTEN__
#include <stdint.h>
/* Chamador serializa escrita/poda. Apenas arquivos hash.ext entram na poda. */
typedef int (*NvCacheProtegido)(const char *caminho, void *ctx);
/* Antes dos leitores/escritores: move apenas arte regeneravel e suas marcas.
   Retorna -1 se destino nao puder ser usado; >= 0 permite seleciona-lo.
   Arquivos desconhecidos e links simbolicos ficam intocados na origem. */
int nv_cache_migrar_arte(const char *origem, const char *destino);
long nv_cache_podar(const char *dir, long *total, long entrada, long teto,
                   uint64_t reserva, int forcar, NvCacheProtegido protegido, void *ctx);
#endif
#endif
