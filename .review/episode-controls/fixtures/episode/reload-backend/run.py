#!/usr/bin/env python3
"""Host-only contracts against verbatim product API/helper bodies; no device claim."""
from pathlib import Path
import argparse, hashlib, json, subprocess

parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--cc',default='cc');parser.add_argument('--sanitize',action='store_true');a=parser.parse_args();root=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);here=Path(__file__).resolve().parent
def function(path, signature):
    s=(root/path).read_text(); start=s.index(signature); p=s.index('{',start)
    depth=0; state=''; i=p
    while i<len(s):
        c=s[i]; n=s[i:i+2]
        if state=='//':
            if c=='\n': state=''
        elif state=='/*':
            if n=='*/': state=''; i+=1
        elif state:
            if c=='\\': i+=1
            elif c==state: state=''
        elif n in ('//','/*'): state=n; i+=1
        elif c in ('"',"'"): state=c
        elif c=='{': depth+=1
        elif c=='}':
            depth-=1
            if not depth: return s[start:i+1]
        i+=1
    raise ValueError(signature)

common='''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "video.h"
#include "video_reconexao.h"
static int opened, openAccepted=1, savedPause, pauseCalls;
static NvReconexao recon;
static int reconPermitida=1, reconProxima=1, reconIniciou=1;
static int recarregarPausa=-1, pausaPedida=1;
static int audioAtual=0, legAtual=-1, reconAudio=-1, reconLeg=-1;
static int nAudio=1,nLeg=0,reconFaixasPend;
static char urlAtual[4096]="https://stream.invalid/signed?token=private";
static int video_reconectando(void){return recon.pendente;}
'''.replace('static int video_reconectando','int video_reconectando')

android=common+'''
#include <jni.h>
static int posNativoMs=1234, posMs=9000, ativo=1, prontoLoad=1, falhou, terminou, reconErroPend;
static int reconBuscarMs=-1,recarregarPosMs, nNovasA,nNovasL,legAte;
static VideoFaixa novasA[NV_FAIXA_MAX],novasL[NV_FAIXA_MAX],faixaAudio[NV_FAIXA_MAX],faixaLeg[NV_FAIXA_MAX];
static int savedAudio=-2,savedLeg=-2,preferred;
static int mVelocidade=1,mBuscar=2,velEnviada=100,velPedida=150,velRecusada,tocando,bufferando,reconErroCod;
static int seekCalls,seekTarget,speedTarget,retomada=1;
static unsigned tick=1000;
static unsigned SDL_GetTicks(void){return tick;}
double video_pos(void){return posMs/1000.0;}
int video_retomada_inicial_estado(void){return retomada;}
static void kInt(int method,int value){if(method==mBuscar){seekCalls++;seekTarget=value;}else if(method==mVelocidade)speedTarget=value;}
static void vel_log(int value,const char *text){(void)value;(void)text;}
static int erroDeRede(int code){return code>=2000&&code<=2002;}
static int ambienteOk=1;
static void *ambiente(void){return ambienteOk?(void*)1:NULL;}
static int abrirSessao(int ms,int fraction){assert(!fraction); opened++; posMs=0; prontoLoad=0;pausaPedida=0;velEnviada=100;savedPause=ms;return openAccepted;}
void video_pausar(int p){pauseCalls++;pausaPedida=p;}
void video_escolher_audio(int i){savedAudio=i;}
void video_escolher_legenda(int i){savedLeg=i;}
static void escolherAudioPreferido(void){preferred++;}
'''+function('src/video_android.c','int video_recarregar(void)')+'\n'+function('src/video_android.c','JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixasFim(')+'\n'+function('src/video_android.c','void video_bombear(void)')+'''
int main(void){
 recon.tentativa=2; recon.alvo=100; assert(video_recarregar()==1); assert(opened==1);
 assert(savedPause==1234 && recarregarPosMs==1234); assert(pauseCalls==1&&pausaPedida==1);
 assert(reconAudio==0 && reconLeg==-1 && reconFaixasPend==1);
 assert(recon.tentativa==2&&recon.alvo==100&&reconPermitida&&reconProxima&&reconIniciou);
 assert(!strcmp(urlAtual,"https://stream.invalid/signed?token=private")); assert(!video_recarregar());
 nNovasA=1;Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixasFim(NULL,NULL,0,-1);
 assert(savedAudio==0&&savedLeg==-1&&!preferred&&!reconFaixasPend);
 recarregarPausa=-1;prontoLoad=1; posNativoMs=0; pausaPedida=0; assert(video_recarregar());assert(savedPause==0);
 recarregarPausa=-1;prontoLoad=1; ativo=0;assert(!video_recarregar());ativo=1;
 prontoLoad=0;assert(!video_recarregar());prontoLoad=1;
 falhou=1;assert(!video_recarregar());falhou=0;
 terminou=1;assert(!video_recarregar());terminou=0;
 reconErroPend=1;assert(!video_recarregar());reconErroPend=0;
 recon.pendente=1;assert(!video_recarregar());recon.pendente=0;
 ambienteOk=0;assert(!video_recarregar());ambienteOk=1;
 posNativoMs=-1;assert(!video_recarregar());posNativoMs=100;
 openAccepted=0;assert(!video_recarregar()); assert(recarregarPausa==-1);
 openAccepted=1;prontoLoad=1;recarregarPausa=-1;nv_recon_zerar(&recon);posNativoMs=12345;pausaPedida=1;
 assert(video_recarregar());prontoLoad=1;video_bombear();assert(recarregarPausa==-1&&pausaPedida==1&&speedTarget==150&&!seekCalls);
 prontoLoad=1;posNativoMs=12345;assert(video_recarregar());retomada=-1;prontoLoad=1;video_bombear();assert(seekCalls==1&&seekTarget==12345&&pausaPedida==1);retomada=1;
 prontoLoad=1;assert(video_recarregar());reconPermitida=0;reconErroPend=1;reconErroCod=2001;video_bombear();assert(falhou&&recarregarPausa==-1&&!recon.tentativa);
 falhou=0;prontoLoad=1;reconPermitida=1;posNativoMs=12345;pausaPedida=1;assert(video_recarregar());
 for(int attempt=1;attempt<=NV_RECON_MAX;attempt++){
   reconErroPend=1;reconErroCod=2001;video_bombear();assert(recon.tentativa==attempt&&recon.pendente&&fabs(recon.alvo-12.345)<0.0001&&recarregarPausa==1);
   tick=recon.quando;video_bombear();assert(savedPause==12345&&pausaPedida==1&&!recon.pendente&&recarregarPausa==1);
 }
 reconErroPend=1;video_bombear();assert(falhou&&recon.esgotou&&recon.tentativa==NV_RECON_MAX&&recarregarPausa==-1);
 puts("Android API + actual pump: snapshots, tracks, pause/speed, accepted/refused resume, bounded preserved reconnect budget and fatal cleanup passed");
}
'''

