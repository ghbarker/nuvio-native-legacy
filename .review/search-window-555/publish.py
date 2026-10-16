#!/usr/bin/env python3
"""Publish one new, independently validated public source branch using GITHUB_TOKEN."""
import hashlib, json, os, pathlib, re, urllib.error, urllib.request
REPO = 'ghbarker/nuvio-native-legacy'
BASE = 'c2e50a9325719ffae85005c0542338f6b44878c3'
BRANCH = 'fix/search-result-window-555-20261016'
REVIEW_BRANCH = 'review/publish-search-window-555-20261016'
PRODUCT = '811767dbaac62669270103b0f61340c7f0699faa'
PRODUCT_TREE = 'b8f39fd5f323063bae8c8762fc30bcddba03a4ab'
TEST_TREE = '3bde79c5b50645c932c05386a4eb46b756104d8a'
HEAD = 'dba6ab8c03cf25a301054a02d01ce0d146efdad3'
CI_REVIEW = '83a5e02114f734a6d910af9f5de8db4a736047d5'
RUN = 38957911030
ATTEMPT = 1
JOB = 117165329229
MANIFEST = '4ed56d94a61903e02866b3e7746a30649ba9ef1a52510063072effcd141e81dd'
CI_PLAN = '56641c4d3bb4022c0b1e84122369bffaa27216b6c7b6950a66f43abc13f0e625'
ARTIFACT_PREFIX = 'issue555-final'
PRODUCT_PATHS = {'src/busca.c','src/descoberta.c','src/descoberta.h','src/spotlight.c'}
TEST_PATHS = {'tests/busca_limite.c','tests/busca_limite.sh','tests/busca_toque_modo.c','tests/busca_toque_modo.sh','tests/detalheanime.c','tests/farah.c','tests/fixtures/busca_bigg_boss.json','tests/spotlight_regressao.c','tests/spotlight_regressao.sh'}
PREFIX = '/repos/' + REPO
PLAN = pathlib.Path(__file__).with_name('plan.json')
def need(ok, message):
 if not ok: raise RuntimeError(message)
