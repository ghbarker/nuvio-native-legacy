#!/usr/bin/env python3
"""External compatibility inputs only; never modifies product/tests or assertions."""
from pathlib import Path
import argparse,hashlib,json,importlib.util
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();src=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
(out/'src').symlink_to(src/'src',target_is_directory=True) if not (out/'src').exists() else None
(out/'tests').mkdir(exist_ok=True)
for f in (src/'tests').iterdir():
 t=out/'tests'/f.name
 if not t.exists():t.symlink_to(f,target_is_directory=f.is_dir())
changed=[]
def modify(name,transform):
 original=(src/'tests'/name).read_text();adapted=transform(original)
 assert original!=adapted
 # Every assertion remains exactly the same text, in the same order.
 import re
 assertions=lambda s: re.findall(r'\bassert\s*\((.*?)\)\s*;',s,re.S)
 assert assertions(original)==assertions(adapted),name
 target=out/'tests'/name;target.unlink();target.write_text(adapted)
 changed.append({'path':'tests/'+name,'original_sha256':hashlib.sha256(original.encode()).hexdigest(),'companion_sha256':hashlib.sha256(adapted.encode()).hexdigest(),'assertions_unchanged':True})
def snapshot(s):
 old='  fioFontes(&c);'; assert s.count(old)==2
 new='''  // Companion setup supplies the same immutable input as real consultar().
  c.sessao = nv_perfil_sessao_ler();
  c.origens = capturarPedidos(c.sessao, &c.nOrigens, &c.versao);
  fioFontes(&c);
  free(c.origens); c.origens = NULL;'''
 return s.replace(old,new)
modify('addons_proveniencia.c',snapshot)
def initialized_secret(s):
 old='char segredo[160], codigo[16], senha[64], perfil[2048];';assert s.count(old)==1
 return s.replace(old,'char segredo[160], codigo[16], senha[64] = {0}, perfil[2048];')
modify('jellyfin.c',initialized_secret)

def budget(s):
 marker='#include "../src/app.c"';assert s.count(marker)==1
 aliases='''// Legacy fixture retains its original first/channel selector doubles.
#define stream_primeira_boa_sessao teste_stream_primeira_boa_sessao
#define stream_canal_primeira_viva_sessao teste_stream_canal_primeira_viva_sessao
'''
 s=s.replace(marker,aliases+marker)
 marker='\nint main(int argc, char **argv) {';assert s.count(marker)==1
 wrappers='''
int teste_stream_primeira_boa_sessao(int n, uint64_t epoch) {
  if (epoch != 0) abort(); return stream_primeira_boa(n);
}
int teste_stream_canal_primeira_viva_sessao(int n, uint64_t epoch) {
  if (epoch != 0) abort(); return stream_canal_primeira_viva(n);
}
'''
 return s.replace(marker,wrappers+marker)