webos=common+'''
static double posNativa=12.5,posAoCarregar;
static int ligado=1,pronto=1,falhou,terminou,dtsSessao,dtsModoDv,dtsNativePending,dtsEstado,dvSilencio,dvSegurar,recuperando,reconErroPend,semDVForcado;
static int audioAoCarregar=-1,legAoCarregar=-1;
static char midia[64]="media-1",legUrlAtual[1024]="https://subtitle.invalid/track.srt",legUrlAoCarregar[1024],reconLegUrl[1024];
static int tocarInterno(const char *url,int dv){assert(url==urlAtual);assert(dv);opened++;audioAtual=0;legAtual=-1;legUrlAtual[0]=0;recarregarPausa=-1;pausaPedida=0;return openAccepted;}
static int selectedAudio=-2,selectedLeg=-2,cronPediu,cronLoad,acb,bindPendente;
static double seekTarget,dvEntradaSeg;
static char externalRestored[1024];
void video_escolher_audio(int n){selectedAudio=n;}
void video_escolher_legenda(int n){selectedLeg=n;}
void video_legenda_externa(const char *u){snprintf(externalRestored,sizeof externalRestored,"%s",u);}
void video_pausar(int n){pauseCalls++;savedPause=n;}
void video_buscar(double n){seekTarget=n;}
static void seekAgora(double n){seekTarget=n;}
static void aplicarEstilo(void){}
static void dvSilenciarTv(void){}
static void marco(const char *s){(void)s;}
static unsigned long msDesdePedido(void){return 0;}
'''+function('src/video.c','static int recarregarMesmaFonte(')+'\n'+function('src/video.c','int video_recarregar(void)')+'\nstatic void loadCompleted(void){const char *p="loadCompleted";\n'+function('src/video.c','if (strstr(p, "loadCompleted"))')+'\n}\n'+'''
int main(void){
 recon.tentativa=2;recon.alvo=100;assert(video_recarregar());assert(opened==1&&posAoCarregar==12.5);
 assert(audioAoCarregar==0&&legAoCarregar==-1&&recarregarPausa==1&&pausaPedida==1);
 assert(!strcmp(legUrlAoCarregar,"https://subtitle.invalid/track.srt"));assert(!strcmp(reconLegUrl,legUrlAoCarregar));
 assert(recon.tentativa==2&&recon.alvo==100&&reconPermitida&&reconProxima&&reconIniciou);
 assert(!video_recarregar());loadCompleted();assert(recarregarPausa==-1&&seekTarget==12.5&&savedPause==1&&selectedAudio==0&&!strcmp(externalRestored,"https://subtitle.invalid/track.srt"));
 recarregarPausa=-1;posNativa=0;pausaPedida=0;assert(video_recarregar());assert(posAoCarregar==0&&recarregarPausa==0);loadCompleted();assert(seekTarget==0&&selectedLeg==-1&&recarregarPausa==-1);
 legAtual=2;posNativa=55.25;assert(video_recarregar());loadCompleted();assert(selectedLeg==2&&seekTarget==55.25&&recarregarPausa==-1);
 recarregarPausa=-1;dtsSessao=1;assert(!video_recarregar());dtsSessao=0;
 dtsNativePending=1;assert(!video_recarregar());dtsNativePending=0;
 dtsModoDv=1;assert(!video_recarregar());dtsModoDv=0;
 dtsEstado=VIDEO_DTS_PREPARANDO;assert(!video_recarregar());dtsEstado=VIDEO_DTS_CONVERTIDO;assert(!video_recarregar());dtsEstado=0;
 dvSilencio=1;assert(!video_recarregar());dvSilencio=0;dvSegurar=1;assert(!video_recarregar());dvSegurar=0;
 recuperando=1;assert(!video_recarregar());recuperando=0;
 reconErroPend=1;assert(!video_recarregar());reconErroPend=0;
 ligado=0;assert(!video_recarregar());ligado=1;
 pronto=0;assert(!video_recarregar());pronto=1;
 falhou=1;assert(!video_recarregar());falhou=0;
 terminou=1;assert(!video_recarregar());terminou=0;
 posNativa=NAN;assert(!video_recarregar());posNativa=-1;assert(!video_recarregar());posNativa=INT_MAX;assert(!video_recarregar());posNativa=1;
 openAccepted=0;assert(!video_recarregar());
 puts("webOS API + actual loadCompleted block: external/internal/Off, audio 0, zero/exact position, pause, converted-engine guards and unchanged budget passed");
}
'''

