#!/usr/bin/env python3
"""Publish only new, independently validated metadata branches with GITHUB_TOKEN."""
import hashlib, json, os, pathlib, re, urllib.error, urllib.request

REPO = 'ghbarker/nuvio-native-legacy'
BASE = 'c2e50a9325719ffae85005c0542338f6b44878c3'
ALLOWED = {
 '524': ('fix/series-metadata-524-20261016', 'd91f706c2ee5f10e6a8e65ba511131f57f33c7ab', '36f79fd813be5e18e8fa31f62c8ca65d003d7dfe', '26e6c2a96d7938a584d7372c71e3f1e1a02df659', 117143688366),
 '512': ('fix/trakt-episode-ids-512-20261016', 'b53143cc76624a857ad8c1b2bfc8603bf0d1477a', '8d1f25e31dab32d45fd8e2a8125a346f7c21e29f', 'aa2f1747ef9af6f91735fd4cd5e502a2733f5ac3', 117146979325)}
# Each selected candidate keeps its own actual validation origin.
ORIGINS = {
 '524': ('12ba833134f7c554024beb30fad3a4460d3e022c', 38951486690, 1, 'bc66b30b903710732da6c794eb96b69d6eef38cf476b65997f951671df0d3057', 'metadata-current'),
 '512': ('9702aea28ec915545507830cb90d5fcb31415b8b', 38952462207, 1, '0b63de2a722d11f3e7ce7fb535e8dc9346a8f8eed2bfc6cc93138a25eba233e4', 'metadata512-recovery')}
PREFIX = '/repos/' + REPO
PLAN = pathlib.Path(__file__).with_name('plan.json')
def need(ok, message):
 if not ok: raise RuntimeError(message)
