"""Bounded preprocessor audit of the actual four positional Settings tables."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess

parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--cc',default='cc');args=parser.parse_args()
SOURCE=args.source.resolve()
BASE='10e3d5803a9b9dd5c36775aab298ab0534fc982e'
OLD='f2a03e9797857bb0c1b9a671f2b32e85a536c545'

def git(*args):return subprocess.check_output(['git','-C',str(SOURCE),*args])
def extract(text,pattern):
    match=re.search(pattern,text,re.S);assert match,pattern
    return match.group(0)
def split_items(body):
    result=[];start=0;depth=0;quoted=False;escaped=False
    for i,char in enumerate(body):
        if quoted:
            if escaped:escaped=False
            elif char=='\\':escaped=True
            elif char=='"':quoted=False
        elif char=='"':quoted=True
        elif char in '([{':depth+=1
        elif char in ')]}':depth-=1
        elif char==','and depth==0:
            value=body[start:i].strip()
            if value:result.append(value)
            start=i+1
    tail=body[start:].strip()
    if tail:result.append(tail)
    assert not quoted and depth==0
    return result
def tables(commit,macros):
    source=git('show',commit+':src/ajustes.c').decode()
    defaults=git('show',commit+':src/ajustes_ux_padrao.inc').decode()
    content='\n'.join([
        extract(source,r'typedef enum \{.*?\n\} OpcaoId;'),
        extract(source,r'static const Opcao OPCOES\[AJ_N\] = \{.*?\n\};'),
        extract(source,r'static const char \*CHAVE\[\] = \{.*?\n\};'),
        'static int valor[] = {\n'+defaults+'\n};'])
    command=[args.cc,'-E','-P','-x','c',*[('-D'+m)for m in macros],'-']
    preprocessed=subprocess.check_output(command,input=content.encode()).decode()
    specs=[r'typedef enum \{(.*?)\n\} OpcaoId;',r'static const Opcao OPCOES\[AJ_N\] = \{(.*?)\n\};',r'static const char \*CHAVE\[\] = \{(.*?)\n\};',r'static int valor\[\] = \{(.*?)\n\};']
    items=[split_items(re.search(p,preprocessed,re.S).group(1))for p in specs]
    assert items[0].pop()=='AJ_N'
    assert len({len(v)for v in items})==1,[len(v)for v in items]
    return dict(zip(items[0],zip(*items[1:])))

head=git('rev-parse','HEAD').decode().strip();modes=[]
for label,macros in [('native',[]),('native-touch',['NV_TOUCH_UI']),('android',['NV_ANDROID']),('android-touch',['NV_ANDROID','NV_TOUCH_UI'])]:
    base=tables(BASE,macros);candidate=tables(head,macros)
    assert list(candidate)==list(base)+['AJ_COLECAO_EXIBICAO']
    assert all(candidate[name]==entry for name,entry in base.items())
    row,key,default=candidate['AJ_COLECAO_EXIBICAO']
    assert row=='ESC("Exibição padrão das coleções", V_COLECAO_EXIBICAO, VT_LAY_N)'
    assert key=='"colecaoExibicaoPadraoLocal"'and default=='0'
    download='AJ_BAIXAR_REGISTRO'
    assert (download in candidate)==('NV_ANDROID'in macros)
    if download in candidate:
        assert list(candidate)[-2:]==[download,'AJ_COLECAO_EXIBICAO']
        assert candidate[download]==('ACAO("Baixar registro")','"-baixarRegistro"','0')
    modes.append({'mode':label,'macros':macros,'upstream_count':len(base),'candidate_count':len(candidate),'all_upstream_rows_keys_defaults_preserved':True,'collection_row_key_default_aligned':True,'android_download_slot_preserved':True})
core={'src/addons.c': '7616b62c44238ded48c7dd77240f2e505cb4354033717e49db3e1b61ec044fec', 'src/addons.h': '66c9c48d57e952faabcc9e99fc9d941436b95e90313b76b3ca5a90446ac686f4', 'src/descoberta.c': '0f676953f03c41632a524b7c55e3ba60f146c06ccd8520fd317115b1582a56c8', 'src/descoberta.h': '6fe6cacc35256df87be1719545dc13e849324a1167f5ab80786ecedd620cc941', 'src/vertudo.c': '4a319403dfcb1f8651a2f65e175f25f47bc5ae244720c838bfa03950c0ab99fe', 'src/vertudo.h': '1d6d3723df795cc05805d8042d45b87a52e1b5c90b9c9d008579794bb54a9706', 'tests/colecao_exibicao.c': '1fe18e742611f5b3dbd298f6e2cb31673331b774ed11de8c6f351e8df6ddce57'}
for path,expected in core.items():
    assert hashlib.sha256(git('show',head+':'+path)).hexdigest()==expected,path
assert git('diff','--name-only',BASE,head,'--','.github','android','tools','deploy')==b''
assert git('diff','--name-only',BASE,head,'--','tests').decode().splitlines()==['tests/colecao_exibicao.c']
result={'passed':True,'source_commit':head,'tree':git('rev-parse','HEAD^{tree}').decode().strip(),'base':BASE,'old_head':OLD,'compiler_operation':'preprocessor only; no C compilation or link','modes':modes,'tested_collection_source_and_original_test_byte_identical':core,'protected_trees_unchanged_against_latest_master':True,'only_previously_reviewed_test_delta':True}
out=args.output;out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
