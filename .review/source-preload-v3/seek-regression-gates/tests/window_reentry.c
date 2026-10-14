// Reproduce the unchanged tests/posplay_recuo.c first scenario using the
// selected tree's actual posplay.c and stable synthetic catalog boundaries.
#define main existing_posplay_window_main
#include "next_preload_posplay.c"
#undef main

int main(int argc,char **argv) {
  const int expectBug=argc==2&&!strcmp(argv[1],"--expect-window-bug");
  duration=3600;credits=3500;reset();videoSetting=sourceSetting=0;
  posplay_marcador_real(1);
  posplay_atualizar(.016f,1000,3597,3600,1,0,1);
  assert(posplay_visivel());
  posplay_recuar();posplay_aguardar_busca(1);
  posplay_atualizar(.016f,6000,2000,3600,1,0,0);
  posplay_atualizar(.016f,7000,3600,3600,1,0,1);
  posplay_aguardar_busca(0);
  posplay_atualizar(.016f,8000,3600,3600,1,0,1);
  int t=0,e=0;
  assert(posplay_visivel()&&!posplay_pediu_episodio(&t,&e));
  posplay_atualizar(.016f,9000,2000,3600,1,0,0);
  unsigned outsideDeadline=fecharEm;
  posplay_atualizar(.016f,10000,3597,3600,1,0,1);
  unsigned returnedDeadline=fecharEm;
  posplay_atualizar(.016f,13100,3600,3600,1,0,1);
  int requested=posplay_pediu_episodio(&t,&e);
  printf("actual posplay reentry: outside_deadline=%u return_deadline=%u next_at_13100=%d\n",
         outsideDeadline,returnedDeadline,requested);
  if(expectBug) {
    assert(outsideDeadline==14000&&returnedDeadline==14000&&!requested);
    puts("REPRODUCED: retained real-marker card armed outside credits and postponed Next beyond the original regression deadline");
    posplay_fechar();return 43;
  }
  assert(!outsideDeadline&&returnedDeadline==13000&&requested&&t==1&&e==2);
  assert(!posplay_pediu_episodio(&t,&e));
  puts("PASS actual posplay: rewind re-arms only after returning to the episode's final window; Next is requested once");
  posplay_fechar();return 0;
}
