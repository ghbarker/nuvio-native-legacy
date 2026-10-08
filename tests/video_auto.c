#include "video_auto.h"
#include <assert.h>
#include <stdio.h>
#define READY (VA_PRONTO|VA_BUSCAVEL)
int main(void){
  VideoAuto s;double pos=-1;videoauto_abrir(&s,10);
  assert(!videoauto_estado(&s,1,&pos));
  videoauto_relatar(&s,9,0,0,0,10000,READY,100);assert(!videoauto_estado(&s,100,&pos));
  videoauto_relatar(&s,10,0,0,0,10000,READY,100);assert(videoauto_estado(&s,100,&pos)==1&&pos==10);
  assert(!videoauto_estado(&s,1101,&pos)); // silencio/callback parado
  unsigned seek=videoauto_pedir(&s,200,1);assert(!videoauto_estado(&s,200,&pos));
  videoauto_relatar(&s,10,0,0,1,119000,READY,201);assert(!videoauto_estado(&s,201,&pos)); // tick pre-request
  videoauto_relatar(&s,10,seek,seek,0,119000,READY,202);assert(!videoauto_estado(&s,202,&pos)); // seekTo optimista
  videoauto_relatar(&s,10,seek,seek,1,119250,READY,450);assert(videoauto_estado(&s,450,&pos)==1&&pos==119.25);
  // Reabrir invalida a evidencia antiga, mas nao reabre um seek confirmado.
  videoauto_relatar(&s,10,seek,seek,0,119250,0,460);assert(!videoauto_estado(&s,460,&pos));
  videoauto_relatar(&s,10,seek,seek,0,119500,READY,480);assert(videoauto_estado(&s,480,&pos)==1);
  { VideoAuto retry=s;
    videoauto_relatar(&retry,10,seek,seek,0,119250,0,500);
    videoauto_relatar(&retry,10,seek,seek,0,119500,READY,20000);
    assert(videoauto_estado(&retry,20000,&pos)==1); } // Seek confirmado nao vira timeout no retry
  unsigned newer=videoauto_pedir(&s,500,1);
  videoauto_relatar(&s,10,newer,seek,1,119500,READY,501);assert(!videoauto_estado(&s,501,&pos)); // ACK seek antigo
  unsigned pause=videoauto_pedir(&s,510,0);s.pausado=1;
  videoauto_relatar(&s,10,newer,newer,1,120000,READY,511);assert(!videoauto_estado(&s,511,&pos)); // controle antigo
  videoauto_relatar(&s,10,pause,seek,1,120000,READY,512);assert(!videoauto_estado(&s,512,&pos));
  unsigned resume=videoauto_pedir(&s,520,0);s.pausado=0;
  videoauto_relatar(&s,10,resume,seek,1,120000,READY,521);assert(!videoauto_estado(&s,521,&pos)); // Retomar nao confirma o seek
  videoauto_relatar(&s,10,resume,newer,1,50000,READY,750);assert(videoauto_estado(&s,750,&pos)==1&&pos==50);
  newer=videoauto_pedir(&s,800,1);
  videoauto_relatar(&s,10,newer,newer,0,99000,READY,10801);assert(!videoauto_estado(&s,10801,&pos)); // timeout
  pause=videoauto_pedir(&s,10802,0);
  videoauto_relatar(&s,10,pause,seek,1,99000,READY,10803);assert(!videoauto_estado(&s,10803,&pos));
  newer=videoauto_pedir(&s,10900,1);
  videoauto_relatar(&s,10,newer,newer,-1,99000,READY,10901);assert(!videoauto_estado(&s,10901,&pos)); // falha explicita
  videoauto_relatar(&s,10,newer,newer,1,70000,READY,11150);assert(videoauto_estado(&s,11150,&pos)==1); // seek manual novo recupera
  videoauto_relatar(&s,10,newer,newer,1,70000,VA_BUSCAVEL,11200);assert(!videoauto_estado(&s,11200,&pos)); // buffer/pausa
  videoauto_relatar(&s,10,newer,newer,1,70000,VA_PRONTO,11201);assert(!videoauto_estado(&s,11201,&pos)); // nao buscavel
  videoauto_relatar(&s,10,newer,newer,1,70000,READY|VA_LIVE,11202);assert(!videoauto_estado(&s,11202,&pos));
  newer=videoauto_pedir(&s,11300,1);
  videoauto_relatar(&s,10,newer,newer,0,120000,VA_FIM,11301);assert(videoauto_estado(&s,11301,&pos)==2); // Fim confirmado libera o controle
  videoauto_abrir(&s,11);
  videoauto_relatar(&s,10,newer,newer,1,120000,VA_FIM,11302);assert(!videoauto_estado(&s,11302,&pos)); // player antigo
  videoauto_relatar(&s,11,0,0,0,0,READY,11303);assert(videoauto_estado(&s,11303,&pos)==1);
  puts("video_auto: generation, control/seek ordering, pause, timeout, recovery, end passed");
  return 0;
}