def api(method, suffix, body=None, missing=False):
 if method != 'GET':
  allowed = any((method == 'POST' and suffix == '/git/refs' and body == {'ref':'refs/heads/'+v[0], 'sha':BASE}) or
                (method == 'PATCH' and suffix == '/git/refs/heads/'+v[0] and body == {'sha':v[1], 'force':False}) for k,v in ALLOWED.items() if k in selected)
  need(allowed, 'write outside exact ref allowlist')
 request = urllib.request.Request('https://api.github.com'+PREFIX+suffix,
  data=None if body is None else json.dumps(body).encode(), method=method,
  headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'], 'Accept':'application/vnd.github+json', 'X-GitHub-Api-Version':'2022-11-28'})
 try:
  with urllib.request.urlopen(request, timeout=30) as response: return json.load(response)
 except urllib.error.HTTPError as error:
  if missing and method == 'GET' and error.code == 404: return None
  raise RuntimeError('GitHub API '+method+' '+suffix+' failed HTTP '+str(error.code)) from None

plan = json.loads(PLAN.read_text())
need(os.environ.get('PUBLISH_ALLOWED') == 'true' and plan['root_reviewed'] is True and plan['publication_permitted'] is True, 'publication gates closed')
need(os.environ['GITHUB_REPOSITORY'] == REPO and os.environ['GITHUB_REF'] == 'refs/heads/review/publish-metadata-branches-20261016', 'wrong repository/review branch')
selected = plan['selected_candidates']
need(selected and len(set(selected)) == len(selected) and set(selected) <= set(ALLOWED), 'invalid selection')
need(plan['base'] == BASE, 'wrong base')
review = api('GET', '/git/commits/'+os.environ['GITHUB_SHA'])
need([p['sha'] for p in review['parents']] == [BASE, ALLOWED['524'][1], ALLOWED['512'][1]], 'wrong publication review parents')
proofs = []
for key in selected:
 branch, head, product, tree, job_id = ALLOWED[key]
 CI_REVIEW, RUN, ATTEMPT, MANIFEST, artifact_prefix = ORIGINS[key]
 run = api('GET', '/actions/runs/'+str(RUN))
 need(run['head_sha'] == CI_REVIEW and run['run_attempt'] == ATTEMPT and run['repository']['full_name'] == REPO, 'wrong selected CI run origin')
 entry = plan['candidates'][key]
 raw = entry['root_verified_receipt_json']
 need(isinstance(raw,str) and hashlib.sha256(raw.encode()).hexdigest() == entry['root_verified_receipt_sha256'], 'missing/mismatched root receipt')
 receipt = json.loads(raw); binding = receipt['source_binding']
 need(receipt['outcome'] == 'passed' and receipt['independent_consumer'] is True, 'unverified receipt')
 need(receipt['issue'] == int(key) and receipt['product_commit'] == product and receipt['separate_public_test_commit'] == head, 'wrong product/test proof')
 expected = {'verified':True,'source_clean':True,'candidate_pr':int(key),'source_commit':head,'tree':tree,'source_parents':[product],
  'base':BASE,'review_commit':CI_REVIEW,'workflow_run_id':RUN,'workflow_run_attempt':ATTEMPT,'manifest_sha256':MANIFEST}
 need(all(binding.get(k)==v for k,v in expected.items()), 'wrong proof source/CI binding')
 need(receipt['actual_job_id']==job_id and receipt['actual_job_conclusion']=='success', 'wrong proof job')
 counts = {'groups':10 if key=='524' else 6,'compiler_invocations':18 if key=='524' else 6,'linked_tests':10 if key=='524' else 6,
           'full_app_compiles':670,'full_links':2,'production_rust':True,'scenario_executions':94 if key=='524' else 6}
 need(all(receipt['checks'].get(k)==v for k,v in counts.items()), 'incomplete public/full-app validation')
 need(re.fullmatch('[0-9a-f]{64}',receipt['consumer_sha256']) and len(receipt['api_inputs_sha256'])>=3, 'missing independent/API provenance')
 if key=='524':
  negative = receipt['negative_control']
  need(negative['outcome']=='expected_behavioral_red' and negative['positive_group'] is False and negative['baseline_commit']==BASE, 'missing actual baseline control')
 job = api('GET', '/actions/jobs/'+str(job_id))
 need(job['status']=='completed' and job['conclusion']=='success' and job['head_sha']==CI_REVIEW and job['run_id']==RUN and job['run_attempt']==ATTEMPT, 'native job not successful at approved origin')
 artifact = api('GET', '/actions/artifacts/'+str(receipt['artifact_id']))
 name = artifact_prefix+'-'+key+'-native-proof-'+CI_REVIEW+'-attempt'+str(ATTEMPT)
 need(not artifact['expired'] and artifact['name']==name and artifact['workflow_run']['id']==RUN and artifact['workflow_run']['head_sha']==CI_REVIEW and
      artifact['digest']=='sha256:'+receipt['artifact_zip_sha256'] and artifact['size_in_bytes']==receipt['artifact_bytes']>0, 'wrong official proof artifact')
 test = api('GET', '/commits/'+head); prod = api('GET', '/commits/'+product)
 need([p['sha'] for p in test['parents']]==[product] and [p['sha'] for p in prod['parents']]==[BASE] and test['commit']['tree']['sha']==tree, 'not reviewed product/test ancestry')
 need(test['files'] and all(f['filename'].startswith('tests/') for f in test['files']), 'test commit contains product/workflow edits')
 need(prod['files'] and all(not f['filename'].startswith(('tests/','.github/')) for f in prod['files']), 'product commit contains tests/workflow edits')
 need(api('GET','/git/ref/heads/'+branch,missing=True) is None, 'branch already exists; refusing to modify it')
 proofs.append({'issue':int(key),'branch':branch,'head':head,'job':job_id,'root_receipt_sha256':entry['root_verified_receipt_sha256']})
for proof in proofs:
 branch, head = proof['branch'], proof['head']
 created=api('POST','/git/refs',{'ref':'refs/heads/'+branch,'sha':BASE})
 need(created['object']['sha']==BASE, 'unexpected seed result')
 advanced=api('PATCH','/git/refs/heads/'+branch,{'sha':head,'force':False})
 need(advanced['object']['sha']==head, 'unexpected fast-forward result')
 need(api('GET','/git/ref/heads/'+branch)['object']['sha']==head, 'published head verification failed')
pathlib.Path('metadata-branches-published.json').write_text(json.dumps({'outcome':'passed','repository':REPO,'publication_review':os.environ['GITHUB_SHA'],'base':BASE,'branches':proofs},indent=2)+'\n')
print(json.dumps({'outcome':'passed','branches':proofs}))