modify('fontesmortas_orcamento.c',budget)
(out/'epoch.c').write_text('''// Scoped imports for reduced single-session legacy fixtures only.
#include <stdint.h>
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t g) { return g == 0; }
int perfis_sessao_travar(uint64_t g) { return g == 0; }
void perfis_sessao_destravar(void) { }
''')
# Existing narrow adapters forward scoped requests to original fixture doubles.
(out/'playback-neighbours.c').write_text('// Only the unchanged upstream fontefalha_playback reduced fixture links\n// this adapter. It intentionally has one session and no provider accounts.\n// All routing, player and mailbox assertions remain in the original fixture.\n#include "debrid.h"\n#include "streams.h"\n#include "stalker.h"\n#include <assert.h>\n#include <string.h>\nstatic const DebridContexto *entered;\nvoid debrid_contexto_capturar(DebridContexto *c) {\n  assert(c); memset(c, 0, sizeof *c);\n}\nvoid debrid_contexto_entrar(const DebridContexto *c) {\n  assert(c && c->sessao == 0 && !entered); entered = c;\n}\nvoid debrid_contexto_sair(void) { assert(entered); entered = 0; }\nint stream_primeira_boa_sessao(int n, uint64_t epoch) {\n  assert(epoch == 0); return stream_primeira_boa(n);\n}\nint stream_canal_primeira_viva_sessao(int n, uint64_t epoch) {\n  assert(epoch == 0); return stream_canal_primeira_viva(n);\n}\nint stream_verificar_proxima_vod_sessao(int idx, int n, int *checked, uint64_t epoch) {\n  assert(epoch == 0); return stream_verificar_proxima_vod(idx, n, checked);\n}\nint stalker_resolver_sessao(const char *id, char *url, unsigned n, uint64_t epoch) {\n  assert(epoch == 0); return stalker_resolver(id, url, n);\n}\nint stream_falha_aplicar_sessao(int idx, unsigned generation, int service,\n                              const char *url, uint64_t epoch) {\n  assert(epoch == 0); return stream_falha_aplicar(idx, generation, service, url);\n}\n')
(out/'budget-neighbours.c').write_text('// Only the original reduced fontesmortas_orcamento fixture links these\n// omitted account neighbours. The next-VOD worker and budget remain real.\n#include "debrid.h"\n#include "stalker.h"\n#include <assert.h>\n#include <string.h>\nstatic const _Thread_local DebridContexto *entered;\nvoid debrid_contexto_capturar(DebridContexto *c) {\n  assert(c); memset(c, 0, sizeof *c);\n}\nvoid debrid_contexto_entrar(const DebridContexto *c) {\n  assert(c && c->sessao == 0 && !entered); entered = c;\n}\nvoid debrid_contexto_sair(void) { assert(entered); entered = 0; }\nint stalker_resolver_sessao(const char *id, char *url, unsigned n, uint64_t epoch) {\n  assert(epoch == 0); return stalker_resolver(id, url, n);\n}\n')
(out/'preload-off-neighbours.c').write_text('// Legacy player/failover fixtures predate opt-in preload and have no title page.\n// Supply missing disabled-feature neighbours only. Strong existing fixture\n// mocks always win. The real preload controller has separate ownership tests.\n#include "ajustes.h"\n#include "detail.h"\n#include "fonteregra.h"\n#include "streams.h"\n#include "debrid.h"\n#include <assert.h>\n#define W __attribute__((weak))\n#define OFF(name) W int name(void) { return 0; }\nOFF(ajustes_fonte_preparar)\nOFF(ajustes_fonte_precarregar)\nOFF(ajustes_fonte_manual)\nOFF(ajustes_fonte_primeira)\nOFF(ajustes_fonte_escopo)\nOFF(ajustes_fonte_regex_modo)\nOFF(ajustes_fonte_usar_outros)\nOFF(ajustes_fonte_ordem_uso)\nOFF(ajustes_fonte_prioridade)\nOFF(ajustes_fonte_hdr)\nOFF(ajustes_dolby_vision)\nOFF(ajustes_dv_mkv)\nOFF(ajustes_dolby_atmos)\nOFF(ajustes_tamanho_min_gb)\nOFF(ajustes_tamanho_max_gb)\nOFF(ajustes_buffer_preset)\nOFF(ajustes_cache_seek_mb)\nOFF(ajustes_fonte_conferir_varias)\nOFF(ajustes_fonte_tocar_conferindo)\nW const char *ajustes_qualidade(void) { return "Automática"; }\nW unsigned fonteregra_versao(void) { return 0; }\nW int perfis_ativo(void) { return 1; }\nW const char *sessao_usuario(void) { return ""; }\nW int detail_indice(void) { return -1; }\nW int detail_aberto(void) { return 0; }\nW int detail_ep_foco(int *t, int *e) { if (t) *t = 0; if (e) *e = 0; return 0; }\nW int stream_lista_do_alvo(const char *id) { (void)id; return 0; }\nW int debrid_servico_disponivel(int servico) { (void)servico; return 0; }\nW int stream_falha_renovar(int i, unsigned g, int s, int (*c)(void *), void *ctx,\n                          char *url, unsigned n) {\n  (void)i; (void)g; (void)s; (void)c; (void)ctx; (void)url; (void)n;\n  assert(!"Disabled preload must not renew a prepared route"); return 0;\n}\nW int debrid_resolver_servico_cache(int s, const char *hash, int file,\n                                  char *url, unsigned n, int *fora) {\n  (void)s; (void)hash; (void)file; (void)url; (void)n; (void)fora;\n  assert(!"Legacy budget has no native provider route"); return 0;\n}\n')
(out/'manifest.json').write_text(json.dumps({'source':str(src),'label':'legacy assertions + dispatch snapshot setup','files':changed,'product_tests_unchanged':True},indent=2))
print(out/'manifest.json')