tizen=common+'''
enum{EST_POS,EST_DUR,EST_TOCANDO,EST_PRONTO,EST_ABERTO,EST_LARG,EST_ALT,EST_ERRO,EST_BUFPCT,EST_FIM,EST_N};
static int temAvplay=1,ativo=1,pronto=1,tocando,houveErro,terminou,faixasLidas,seekEm,reconNA,reconNL;
static int janX,janY,janW=1920,janH=1080,aberturaEm;
static double posSeg=999,reconBuscar=-1,actualPosition=12.25;
static char erroTexto[96];
static VideoFaixa reconFxA[NV_FAIXA_MAX],reconFxL[NV_FAIXA_MAX],faixaAudio[NV_FAIXA_MAX],faixaLeg[NV_FAIXA_MAX];
static unsigned tick=100;
static int selectedAudio=-2,selectedLeg=-2;
static double seekTarget;
static unsigned SDL_GetTicks(void){return tick;}
static void marco(const char *s){(void)s;}
void video_escolher_audio(int n){selectedAudio=n;}
void video_escolher_legenda(int n){selectedLeg=n;}
void video_buscar(double n){seekTarget=n;}
static void logarErro(const char *op){(void)op;}
static double avChamar(const char *cmd,const char *txt,double a,double b,double c,double d,char *dst,int len){
 (void)txt;(void)a;(void)b;(void)c;(void)d;(void)len;
 if(!strcmp(cmd,"estado")){double *s=(double*)dst;s[EST_PRONTO]=s[EST_ABERTO]=1;s[EST_POS]=actualPosition;return 1;}
 if(!strcmp(cmd,"abrir")){opened++;assert(!strcmp(txt,urlAtual));return openAccepted;}
 if(!strcmp(cmd,"buscar"))seekTarget=a;
 if(!strcmp(cmd,"pausar")){pauseCalls++;savedPause=a;}
 return 1;
}
#define AVS(cmd,s) avChamar(cmd,s,0,0,0,0,NULL,0)
#define AVN(cmd,n) avChamar(cmd,NULL,n,0,0,0,NULL,0)
'''+function('src/video_tizen.c','static int reabrirMesmaFonte(')+'\n'+function('src/video_tizen.c','int video_recarregar(void)')+'\n'+function('src/video_tizen.c','void video_pausar(int pausado)')+'\nstatic void restoreTracks(void){\n'+function('src/video_tizen.c','if (reconFaixasPend)')+'\n}\nstatic void prepared(void){\n'+function('src/video_tizen.c','if (reconBuscar >= 0.0 && pronto)')+'\n'+function('src/video_tizen.c','if (recarregarPausa >= 0 && (pronto || houveErro || terminou))')+'\n}\nstatic void autoRetry(void){int estavaPronto=pronto;\n'+function('src/video_tizen.c','if (temAvplay && nv_recon_vencida(&recon, SDL_GetTicks()))')+'\n(void)estavaPronto;\n}\n'+'''
int main(void){
 recon.tentativa=2;recon.alvo=100;assert(video_recarregar());assert(opened==1&&reconBuscar==12.25);
 assert(reconAudio==0&&reconLeg==-1&&recarregarPausa==1&&reconNA==1&&reconNL==0);
 assert(recon.tentativa==2&&recon.alvo==100&&reconPermitida&&reconProxima&&reconIniciou);
 assert(!video_recarregar());nAudio=1;restoreTracks();pronto=1;prepared();assert(selectedAudio==0&&selectedLeg==-1&&seekTarget==12250&&savedPause==1&&recarregarPausa==-1);
 recarregarPausa=-1;pronto=1;actualPosition=0;pausaPedida=0;assert(video_recarregar());assert(reconBuscar==0&&recarregarPausa==0);pronto=1;prepared();assert(seekTarget==0&&recarregarPausa==-1);
 actualPosition=22;pronto=1;pausaPedida=1;assert(video_recarregar());video_pausar(0);assert(recarregarPausa==0);pronto=1;prepared();assert(recarregarPausa==-1&&pausaPedida==0);
 recarregarPausa=-1;pronto=1;temAvplay=0;assert(!video_recarregar());temAvplay=1;
 ativo=0;assert(!video_recarregar());ativo=1;
 houveErro=1;assert(!video_recarregar());houveErro=0;
 terminou=1;assert(!video_recarregar());terminou=0;
 actualPosition=NAN;assert(!video_recarregar());actualPosition=-1;assert(!video_recarregar());actualPosition=INT_MAX;assert(!video_recarregar());actualPosition=1;
 openAccepted=0;assert(!video_recarregar());assert(houveErro);
 houveErro=0;nv_recon_zerar(&recon);assert(nv_recon_erro(&recon,1,tick,22));
 for(int attempt=2;attempt<=NV_RECON_MAX;attempt++){tick=recon.quando;autoRetry();assert(recon.tentativa==attempt&&recon.pendente&&!houveErro);}
 tick=recon.quando;autoRetry();assert(houveErro&&recon.esgotou&&recon.tentativa==NV_RECON_MAX);
 puts("Tizen API + actual restore/retry blocks: URL/position, audio0/Off, pause change during reload, failed automatic opens exhaust existing budget passed");
}
'''

