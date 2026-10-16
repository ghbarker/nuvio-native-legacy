#!/usr/bin/env python3
"""Record only the explicit public cache action outcomes; no general env capture."""
from pathlib import Path
import argparse,json,os,re
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();records={}
for name in ('native','gradle'):
 stem='APK20031_'+name.upper()+'_CACHE_';keys={field:os.getenv(stem+suffix,'') for field,suffix in [('primary_key','PRIMARY'),('matched_key','MATCHED')]}
 for value in keys.values():assert re.fullmatch(r'[A-Za-z0-9._-]{0,512}',value),'Unexpected cache-key format'
 hit=os.getenv(stem+'HIT','');assert hit in ('','true','false')
 records[name]={**keys,'exact_hit':hit=='true','restored':bool(keys['matched_key'])}
report={'scope':'Public cache restore outputs only; restored native artifacts are still validated by unchanged production deps.sh recipe/NDK stamps on both layers','caches':records};a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
if os.getenv('GITHUB_STEP_SUMMARY'):
 with open(os.environ['GITHUB_STEP_SUMMARY'],'a') as f:
  f.write('Android dependency cache restoration\n\n')
  for name,r in records.items():f.write('- '+name+': '+('exact hit' if r['exact_hit'] else 'fallback restored' if r['restored'] else 'miss')+'; matched key `'+(r['matched_key'] or '(none)')+'`.\n')
print('Recorded native/Gradle cache hit and matched-key outcomes; production dependency validation remains enabled')
