#!/usr/bin/env python3
"""Cheap reviewed correction/source pin checks before either retry job does work."""
from pathlib import Path
import hashlib,json,os,re,subprocess
HERE=Path(__file__).resolve().parent;ORIGINAL=HERE.parent/'apk20021'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 lock=json.loads((HERE/'retry-lock.json').read_text());assert lock['frozen'] is True and lock['root_reviewed'] is True
 assert sha(HERE/'retry-lock.json')==os.environ['APK20021_RETRY_LOCK_SHA256']
 assert os.environ['GITHUB_REPOSITORY']==lock['repository']=='ghbarker/nuvio-native-legacy'
 assert sha(ORIGINAL/'plan.json')==lock['plan_sha256']==os.environ['NUVIO_REVIEW_PLAN_SHA256']
 plan=json.loads((ORIGINAL/'plan.json').read_text());assert plan['frozen'] and plan['root_reviewed']
 assert plan['ci_execution_permitted'] and sha(HERE/'original-plan.json')==lock['original_plan_sha256']
 old=json.loads((HERE/'original-plan.json').read_text());assert old['frozen'] and old['root_reviewed']
 assert old['sources']==plan['sources'] and old['bundle']==plan['bundle'] and old['composition_manifest']==plan['composition_manifest']
 for layer,e in lock['sources'].items():assert e['commit']==plan['sources'][layer]['commit'] and e['tree']==plan['sources'][layer]['tree']
 for name,expected in lock['original_script_sha256'].items():assert sha(ORIGINAL/name)==expected,name
 for name,expected in lock['retry_script_sha256'].items():assert sha(HERE.parent/name)==expected,name
 assert len(lock['retained'])==6 and len({e['job_id']for e in lock['retained']})==6
 for e in lock['retained']:
  assert e['actual_success_verified'] is True and re.fullmatch('[0-9a-f]{64}',e['artifact']['sha256']) and e['job_id']!=lock['excluded_failed_feature']['job_id'] and e['artifact']['id']!=lock['excluded_failed_feature']['artifact_id']
 assert subprocess.check_output(['git','-C',str(HERE.parents[1]),'rev-parse','HEAD'],text=True).strip()==os.environ['GITHUB_SHA']
 print('PASS exact unchanged source graph, independent plan contexts and approved real-producer closure correction')
if __name__=='__main__':main()