records=[]
for name,body in [('android',android),('webos',webos),('tizen',tizen)]:
    src=out/(name+'-contracts.c');src.write_text(body)
    for san in [a.sanitize]:
        exe=out/(name+('-san' if san else ''))
        cmd=[a.cc,'-std=gnu11','-g','-O1','-Werror=implicit-function-declaration','-I'+str(root/'src'),'-I'+str(here.parent/'sdk'),str(src),'-lm','-o',str(exe)]
        if san:cmd[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize-recover=all']+([]if __import__('sys').platform=='darwin'else['-fno-pie','-no-pie'])
        build=subprocess.run(cmd,text=True,capture_output=True);(out/(exe.name+'-build.log')).write_text(build.stdout+build.stderr)
        assert build.returncode==0,(name,build.stderr)
        run=subprocess.run([str(exe)],text=True,capture_output=True,timeout=10);(out/(exe.name+'-run.log')).write_text(run.stdout+run.stderr)
        assert run.returncode==0,(name,san,run.stdout,run.stderr)
        records.append({'target':name,'sanitizers':san,'passed':True})
receipt={'scope':'Host-only verbatim API/helper/callback/pump/retry block contracts with recording native boundaries; no real device execution','records':records,'compiler':subprocess.check_output([a.cc,'--version'],text=True).splitlines()[0],'runner_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'source_sha256':{p:hashlib.sha256((root/'src'/p).read_bytes()).hexdigest() for p in ['video.h','video.c','video_android.c','video_tizen.c','video_tpk.c']}}
(out/'contracts-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'passed':len(records),'receipt':str(out/'contracts-receipt.json')}))
