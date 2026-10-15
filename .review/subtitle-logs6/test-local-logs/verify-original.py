#!/usr/bin/env python3
"""Original report/view behavior is byte-checked; this never runs any uploader."""
import argparse,hashlib,json,re
from pathlib import Path

def function(text,name):
    match=re.search(r'^.*\b'+re.escape(name)+r'\s*\([^;]*?\)\s*\{',text,re.M)
    assert match, name
    start=match.start(); pos=match.end()-1; depth=0
    # C strings/comments cannot change brace nesting.
    token=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*|[{}]')
    for item in token.finditer(text,pos):
        if item.group()=='{': depth+=1
        elif item.group()=='}':
            depth-=1
            if not depth:return text[start:item.end()]
    raise AssertionError(name)

def digest(data):return hashlib.sha256(data).hexdigest()

def check(source):
    source=Path(source); contract=json.loads(Path(__file__).with_name('original-contract.json').read_text())
    for path,want in contract['files'].items():assert digest((source/path).read_bytes())==want,path
    for path,functions in contract['functions'].items():
        body=(source/path).read_text()
        for name,want in functions.items():assert digest(function(body,name).encode())==want,(path,name)
    avisos=(source/'src/avisos.c').read_text()
    assert 'NV_TEST_LOCAL_LOGS' not in avisos, 'Do not intercept original reports'
    assert 'NV_TEST_LOCAL_LOGS' not in (source/'src/registro.c').read_text(), 'Original live viewer preserved'
    settings=(source/'src/ajustes.c').read_text()
    assert 'int  ajustes_envio_auto(void)         { return lig(AJ_ENVIO_AUTO); }' in settings
    assert 'if (op == AJ_ENVIO_AUTO) return 1;' not in settings
    assert 'if (focoOp == AJ_VER_REGISTRO) { registro_abrir(); return; }' in settings
    assert 'if (focoOp == AJ_ENVIAR_LOG) { registro_envio_abrir(); return; }' in settings
    assert settings.count('AJ_TEST_LOG_DOWNLOAD, // isolated APK only, never persisted')==1
    assert settings.count('ACAO("Download logs")')==1
    assert 'AJ_TEST_LOG_VIEW' not in settings, 'Do not duplicate the original live viewer'
    local=(source/'android/app/src/main/java/space/nuvio/nativelegacy/TestLocalLogs.kt').read_text()
    assert 'ACTION_CREATE_DOCUMENT' in local and 'ACTION_SEND' not in local
    assert 'HttpURLConnection' not in local and '.openConnection(' not in local
    print('PASS original Send/report/crash/auto preference and live View source preserved; Download separate')
    return {'verified':True,'original_reporting_preserved':True,'original_live_viewer_preserved':True,'download_test_only':True}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('--output',type=Path);a=p.parse_args()
    result=check(a.source)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
