#!/usr/bin/env python3
"""After fresh exact-source checks, advance only the already authorized PR431 branch."""
from pathlib import Path
import argparse, json, os, subprocess, tempfile
from common import git, plan, sha, verify

def verify_evidence(directory, review_commit):
    p=plan();expected=p['source'];records=[]
    def load(pattern):
        files=list(Path(directory).glob(pattern));assert len(files)==1,(pattern,len(files))
        value=json.loads(files[0].read_text());assert value['outcome']=='passed'
        binding=value['source_binding']
        assert binding['source_commit']==expected['commit'] and binding['tree']==expected['tree']
        assert binding['review_commit']==review_commit and binding['manifest_sha256']==expected['manifest_sha256']
        records.append({'file':str(files[0].relative_to(directory)),'sha256':sha(files[0])})
        return value
    native=load('**/native-verified.json')
    assert {a['ui'] for a in native['apps']}=={'tv','touch'} and len(native['apps'])==2
    assert all(a['complete_translation_units']>100 for a in native['apps'])
    assert native['reused_test_passes'] is False
    android=load('**/android-verified.json')
    assert {a['abi'] for a in android['actual_arms']}=={'arm64-v8a','armeabi-v7a'}
    assert android['native_16kb_pages']=='passed' and android['zip_alignment']=='passed'
    assert android['source_restored_clean'] and android['private_logs_absent']
    assert android['package']=='space.nuvio.nativelegacy' and android['reused_test_passes'] is False
    runtime=load('**/runtime-verified.json')
    assert runtime['reused_test_passes'] is False
    assert runtime['settings_apply_cancel_persistence_cases']==48
    assert runtime['storage_error_geometry_cases']==12 and runtime['tv_notice_observations']==12
    assert runtime['mode_leaf_executions']==18
    assert runtime['normal_and_sanitizer_modes']==['normal','address+undefined']
    files=list(Path(directory).glob('**/original-asan-verified.json'))
    assert len(files)==3
    suites=set()
    for file in files:
        value=json.loads(file.read_text());assert value['outcome']=='passed'
        b=value['source_binding'];assert b['source_commit']==expected['commit'] and b['tree']==expected['tree']
        assert b['review_commit']==review_commit and b['manifest_sha256']==expected['manifest_sha256']
        assert value['script_unchanged'] and value['original_flags_unchanged']
        assert value['sanitizers']==['address','undefined']
        assert value['suite'] not in suites;suites.add(value['suite'])
        records.append({'file':str(file.relative_to(directory)),'sha256':sha(file)})
    assert suites=={'fontepref','fontepref_file','player_retido'}
    return records

def main():
    parser=argparse.ArgumentParser()
    for key in ['source','proof','evidence','output']:parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--check-only',action='store_true');a=parser.parse_args()
    p,proof,manifest=verify(a.source,a.proof);pub=p['publication']
    assert os.environ.get('GITHUB_REPOSITORY')==pub['repository']
    assert os.environ.get('GITHUB_REF')=='refs/heads/'+pub['review_branch']
    records=verify_evidence(a.evidence,os.environ['GITHUB_SHA'])
    # A true rebase rewrites the six commits. The old published head is an
    # explicit reviewed lease, not an ancestor requirement for the new head.
    assert pub['kind']=='reviewed-rebase'
    assert pub['required_base']==p['upstream']['commit']
    assert git(a.source,'merge-base','--is-ancestor',pub['required_base'],pub['exact_new'])==''
    commits=git(a.source,'rev-list','--reverse',pub['required_base']+'..'+pub['exact_new']).splitlines()
    assert commits==pub['expected_product_commits'] and len(commits)==6
    # The existing PR receives only the source object already tested above.
    assert git(a.source,'rev-parse','HEAD')==pub['exact_new']
    if a.check_only:
        print('All exact-source gate evidence verified; no remote branch request made');return
    token=os.environ.get('GH_TOKEN','');assert token,'Missing success-only branch publication token'
    remote='https://github.com/'+pub['repository']+'.git';ref='refs/heads/'+pub['branch']
    with tempfile.TemporaryDirectory(prefix='pr431-publish-auth-') as temp:
        ask=Path(temp)/'askpass';ask.write_text('#!/usr/bin/env python3\nimport os,sys\nprint("x-access-token" if "username" in sys.argv[1].lower() else os.environ["GH_TOKEN"])\n');ask.chmod(0o700)
        env=dict(os.environ,GIT_ASKPASS=str(ask),GIT_TERMINAL_PROMPT='0')
        def run(*args):return subprocess.check_output(['git','-C',str(a.source),*args],env=env,text=True,stderr=subprocess.STDOUT).strip()
        before=run('ls-remote',remote,ref).split()
        assert len(before)==2 and before[1]==ref and before[0]==pub['expected_old'],'PR branch changed; refusing publication'
        if pub['require_upstream_unchanged']:
            upstream_ref='refs/heads/'+p['upstream']['branch']
            upstream=run('ls-remote',p['upstream']['url'],upstream_ref).split()
            assert upstream==[pub['required_base'],upstream_ref],'Upstream advanced; refusing publication until the new base is reviewed'
        # Only the exact root-reviewed old head may be replaced. Never use
        # unconditional --force or infer a lease from a mutable remote-tracking ref.
        run('push','--force-with-lease='+ref+':'+pub['expected_old'],remote,pub['exact_new']+':'+ref)
        after=run('ls-remote',remote,ref).split()
        assert after==[pub['exact_new'],ref],'Published branch does not match the tested object'
    receipt={'outcome':'exact-validated-pr431-branch-updated','publication_kind':pub['kind'],'base':pub['required_base'],'repository':pub['repository'],'branch':pub['branch'],'old_commit':pub['expected_old'],'new_commit':pub['exact_new'],'tree':p['source']['tree'],'review_commit':os.environ['GITHUB_SHA'],'run_id':int(os.environ['GITHUB_RUN_ID']),'evidence':records,'tests_workflows_private_artifacts_added_to_product':False,'new_feature_pr_or_release_created':False}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(receipt,indent=2)+'\n')
    print('Updated only the existing PR431 source branch to',pub['exact_new'])

if __name__=='__main__':main()
