#!/usr/bin/env python3
"""Generate an explicit review-only workflow from named components, never step indices."""
from pathlib import Path
import argparse,json
p=argparse.ArgumentParser();p.add_argument('--prefix',default='.review/logs8');p.add_argument('--output',default='logs8-review.yml');p.add_argument('--branch',default='review/logs8-integration-20261015');a=p.parse_args();here=Path(__file__).resolve().parent;prefix=a.prefix.rstrip('/');plan_sha=__import__('hashlib').sha256((here/'plan.json').read_bytes()).hexdigest()
checkout={'uses':'actions/checkout@v4','with':{'fetch-depth':0}}
check={'name':'Require the exact root-frozen plan','env':{'INPUT_PLAN_SHA':'${{ inputs.plan_sha256 }}'},'run':f'''python3 - <<'CHECK'
import hashlib,json,os
from pathlib import Path
p=Path({json.dumps(prefix+'/plan.json')})
expected=os.environ['LOGS8_PLAN_SHA']
assert hashlib.sha256(p.read_bytes()).hexdigest()==expected, 'Wrong frozen plan hash'
provided=os.environ.get('INPUT_PLAN_SHA','')
assert not provided or provided==expected, 'Manual input must match the pinned push plan'
assert json.loads(p.read_text())['frozen'], 'Review-only plan has not been frozen'
CHECK
'''}
materialize={'name':'Materialize and verify product/test commit objects and unchanged tests','run':f'''python3 {prefix}/materialize.py --layer product --output source-product --proof build/review/logs8/product-source.json --base-repository "$GITHUB_WORKSPACE"
python3 {prefix}/materialize.py --layer test --output source-test --proof build/review/logs8/test-source.json --base-repository "$GITHUB_WORKSPACE"
'''}
workflow={'name':'Logs8 integrated review build','on':{'push':{'branches':[a.branch]},'workflow_dispatch':{'inputs':{'plan_sha256':{'description':'SHA256 of the root-frozen plan.json','required':False,'type':'string'}}}},'env':{'LOGS8_PLAN_SHA':plan_sha},'permissions':{'contents':'read','actions':'read'},'concurrency':{'group':'logs8-review-${{ github.ref }}','cancel-in-progress':False},'jobs':{
 'native':{'runs-on':'macos-14','timeout-minutes':45,'steps':[checkout,check,materialize,{'name':'Install real SDL SDK and pin the system C driver','run':'brew install sdl2 sdl2_image sdl2_ttf\necho "LOGS8_REAL_CC=/usr/bin/cc" >> "$GITHUB_ENV"\n/usr/bin/cc --version\n'},{'name':'Complete TV/touch product and private app links plus focused actual C normal/sanitized','run':f'python3 {prefix}/native/run.py --product source-product --test source-test --output build/review/logs8/native --cc "$LOGS8_REAL_CC" --product-proof build/review/logs8/product-source.json --test-proof build/review/logs8/test-source.json'},{'uses':'actions/upload-artifact@v4','if':'always()','with':{'name':'logs8-native-${{ github.sha }}','path':'build/review/logs8','if-no-files-found':'error'}}]},
 'kotlin':{'runs-on':'ubuntu-24.04','timeout-minutes':15,'steps':[checkout,check,materialize,{'uses':'actions/setup-java@v4','with':{'distribution':'temurin','java-version':'21'}},{'uses':'actions/cache@v4','with':{'path':'~/.cache/nuvio-kotlin-preflight','key':'logs8-kotlin-tools-${{ hashFiles(\''+prefix+'/kotlin/tools.json\') }}'}},{'name':'Run actual integrated Download and private snapshot32contracts','run':f'python3 {prefix}/kotlin/run.py --source source-test --output build/review/logs8/kotlin --tools "$HOME/.cache/nuvio-kotlin-preflight" --source-proof build/review/logs8/test-source.json'},{'name':'Bind only15unchanged historical player contracts to exact current graph','env':{'GH_TOKEN':'${{ github.token }}'},'run':f'python3 {prefix}/runtime/verify_reuse.py --source source-test --source-proof build/review/logs8/test-source.json --output build/review/logs8/player-reuse --repository "$GITHUB_REPOSITORY"'},{'uses':'actions/upload-artifact@v4','if':'always()','with':{'name':'logs8-kotlin-player-${{ github.sha }}','path':'build/review/logs8','if-no-files-found':'error'}}]}
}}
fragment=here/'android/job-fragment.json'
if fragment.exists():
 job=json.loads(fragment.read_text());assert isinstance(job,dict) and 'steps' in job
 steps=[];found=0
 for step in job['steps']:
  steps.append(step)
  if step.get('uses')=='actions/checkout@v4':steps.append(check);found+=1
 assert found==1,'Android job must have one named checkout boundary'
 job['steps']=steps
 workflow['jobs']['android']=job
else:(here/'workflow-fragment-pending.json').write_text(json.dumps({'android_fragment_pending':True})+'\n')
workflow['jobs']['original3']={'runs-on':'macos-14','timeout-minutes':30,'strategy':{'fail-fast':False,'matrix':{'suite':['fontepref','fontepref_file','player_retido']}},'steps':[checkout,check,materialize,
 {'name':'Install the same real SDK libraries and pin system compiler before wrapper PATH','run':'brew install sdl2 sdl2_image sdl2_ttf sdl3\n/usr/bin/cc --version\n'},
 {'name':'Replay the unchanged original full-source ownership sanitizer script','run':f'python3 {prefix}/original3/run.py --source source-product --source-proof build/review/logs8/product-source.json --suite "${{{{ matrix.suite }}}}" --wrapper {prefix}/original3/cc_rpath.py --output build/review/logs8/original-${{{{ matrix.suite }}}}'},
 {'uses':'actions/upload-artifact@v4','if':'always()','with':{'name':'logs8-original-${{ matrix.suite }}-${{ github.sha }}','path':'build/review/logs8','if-no-files-found':'error'}}]}
workflow['jobs']['review-ready']={'runs-on':'ubuntu-24.04','needs':list(workflow['jobs']),'steps':[checkout,check,
 *[{'uses':'actions/download-artifact@v4','with':{'name':name+'-${{ github.sha }}','path':'review-results/'+directory}} for name,directory in [('logs8-native','native'),('logs8-kotlin-player','kotlin'),('logs8-android','android'),('logs8-original-fontepref','original-fontepref'),('logs8-original-fontepref_file','original-fontepref_file'),('logs8-original-player_retido','original-player_retido')]],
 {'name':'Bind all successful receipts to the exact frozen source and identity','run':f'python3 {prefix}/aggregate.py --results review-results --output build/review/logs8/all-gates-verified.json'},
 {'uses':'actions/upload-artifact@v4','with':{'name':'logs8-all-gates-${{ github.sha }}','path':'build/review/logs8/all-gates-verified.json','if-no-files-found':'error'}}]}
out=here/a.output;out.write_text(json.dumps(workflow,indent=2)+'\n');print('Generated review-only workflow',out,'jobs',','.join(workflow['jobs']))
