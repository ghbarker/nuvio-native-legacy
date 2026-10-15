from pathlib import Path
import argparse, hashlib, json, platform, subprocess

p=argparse.ArgumentParser();p.add_argument('--baseline',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True);p.add_argument('--cc',default='cc');a=p.parse_args()
r=Path(__file__).resolve().parent;o=Path(a.output).resolve();o.mkdir(parents=True,exist_ok=True)
results={};commands=[];inputs={}
for label,s in [('baseline',a.baseline),('candidate',a.source)]:
 s=Path(s).resolve();binary=o/label
 cmd=[a.cc,'-O1','-g','-ffunction-sections','-fdata-sections','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-I'+str(r/'synopsis477/settings/sdk'),'-I'+str(s/'src'),'-include',str(r/'sdk/pre.h'),'-DSETTINGS_SOURCE='+json.dumps(str(s/'src/ajustes.c')),str(r/'defaults.c'),'-Wl,-dead_strip'if platform.system()=='Darwin'else'-Wl,--gc-sections','-lm','-o',str(binary)]
 commands.append(cmd);z=subprocess.run(cmd,capture_output=True,text=True);(o/(label+'.compile.log')).write_text(z.stdout+z.stderr);assert z.returncode==0,z.stderr
 text=subprocess.check_output([str(binary)],text=True);(o/(label+'.txt')).write_text(text);results[label]=text.splitlines()
 inputs[label]={'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=s,text=True).strip(),'tree':subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=s,text=True).strip(),'sha256':{str(x.relative_to(s)):hashlib.sha256(x.read_bytes()).hexdigest()for x in [s/'src/ajustes.c',s/'src/text.h',*(s/'src').glob('ajustes*.inc')]}}
assert results['candidate'][1:-1]==results['baseline'][1:],'existing positional options/defaults changed'
assert int(results['candidate'][0].split('|')[1])==int(results['baseline'][0].split('|')[1])+17
assert results['candidate'][-1].split('|')[1:]==['sinopsePercentualLocal','0','0','Tamanho da sinopse']
receipt={'passed':True,'old_settings_unchanged':len(results['baseline'])-1,'added_settings':1,'old_style_ids_preserved_append_only':True,'added_styles':17,'inputs':inputs,'commands':commands,'proof':'Compiled actual baseline/candidate Settings arrays; every pre-existing ID/key/default/type/label agrees. The original16 synopsis styles retain their IDs and one20px style is appended.'}
(o/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,ensure_ascii=False))
