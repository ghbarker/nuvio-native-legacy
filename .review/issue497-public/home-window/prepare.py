"""Prepare unchanged Home-window assertions with current typed storage neighbours.
No product or original test file is changed. All exact external adaptations
are recorded; assertion expressions and their order must remain identical.
"""
from pathlib import Path
import argparse, hashlib, json, re
p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args(); source=a.source.resolve(); out=a.out.resolve(); out.mkdir(parents=True,exist_ok=True)
here=Path(__file__).resolve().parent
original=(source/'tests/homejanelas.c').read_text(); body=original; edits=[]
def change(old,new,reason):
 global body
 assert body.count(old)==1,reason
 body=body.replace(old,new); edits.append({'old':old,'new':new,'reason':reason})
change('#include "../src/descoberta.c"','#include DISCOVERY_SOURCE','include the exact frozen product source from the review output directory')
change('void cat_republicar_fileiras(const CatFileira *f, int n) {','void fixture_original_cat_republicar_fileiras(const CatFileira *f, int n) {','the old clipping-only neighbour omits current production CW preservation')
change('int cat_home_apenas_fixas(void) { return nPub == 0; }','int fixture_original_cat_home_apenas_fixas(void) { return nPub == 0; }','current readiness recognizes a CW-only catalogue as fixed rows')
change('void  cat_trocar_continuar(const CatItem *l, int q) { (void)l; (void)q; }','void fixture_original_cat_trocar_continuar(const CatItem *l, int q) { (void)l; (void)q; }','a no-op CW storage neighbour cannot receive asynchronous production publications')
change('quieto = buscando ? 0 : quieto + 1;','quieto = desc_montando() ? 0 : quieto + 1;','the observer must await the separate CW worker before checking original assertions')
assertions=lambda text: re.findall(r'assert\s*\([^;]*?\);',text,re.S)
assert assertions(body)==assertions(original) and assertions(original)
catalog=(source/'src/catalogo.c').read_text(); start=catalog.index('void cat_republicar_fileiras('); end=catalog.index('\n}\n',start)+3
helper=here.parent/'startup/discovery/catalog-republish-actual.inc'
assert helper.read_text()==catalog[start:end], 'production catalogue boundary drifted'
startup=(here.parent/'startup/discovery/latch.c').read_text(); start=startup.index('void cat_trocar_continuar('); end=startup.index('\nstatic int catalogRows',start)
assert (here/'cw-splice.inc').read_text()==startup[start:end]+'\n'
result=out/'home-window-body.inc'; result.write_text(body)
sha=lambda data: hashlib.sha256(data).hexdigest()
receipt={'source_test_sha256':sha(original.encode()),'generated_body_sha256':sha(body.encode()),'assertions_unchanged':True,'assertions':len(assertions(original)),'assertions_sha256':sha('\n'.join(assertions(original)).encode()),'edits':edits,'catalog_helper_sha256':sha(helper.read_bytes()),'cw_splice_sha256':sha((here/'cw-splice.inc').read_bytes())}
(out/'home-window-adapter.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,sort_keys=True))
