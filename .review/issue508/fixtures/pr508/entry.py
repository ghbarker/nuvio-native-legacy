"""Source-bound focused #508 validation; GNU/Linux CPU cases, no device claims."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess, sys

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[1]))
from common import source_binding
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--proof',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cc',default='/usr/bin/cc')
a=p.parse_args(); source=a.source.resolve(); out=a.output.resolve()
out.mkdir(parents=True,exist_ok=False)
plan,binding,manifest=source_binding(source,a.proof)
origin=json.loads((HERE/'provenance.json').read_text())
assert sha(HERE/'translation-map.json')==origin['translation_map_sha256']
mapping=json.loads((HERE/'translation-map.json').read_text())
assert len(mapping)==29 and sum(len(v) for v in mapping.values())==87
tables=sorted('src/'+p.name for p in (source/'src').glob('idioma_*.h'))
assert tables==sorted(mapping)
def translated(text,key):
    matches=re.findall(r'(?:T\(|\{\s*)"'+re.escape(key)+r'"\s*,\s*("(?:\\.|[^"\\])*")',text)
    assert len(matches)==1,(key,len(matches))
    return json.loads(matches[0])
translation_records=[]
for path,entries in sorted(mapping.items()):
    text=(source/path).read_text()
    old=subprocess.check_output(['git','-C',str(source),'show',manifest['base']+':'+path],text=True)
    detailed=translated(text,'Detalhada')
    assert detailed==translated(old,'Detalhada'),path
    actual={key:translated(text,key) for key in entries}
    assert actual==entries,path
    labels=[actual['Arte desfocada'],detailed,actual['Arte nítida']]
    assert len(set(labels))==3 and all(s.strip()==s and s for s in labels),path
    translation_records.append({'path':path,'sha256':sha(source/path),
                                'new_strings':3,'distinct_labels':labels})
assert translated((source/'src/idioma_tab.h').read_text(),'Arte desfocada')=='Blurred artwork'
assert translated((source/'src/idioma_tab.h').read_text(),'Arte nítida')=='Clear artwork'
static={'passed':True,'source_commit':manifest['head'],'source_tree':manifest['tree'],
        'tables':29,'new_translated_strings':87,'saved_modes':{'0':'Blurred artwork','1':'Detailed','2':'Clear artwork'},
        'existing_detailed_translations_preserved':True,'records':translation_records,
        'translation_map_sha256':sha(HERE/'translation-map.json')}
(out/'translations.json').write_text(json.dumps(static,ensure_ascii=False,indent=2)+'\n')
groups=[]
for sanitize in [False,True]:
  for touch in [False,True]:
    for kind in ['cinema','settings']:
      mode=kind+'-'+('touch' if touch else 'tv')+'-'+('asan' if sanitize else 'normal')
      dest=out/mode; log=out/(mode+'.log')
      command=[sys.executable,str(HERE/'run.py'),'--source',str(source),'--output',str(dest),
               '--cc',a.cc,'--kind',kind,*(['--sanitize'] if sanitize else []),*(['--touch'] if touch else [])]
      with log.open('w') as stream:
        result=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,timeout=1800)
      if result.returncode: print('\n'.join(log.read_text().splitlines()[-70:]),flush=True)
      assert result.returncode==0,(mode,result.returncode)
      receipt=dest/'receipt.json'; report=json.loads(receipt.read_text())
      assert report['passed'] and report['outcome']=='passed' and report['kind']==kind
      assert report['source_commit']==manifest['head'] and report['source_tree']==manifest['tree']
      assert report['sanitize']==sanitize and report['touch_compiled']==touch and not report['runtime_mobile']
      assert report['checks']==(38 if kind=='cinema' else 19) and not report['abort_traps_fired']
      assert report['actual_detail_joined']==(kind=='cinema')
      for path,digest in report['dependency_sha256'].items(): assert sha(path)==digest,path
      for path,digest in report['raw_files_sha256'].items(): assert sha(dest/path)==digest,path
      assert sha(dest/report['binary'])==report['binary_sha256']
      groups.append({'name':mode,'kind':kind,'checks':report['checks'],'sanitize':sanitize,
                     'touch_compiled':touch,'receipt':str(receipt.relative_to(out)),
                     'receipt_sha256':sha(receipt),'binary':str((dest/'probe').relative_to(out)),
                     'binary_sha256':sha(dest/'probe'),'command':command,'execution_log':log.name,
                     'execution_log_sha256':sha(log)})
source_binding(source,a.proof)
raw={str(path.relative_to(out)):sha(path) for path in sorted(out.rglob('*')) if path.is_file() and path.suffix!='.o'}
result={'passed':True,'outcome':'passed','issue':508,'source_commit':manifest['head'],
        'source_tree':manifest['tree'],'source_binding':binding,'proof_sha256':sha(a.proof),
        'groups':groups,'group_count':8,'translation_tables':29,'translation_strings':87,
        'checks_per_cinema_group':38,'checks_per_settings_group':19,
        'fixture_provenance_sha256':sha(HERE/'provenance.json'),'raw_files_sha256':raw,
        'scope':'Fresh real player/detail/artehero/abrecine drawing and ownership state plus real Settings schema/range; both TV/touch compilation, normal and ASan/UBSan. Original17 Cinema assertions retained;21 focused ownership/classification checks appended. Fixed1920x1080 offline CPU boundaries; no GL/device/mobile geometry/network or playback-start timing claim.'}
(out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS #508: eight source-bound groups, Cinema38 and Settings19 each;29 translation tables/87 strings',flush=True)