def api(method, suffix, body=None, missing=False):
 if method != 'GET':
  need((method == 'POST' and suffix == '/git/refs' and body == {'ref':'refs/heads/'+BRANCH,'sha':BASE}) or
       (method == 'PATCH' and suffix == '/git/refs/heads/'+BRANCH and body == {'sha':HEAD,'force':False}), 'write outside single exact non-force ref allowlist')
 request=urllib.request.Request('https://api.github.com'+PREFIX+suffix,
  data=None if body is None else json.dumps(body).encode(),method=method,
  headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28'})
 try:
  with urllib.request.urlopen(request,timeout=30) as response:return json.load(response)
 except urllib.error.HTTPError as error:
  if missing and method=='GET' and error.code==404:return None
  raise RuntimeError('GitHub API '+method+' '+suffix+' failed HTTP '+str(error.code)) from None

plan=json.loads(PLAN.read_text())
need(os.environ.get('PUBLISH_ALLOWED')=='true' and plan['root_reviewed'] is True and plan['publication_permitted'] is True,'publication gates closed')
need(os.environ['GITHUB_REPOSITORY']==REPO and os.environ['GITHUB_REF']=='refs/heads/'+REVIEW_BRANCH,'wrong repository/review branch')
need(isinstance(HEAD,str) and re.fullmatch('[0-9a-f]{40}',HEAD) and all(x is not None for x in [CI_REVIEW,RUN,ATTEMPT,JOB,MANIFEST,CI_PLAN]),'source/CI facts pending')
need(plan['base']==BASE and plan['branch']==BRANCH and plan['source_commit']==HEAD and plan['product_commit']==PRODUCT and plan['source_tree']==TEST_TREE,'wrong frozen public source')
repo=api('GET','');need(repo['full_name']==REPO and repo['fork'] is True,'not the approved fork')
review=api('GET','/git/commits/'+os.environ['GITHUB_SHA'])
need([p['sha'] for p in review['parents']]==[BASE,HEAD],'wrong publication review parents')
raw=plan['root_verified_receipt_json'];need(isinstance(raw,str) and hashlib.sha256(raw.encode()).hexdigest()==plan['root_verified_receipt_sha256'],'missing/mismatched root receipt')
receipt=json.loads(raw);binding=receipt['source_binding']
need(receipt['outcome']=='passed' and receipt['independent_consumer'] is True and receipt['issue']==555,'unverified receipt')
need(receipt['product_commit']==PRODUCT and receipt['separate_public_test_commit']==HEAD,'wrong separate product/test proof')
expected={'verified':True,'source_clean':True,'candidate_pr':555,'source_commit':HEAD,'tree':TEST_TREE,'source_parents':[PRODUCT],
 'base':BASE,'review_commit':CI_REVIEW,'workflow_run_id':RUN,'workflow_run_attempt':ATTEMPT,'manifest_sha256':MANIFEST}
need(all(binding.get(k)==v for k,v in expected.items()),'wrong proof source/CI binding')
need(plan['ci_origin']=={'review':CI_REVIEW,'run':RUN,'attempt':ATTEMPT,'job':JOB,'manifest_sha256':MANIFEST,'plan_sha256':CI_PLAN},'wrong frozen CI origin')
need(receipt['actual_job_id']==JOB and receipt['actual_job_conclusion']=='success','wrong proof job')
counts={'groups':8,'compiler_invocations':8,'linked_tests':8,'full_app_compiles':670,'full_links':2,'production_rust':True,'scenario_executions':8}
need(all(receipt['checks'].get(k)==v for k,v in counts.items()),'incomplete current public/full-app validation')
need(re.fullmatch('[0-9a-f]{64}',receipt['consumer_sha256']) and len(receipt['api_inputs_sha256'])>=3,'missing independent/API provenance')
run=api('GET','/actions/runs/'+str(RUN))
need(run['head_sha']==CI_REVIEW and run['run_attempt']==ATTEMPT and run['repository']['full_name']==REPO,'wrong actual CI run origin')
job=api('GET','/actions/jobs/'+str(JOB))
need(job['status']=='completed' and job['conclusion']=='success' and job['head_sha']==CI_REVIEW and job['run_id']==RUN and job['run_attempt']==ATTEMPT,'approved native job not successful')
artifact=api('GET','/actions/artifacts/'+str(receipt['artifact_id']))
name=ARTIFACT_PREFIX+'-555-native-proof-'+CI_REVIEW+'-attempt'+str(ATTEMPT)
need(not artifact['expired'] and artifact['name']==name and artifact['workflow_run']['id']==RUN and artifact['workflow_run']['head_sha']==CI_REVIEW and
 artifact['digest']=='sha256:'+receipt['artifact_zip_sha256'] and artifact['size_in_bytes']==receipt['artifact_bytes']>0,'wrong official proof artifact')
test=api('GET','/commits/'+HEAD);product=api('GET','/commits/'+PRODUCT)
need([p['sha'] for p in test['parents']]==[PRODUCT] and [p['sha'] for p in product['parents']]==[BASE],'wrong product/test ancestry')
need(test['commit']['tree']['sha']==TEST_TREE and product['commit']['tree']['sha']==PRODUCT_TREE,'wrong public trees')
need({x['filename'] for x in test['files']}==TEST_PATHS and {x['filename'] for x in product['files']}==PRODUCT_PATHS,'public source/test scope changed')
need(api('GET','/git/ref/heads/'+BRANCH,missing=True) is None,'branch already exists; refusing to modify it')
created=api('POST','/git/refs',{'ref':'refs/heads/'+BRANCH,'sha':BASE});need(created['object']['sha']==BASE,'unexpected seed result')
advanced=api('PATCH','/git/refs/heads/'+BRANCH,{'sha':HEAD,'force':False});need(advanced['object']['sha']==HEAD,'unexpected fast-forward result')
need(api('GET','/git/ref/heads/'+BRANCH)['object']['sha']==HEAD,'published head verification failed')
result={'outcome':'passed','repository':REPO,'publication_review':os.environ['GITHUB_SHA'],'base':BASE,'branch':BRANCH,'head':HEAD,
 'product_commit':PRODUCT,'separate_public_test_commit':HEAD,'actual_native_job':JOB,'root_receipt_sha256':plan['root_verified_receipt_sha256'],
 'github_token_only':True,'force':False,'source_workflows_unchanged':True,'no_sdk_or_apk_or_release_actions':True}
pathlib.Path('search-window-555-published.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
