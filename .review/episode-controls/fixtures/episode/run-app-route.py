#!/usr/bin/env python3
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser();p.add_argument('--cc',default='cc');p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--san',action='store_true');p.add_argument('--android',action='store_true');a=p.parse_args()
s=a.source.resolve();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);here=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
app=s/'src/app.c';original=app.read_text();before=sha(app)
helpers=original[original.index('static unsigned novaGeracaoFonte(void) {'):original.index('static int iniciarFonteJob(',original.index('static unsigned novaGeracaoFonte(void) {'))]
router=original[original.index('static struct { int tem, titulo, t, e, apontar;'):original.index('// AQUECER CONEXOES',original.index('static struct { int tem, titulo, t, e, apontar;'))]
reload=original[original.index('static void recarregarFonteVOD(void) {'):original.index('static void tentarProximaFonteVOD(void) {')]
(o/'app-route-extracted.inc').write_text(helpers+router+reload)
assert 'abrirProximoEpisodio(t,e,0);'in original and 'abrirProximoEpisodio(t,e,1);'in original
flags=[a.cc,'-D_DARWIN_C_SOURCE','-std=gnu11','-O1','-g','-ffunction-sections','-fdata-sections','-Werror=implicit-function-declaration','-include',str(here/'sdk-extra.h'),'-I'+str(here/'sdk'),'-I'+str(s/'src'),'-I'+str(o)]
if a.san:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer',*([]if __import__('sys').platform=='darwin'else['-fno-pie','-no-pie'])]
if a.android:flags+=['-DNV_ANDROID=1']
records=[]
for label,cmd in [('build',flags+[str(here/'app-route.c'),str(s/'src/perfis.c'),*(['-Wl,-dead_strip']if __import__('sys').platform=='darwin'else['-Wl,--gc-sections']),'-lm','-pthread','-o',str(o/'app-route')]),('run',[str(o/'app-route')])]:
 z=subprocess.run(cmd,capture_output=True,text=True);log=o/(label+'.log');log.write_text(z.stdout+z.stderr);records.append({'label':label,'command':cmd,'returncode':z.returncode,'log_sha256':sha(log)});assert not z.returncode,z.stdout+z.stderr
assert sha(app)==before
(o/'result.json').write_text(json.dumps({'outcome':'passed','source':str(s),'app_sha256':before,'actual_extracted_bytes_sha256':sha(o/'app-route-extracted.inc'),'fixture_sha256':sha(here/'app-route.c'),'real_profile_module_sha256':sha(s/'src/perfis.c'),'sanitized':a.san,'android':a.android,'records':records,'scope':'Byte-identical production app routing/cleanup helpers with typed media/worker/catalog boundaries and real profile ownership module. No live decoder/progress/backend assertion.'},indent=2)+'\n');print(z.stdout.strip())
