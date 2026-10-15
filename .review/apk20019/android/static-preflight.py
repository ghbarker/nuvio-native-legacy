#!/usr/bin/env python3
"""Small offline recipe checks only: no app compilation, APK or backend action."""
from pathlib import Path
import copy,hashlib,importlib.util,json,os,shutil,subprocess,tempfile

HERE=Path(__file__).resolve().parent
records=[]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def check(name,fn):
 fn();records.append({'check':name,'passed':True})
def reject(fn):
 try:fn()
 except (AssertionError,KeyError,FileNotFoundError):return
 raise AssertionError('Unsafe recipe input was accepted')
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);return mod

for path in HERE.glob('*.py'):compile(path.read_text(),str(path),'exec')
records.append({'check':'all Python helpers parse without execution','passed':True})
common=load('apk20019_common',HERE/'common.py')
metadata=load('apk20019_metadata',HERE/'safe_android_metadata.py')
old_env={k:os.environ.get(k) for k in ['NUVIO_REVIEW_PLAN_SHA256','GITHUB_SHA']}
try:
 with tempfile.TemporaryDirectory(prefix='static-fixtures-',dir=HERE) as directory:
  root=Path(directory);stage=root/'stage';android=stage/'android';android.mkdir(parents=True)
  for name in ['common.py','prepare_env.py','run.py','prepare_source.py']:
   shutil.copy2(HERE/name,android/name)
  common.HERE=android
  ident={'package':'space.nuvio.nativelegacy.touch','label':'Nuvio TV/Mobile Test','version_code':20019,'version_name':'2.0.19-test-logs9','certificate_sha256':'862d7ef63a0dbde2929f6f231cae295510726d9ac28f9292496d2a38304be5e6'}
  for layer in ['product','test']:(stage/(layer+'.json')).write_text('{}')
  base={'format':1,'frozen':True,'root_reviewed':True,'identity':ident,'bundle':{'sha256':'f'*64},'sources':{layer:{'commit':'1'*40,'tree':'2'*40,'manifest':layer+'.json','manifest_sha256':sha(stage/(layer+'.json'))} for layer in ['product','test']}}
  def setplan(value,pin=True):
   path=stage/'plan.json';path.write_text(json.dumps(value));os.environ['NUVIO_REVIEW_PLAN_SHA256']=sha(path) if pin else '0'*64
  setplan(base);check('complete approved pinned schema accepted',common.plan)
  for field in ['frozen','root_reviewed']:
   value=copy.deepcopy(base);value[field]=False;setplan(value);check(field+' false refused',lambda:reject(common.plan))
  setplan(base,False);check('wrong explicit plan SHA refused',lambda:reject(common.plan))
  os.environ.pop('NUVIO_REVIEW_PLAN_SHA256');check('missing explicit plan SHA refused',lambda:reject(common.plan))
  for layer in ['product','test']:
   for field in ['commit','tree','manifest_sha256']:
    value=copy.deepcopy(base);value['sources'][layer][field]=None;setplan(value);check(layer+' pending '+field+' refused',lambda:reject(common.plan))
  value=copy.deepcopy(base);value['bundle']['sha256']=None;setplan(value);check('pending bundle pin refused',lambda:reject(common.plan))
  for field,bad in [('version_code',20018),('package','space.nuvio.other'),('certificate_sha256','0'*64),('version_name','old-version')]:
   value=copy.deepcopy(base);value['identity'][field]=bad;setplan(value);check('wrong identity '+field+' refused',lambda:reject(common.plan))
  setplan(base);(stage/'product.json').write_text('{"changed":true}');check('modified source manifest refused',lambda:reject(common.plan));(stage/'product.json').write_text('{}')

  # Fail before any build output or private configuration exists.
  value=copy.deepcopy(base);value['frozen']=False;setplan(value)
  for script,args in [('prepare_env.py',['--output',str(root/'private')]),('run.py',['--source',str(root/'absent-source'),'--source-proof',str(root/'absent-proof'),'--output',str(root/'apk-output'),'--layer','test'])]:
   r=subprocess.run(['python3',str(android/script),*args],text=True,capture_output=True)
   assert r.returncode and 'Root has not approved' in r.stderr
   assert not (root/'private').exists() and not (root/'apk-output').exists()
   records.append({'check':script+' refuses unapproved plan before side effects','passed':True})

  # Narrow CI transformation checks use a tiny local repository, not app source.
  source=root/'source';source.mkdir();(source/'tests').mkdir();(source/'src').mkdir();(source/'android/app').mkdir(parents=True)
  (source/'tests/original.txt').write_text('unchanged original test\n');(source/'src/dummy.c').write_text('int dummy;\n')
  gradle='android/app/build.gradle.kts';original='android {\n    signingConfigs {\n    }\n}\n';(source/gradle).write_text(original)
  def git(*args):return subprocess.check_output(['git','-C',str(source),*args],text=True,stderr=subprocess.PIPE).strip()
  git('init','-q');git('add','.');git('-c','user.name=Recipe Preflight','-c','user.email=recipe@example.invalid','commit','-qm','local fixture')
  files=[{'path':str(p.relative_to(source)),'sha256':sha(p)} for p in [source/gradle,source/'src/dummy.c',source/'tests/original.txt']]
  manifest={'files':files,'tests_tree':git('rev-parse','HEAD:tests'),'tests_tree_unchanged':True}
  for layer in ['product','test']:(stage/(layer+'.json')).write_text(json.dumps(manifest));base['sources'][layer].update(commit=git('rev-parse','HEAD'),tree=git('rev-parse','HEAD^{tree}'),manifest_sha256=sha(stage/(layer+'.json')))
  setplan(base);proof={'verified':True,'source_clean':True,'original_tests_tree_unchanged':True,'review_only':False,'layer':'product','source_commit':base['sources']['product']['commit'],'tree':base['sources']['product']['tree'],'manifest_sha256':base['sources']['product']['manifest_sha256'],'bundle_sha256':'f'*64,'review_commit':'a'*40}
  proofpath=root/'proof.json';proofpath.write_text(json.dumps(proof));os.environ.pop('GITHUB_SHA',None)
  check('matching source/proof/test-tree binding accepted',lambda:common.binding(source,proofpath,'product'))
  os.environ['GITHUB_SHA']='b'*40;check('another CI review commit refused',lambda:reject(lambda:common.binding(source,proofpath,'product')));os.environ.pop('GITHUB_SHA')
  approved=original.replace('    signingConfigs {\n','    signingConfigs {\n'+common.DEBUG_SIGNER,1);(source/gradle).write_text(approved)
  transform={'source_commit':proof['source_commit'],'tree':proof['tree'],'layer':'product','source_proof_sha256':sha(proofpath),'changes':[{'path':gradle,'before_sha256':hashlib.sha256(original.encode()).hexdigest(),'after_sha256':sha(source/gradle)}]}
  receipt=root/'transform.json';receipt.write_text(json.dumps(transform));check('exact product debug signer insertion accepted',lambda:common.transformed(source,proofpath,'product',receipt))
  (source/'src/dummy.c').write_text('int changed;\n');forged=copy.deepcopy(transform);forged['changes'].append({'path':'src/dummy.c','before_sha256':files[1]['sha256'],'after_sha256':sha(source/'src/dummy.c')});receipt.write_text(json.dumps(forged));check('forged C mutation receipt refused',lambda:reject(lambda:common.transformed(source,proofpath,'product',receipt)))
  (source/'src/dummy.c').write_text('int dummy;\n');(source/gradle).write_text(original);proof.update(layer='test',manifest_sha256=base['sources']['test']['manifest_sha256']);proofpath.write_text(json.dumps(proof));transform.update(layer='test',source_proof_sha256=sha(proofpath),changes=[]);receipt.write_text(json.dumps(transform));check('test layer accepts only unchanged source',lambda:common.transformed(source,proofpath,'test',receipt))
  (source/'src/dummy.c').write_text('int changed;\n');check('unlisted test-layer source mutation refused',lambda:reject(lambda:common.transformed(source,proofpath,'test',receipt)))

  # AGP's copied compile database may not be beside the real .cxx cache.
  app=root/'metadata/android/app';cpp=app/'src/main/cpp';cpp.mkdir(parents=True)
  directory=app/'.cxx/Debug/example/arm64-v8a';directory.mkdir(parents=True)
  database=app/'build/intermediates/cxx/Debug/example/arm64-v8a/compile_commands.json';database.parent.mkdir(parents=True)
  commands=[{'directory':str(directory),'file':str(root/'metadata/src/app.c'),'arguments':['clang','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-DNV_TEST_LOCAL_LOGS=1','-DNV_REC_URL="SECRET_FIXTURE_VALUE"','-o','CMakeFiles/main.dir/app.c.o']}]
  database.write_text(json.dumps(commands));cache=directory/'CMakeCache.txt'
  cachetext='CMAKE_CACHEFILE_DIR:INTERNAL='+str(directory)+'\nCMAKE_HOME_DIRECTORY:INTERNAL='+str(cpp)+'\nANDROID_ABI:STRING=arm64-v8a\nNV_TEST_LOCAL_LOGS:BOOL=ON\n';cache.write_text(cachetext)
  check('canonical cache found through actual compile directory',lambda:metadata.cache_for_commands(root/'metadata',database,'arm64-v8a',commands))
  assert cache.parent!=database.parent
  cache.write_text(cachetext.replace('ANDROID_ABI:STRING=arm64-v8a','ANDROID_ABI:STRING=armeabi-v7a'));check('wrong ABI cache refused',lambda:reject(lambda:metadata.cache_for_commands(root/'metadata',database,'arm64-v8a',commands)))
  cache.write_text(cachetext.replace(str(cpp),str(root/'other-source')));check('wrong source cache refused',lambda:reject(lambda:metadata.cache_for_commands(root/'metadata',database,'arm64-v8a',commands)))
  cache.unlink();(database.parent/'CMakeCache.txt').write_text(cachetext);check('adjacent copied cache is never used as fallback',lambda:reject(lambda:metadata.cache_for_commands(root/'metadata',database,'arm64-v8a',commands)))
  cache.write_text(cachetext);safe=metadata.collect(root/'metadata','test');safejson=json.dumps(safe);assert 'SECRET_FIXTURE_VALUE' not in safejson and '"arguments"' not in safejson and '"command"' not in safejson
  assert safe['native'][0]['cache']['source_abi_build_bound'];records.append({'check':'sanitized metadata never records property values or commands','passed':True})

  jobfile=root/'job.json';subprocess.run(['python3',str(HERE/'make_job.py'),'--output',str(jobfile)],check=True,capture_output=True);job=json.loads(jobfile.read_text());steps=job['steps']
  sdk=[s for s in steps if s.get('uses')=='android-actions/setup-android@v3'];assert len(sdk)==1 and sdk[0]['with']=={'packages':'platform-tools','log-accepted-android-sdk-licenses':False}
  assert job['env']['NUVIO_REVIEW_PLAN_SHA256']=='PENDING_ROOT_REVIEW' and job['permissions']=={'contents':'read'}
  text=json.dumps(job);assert '--allow-review-plan' not in text and 'publish' not in text.lower() and 'release' not in text.lower()
  for layer in ['product','test']:assert any('--layer '+layer+' --source source-'+layer in s.get('run','') for s in steps)
  assert any('command -v sdkmanager >/dev/null' in s.get('run','') for s in steps)
  records.append({'check':'named job has exact SDK setup, blocked pin and both fresh layers','passed':True})
  for path in ['run.py','verify_apk.py','prepare_source.py']:assert '--allow-review-plan' not in (HERE/path).read_text()
  text=(HERE/'run.py').read_text();assert "env['GRADLE_OPTS']" in text and '-Dorg.gradle.project.nuvio.testLocalLogs=' in text and "env.pop('ORG_GRADLE_PROJECT_nuvio.testLocalLogs'" in text
  records.append({'check':'GRADLE_OPTS correction and no review bypass retained','passed':True})
finally:
 for key,value in old_env.items():
  if value is None:os.environ.pop(key,None)
  else:os.environ[key]=value

receipt={'outcome':'passed','scope':'Offline recipe schema, fail-closed approval/source guards, narrow transformation policy and sanitized metadata fixtures only; no application source suites, Android build, APK verification or backend actions executed','checks':records,'count':len(records),'recipe_files':{p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.suffix in ['.py','.md','.json'] and p.name not in ['static-preflight-receipt.json','recipe-manifest.json']}}
(HERE/'static-preflight-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'passed':len(records),'receipt':str(HERE/'static-preflight-receipt.json')}))
