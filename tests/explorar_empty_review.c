/* Independent empty/error-state coverage, retaining the original reviewer
 * fixture and its actual production Explore and mood data. */
#define main explorar_draw_review_main
#include "explorar_desenho_review.c"
#undef main
int main(void) {
  const float dims[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  climas.n=MAPA_CLIMA_N;entrada=1;focoT=0;
  for(int d=0;d<4;d++)for(int f=0;f<2;f++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];fontWide=f?1.25f:1;
    for(int i=0;i<MAPA_CLIMA_N;i++) {
      phase=100+i;clAberto=i;memset(&climas.c[i],0,sizeof climas.c[i]);climas.c[i].id=i;
      caN[0]=caN[1]=0;nTargets=0;clipped=0;desenharClima();
    }
    memset(&viz,0,sizeof viz);obra(&viz.foco,0);nTrilha=1;trilha[0].obra=viz.foco;origem=ORIGEM_CLIMA;
    phase=200;nTargets=0;clipped=0;desenharViz();
  }
  printf("explorar_empty_review: %d drawing operations, %d geometry violations\n",draws,failures);
  return failures?1:0;
}
