from pathlib import Path
import argparse, hashlib, json, platform, subprocess

p=argparse.ArgumentParser();p.add_argument('--baseline',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True);p.add_argument('--cc',default='cc');a=p.parse_args()
r=Path(__file__).resolve().parent;o=Path(a.output).resolve();o.mkdir(parents=True,exist_ok=True)
results={};commands=[];inputs={}
for label,s in [('baseline',a.baseline),('candidate',a.source)]:
 s=Path(s).resolve();binary=o/label
 cmd=[a.cc,'-O1','-g','-ffunction-sections','-fdata-sections','-DNV_ANDROID=1','-DNV_TOUCH_UI=1','-I'+str(r/'sdk'),'-I'+str(s/'src'),'-include',str(r/'sdk/pre.h'),'-DSETTINGS_SOURCE='+json.dumps(str(s/'src/ajustes.c')),str(r/'defaults.c'),'-Wl,-dead_strip'if platform.system()=='Darwin'else'-Wl,--gc-sections','-lm','-o',str(binary)]
 commands.append(cmd);z=subprocess.run(cmd,capture_output=True,text=True);(o/(label+'.compile.log')).write_text(z.stdout+z.stderr);assert z.returncode==0,z.stderr
 text=subprocess.check_output([str(binary)],text=True);(o/(label+'.txt')).write_text(text);results[label]=text.splitlines()
 inputs[label]={'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=s,text=True).strip(),'tree':subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=s,text=True).strip(),'sha256':{str(x.relative_to(s)):hashlib.sha256(x.read_bytes()).hexdigest()for x in [s/'src/ajustes.c',s/'src/text.h',*(s/'src').glob('ajustes*.inc')]}}
assert results['baseline'][0]==results['candidate'][0], 'Existing text-style IDs/count must stay unchanged'
assert inputs['baseline']['sha256']['src/text.h']==inputs['candidate']['sha256']['src/text.h']
baseline=[row.split('|')for row in results['baseline'][1:]]
candidate=[row.split('|')for row in results['candidate'][1:]]
assert len(baseline)==292 and len(candidate)==310
assert all(len(row)==9 and int(row[0])==i for i,row in enumerate(baseline))
assert all(len(row)==9 and int(row[0])==i for i,row in enumerate(candidate))
intentional=[]
labels={'legenda2CorLocal':'Cor','legenda2FundoLocal':'Fundo','legenda2BordaLocal':'Borda'}
for old,new in zip(baseline,candidate):
 expected=list(old)
 if old[1]=='legenda2TamanhoLocal':
  assert old[4]=='Tamanho da segunda legenda' and old[5]=='21'
  expected[1]='legenda2EscalaLocal';expected[4]='Tamanho';expected[5]='27'
 elif old[1]in labels:expected[4]=labels[old[1]]
 if old!=expected:intentional.append({'id':int(old[0]),'old_key':old[1],'new_key':expected[1]})
 assert new==expected,('Existing positional option changed unexpectedly',old,new)
keys=['-legenda1Tamanho','-legenda1Fonte','-legenda1Cor','-legenda1Opacidade','-legenda1Fundo','-legenda1Posicao','-legenda1Borda','-legenda1Atraso','-legenda1Negrito','-legenda1CorBorda','-legenda1Restaurar','legenda2FonteLocal','legenda2OpacidadeLocal','legenda2PosicaoLocal','legenda2AtrasoLocal','legenda2NegritoLocal','legenda2CorBordaLocal','-legenda2Restaurar']
defaults=[120,0,0,0,0,3,1,0,1,0,0]+[0]*7
assert [row[1]for row in candidate[292:]]==keys
assert [int(row[2])for row in candidate[292:]]==defaults
assert len(intentional)==4
receipt={'passed':True,'old_settings_unchanged':292,'added_settings':18,'existing_ids_and_defaults_preserved':True,'intentional_existing_label_and_size_key_changes':intentional,'old_style_ids_preserved_append_only':True,'added_styles':0,'inputs':inputs,'commands':commands,'proof':'Compiled actual baseline/candidate Settings arrays; all292 pre-existing IDs/defaults/types/ranges remain, except the reviewed secondary size key/range and four secondary labels. Exactly18 subtitle rows append. Text-style header and count unchanged.'}
(o/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,ensure_ascii=False))
