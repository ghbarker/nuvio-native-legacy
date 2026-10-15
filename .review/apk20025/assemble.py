#!/usr/bin/env python3
"""Assemble review-only CI; executable approval remains an explicit parent action."""
from pathlib import Path
import argparse,hashlib,json
r=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=r/'apk20025-review.yml');a=p.parse_args()
plan=json.loads((r/'plan.json').read_text());pin=hashlib.sha256((r/'plan.json').read_bytes()).hexdigest()
checkout={'uses':'actions/checkout@v4','with':{'persist-credentials':False,'fetch-depth':0}}
root='.review/apk20025';out='build/review/apk20025'
def materialize(layer):return {'name':'Materialize exact approved '+layer+' source','run':f'python3 {root}/materialize.py --layer {layer} --output source-{layer} --proof {out}/{layer}-source.json --base-repository "$GITHUB_WORKSPACE"'}
setup={'name':'Install real SDKs and capture system driver before wrappers','run':'brew install sdl2 sdl2_image sdl2_ttf sdl3\nreview_driver="$(command -v cc)"\ntest "$review_driver" = /usr/bin/cc\necho "NUVIO_REVIEW_REAL_CC=$review_driver" >> "$GITHUB_ENV"\n"$review_driver" --version\n'}
def upload(name):return {'uses':'actions/upload-artifact@v4','if':'always()','with':{'name':name+'-${{ github.sha }}','path':out+'/','if-no-files-found':'error','retention-days':14}}
native={'name':'Complete macOS TV/Mobile + actual changed native contracts','runs-on':'macos-14','timeout-minutes':90,'steps':[checkout,materialize('product'),materialize('test'),setup,{'name':'Install reader metric dependency in isolated Python environment','run':'python3 -m venv "$RUNNER_TEMP/apk20025-python"\n"$RUNNER_TEMP/apk20025-python/bin/python" -m pip install fonttools==4.60.1\necho "$RUNNER_TEMP/apk20025-python/bin" >> "$GITHUB_PATH"'}, {'name':'Fresh SDK sanity, actual contracts and four complete application links','run':f'python3 {root}/native/run.py --product source-product --test source-test --product-proof {out}/product-source.json --test-proof {out}/test-source.json --cc "$NUVIO_REVIEW_REAL_CC" --output {out}/native'},upload('apk20025-native')]}
features={'name':'Focused caption/libass and collection source contracts','runs-on':'ubuntu-24.04','timeout-minutes':30,'steps':[checkout,materialize('product'),materialize('test'),{'name':'Install proven actual libass and fixture compiler dependencies','run':'sudo apt-get update -qq\nsudo apt-get install -y build-essential pkg-config libass-dev python3-fonttools fonts-dejavu-core\n/usr/bin/cc --version\npkg-config --modversion libass'}, {'name':'Fresh actual combined caption/style and collection contracts, normal and ASan/UBSan','run':f'python3 {root}/feature-contracts/run.py --product source-product --test source-test --product-proof {out}/product-source.json --test-proof {out}/test-source.json --cc /usr/bin/cc --output {out}/feature-contracts'},upload('apk20025-features')]}
jobs={'native':native,'feature-contracts':features}
for suite in ['fontepref','fontepref_file','player_retido']:
 steps=[checkout,materialize('product'),setup,{'name':'Compile and run real system SDK sanity before full-source suites','run':f'python3 {root}/sdk_sanity.py --output {out}/sdk-{suite}'}]
 for mode in ['normal','asan']:
  steps.append({'name':'Original full-source '+suite+' '+mode,'run':f'python3 {root}/original3/run.py --source source-product --source-proof {out}/product-source.json --suite {suite} --wrapper {root}/original3/cc_rpath.py --mode {mode} --output {out}/original-{suite}-{mode}'})
 steps.append(upload('apk20025-original-'+suite))
 jobs['original-'+suite]={'name':'Original complete '+suite+' normal + ASan/UBSan','runs-on':'macos-14','timeout-minutes':70,'steps':steps}
android=json.loads((r/'android/job-fragment.json').read_text());assert android['env'].pop('NUVIO_REVIEW_PLAN_SHA256')=='PENDING_ROOT_REVIEW';jobs['android']=android
jobs['jvm']=json.loads((r/'jvm/job-fragment.json').read_text())
needs=list(jobs)
jobs['all-gates']={'name':'Require every fresh source-bound gate','runs-on':'ubuntu-24.04','needs':needs,'if':'always()','timeout-minutes':15,'env':{'APK20025_JOB_RESULTS':'${{ toJSON(needs) }}'},'steps':[checkout,{'uses':'actions/download-artifact@v4','with':{'pattern':'apk20025-*-${{ github.sha }}','path':'gate-artifacts','merge-multiple':False}},{'name':'Verify actual receipts, six original modes and pristine compatible APK','run':f'python3 {root}/aggregate.py --artifacts gate-artifacts --output {out}/all-gates-verified.json --job-results-json "$APK20025_JOB_RESULTS"'},upload('apk20025-all-gates')]}
w={'name':'APK20025 exact integrated source review','run-name':'APK20025 full fresh gates ${{ github.sha }}','on':{'push':{'branches':['review/apk20025-20261015'],'paths':['.review/apk20025/**','.github/workflows/apk20025-review.yml']},'workflow_dispatch':{}},'permissions':{'contents':'read','actions':'read'},'concurrency':{'group':'apk20025-${{ github.ref }}','cancel-in-progress':False},'env':{'NUVIO_REVIEW_PLAN_SHA256':pin,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'},'jobs':jobs}
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(w,indent=2)+'\n')
print(json.dumps({'workflow':str(a.output),'plan_sha256':pin,'execution_approved':plan['frozen'] and plan['root_reviewed'],'jobs':list(jobs),'heavy_parallel':['feature-contracts','native','original-fontepref','original-fontepref_file','original-player_retido','android']}))
