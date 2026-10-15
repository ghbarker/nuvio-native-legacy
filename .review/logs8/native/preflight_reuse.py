from pathlib import Path
import tempfile,json
from reuse import assert_translation_sets,dependency_snapshot,reuse_evidence
checks=[]
def rejects(label,fn):
 try:fn()
 except (AssertionError,ValueError):checks.append(label);return
 raise AssertionError('Gate did not fail closed: '+label)
with tempfile.TemporaryDirectory(prefix='logs8-reuse-') as d:
 root=Path(d);a=root/'product';b=root/'test'
 for p in [a,b]:(p/'src').mkdir(parents=True);(p/'src/a.c').write_text('#include "a.h"\nint value;\n');(p/'src/a.h').write_text('int dependency;\n')
 units=assert_translation_sets(a,b);assert units==[Path('src/a.c')];checks.append('equal translation sets accepted')
 obj=root/'a.o';dep=root/'a.d';dep.write_text(str(obj)+': '+str(a/'src/a.c')+' '+str(a/'src/a.h')+'\n')
 snap=dependency_snapshot(dep,obj,a/'src/a.c',a);assert reuse_evidence(snap,a,b)['eligible'];checks.append('identical complete dependency closure reused')
 rejects('empty snapshot rejected',lambda:reuse_evidence([],a,b))
 dep.write_text(str(obj)+':\n');rejects('empty depfile rejected',lambda:dependency_snapshot(dep,obj,a/'src/a.c',a))
 dep.write_text(str(obj)+': '+str(a/'src/a.h')+'\n');rejects('missing source dependency rejected',lambda:dependency_snapshot(dep,obj,a/'src/a.c',a))
 dep.write_text('garbage '+str(a/'src/a.c')+'\n');rejects('wrong object target rejected',lambda:dependency_snapshot(dep,obj,a/'src/a.c',a))
 dep.write_text(str(obj)+': '+str(a/'src/a.c')+' '+str(a/'missing.h')+'\n');rejects('missing dependency rejected',lambda:dependency_snapshot(dep,obj,a/'src/a.c',a))
 (b/'src/extra.c').write_text('int extra;');rejects('test-only C unit rejected',lambda:assert_translation_sets(a,b));(b/'src/extra.c').unlink()
 for p in [a,b]:(p/'src/a.h').write_text('#ifdef NV_TEST_LOCAL_LOGS\nint active;\n#endif\n')
 dep.write_text(str(obj)+': '+str(a/'src/a.c')+' '+str(a/'src/a.h')+'\n');snap=dependency_snapshot(dep,obj,a/'src/a.c',a);assert not reuse_evidence(snap,a,b)['eligible'];checks.append('same-byte macro-sensitive closure recompiled')
 for p in [a,b]:(p/'src/a.h').write_text('int dependency;\n')
 snap=dependency_snapshot(dep,obj,a/'src/a.c',a);(b/'src/a.h').write_text('int changed;');assert not reuse_evidence(snap,a,b)['eligible'];checks.append('changed private dependency recompiled')
 (a/'src/a.h').write_text('int changed_after_compile;');rejects('mutated compiled dependency rejected',lambda:reuse_evidence(snap,a,b))
report={'passed':True,'scope':'Pure recipe fail-closed reuse preflight, no application runtime claim','gates':checks};print(json.dumps(report,indent=2));Path(__file__).with_name('reuse-preflight.json').write_text(json.dumps(report,indent=2)+'\n')
