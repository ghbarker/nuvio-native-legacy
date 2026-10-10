/* Extend the real See all drawing fixture to ordinary and non-director grids. */
#define main vertudo_director_existing_main
#include "vertudo_retrato.c"
#undef main

int main(void) {
  anim_politica_reduzida=1;
  const float dims[][2]={{1080,2340},{1080,1920},{2340,1080},{2520,1080}};
  strcpy(item.titulo,"A very long ordinary title and extended catalog metadata");
  strcpy(item.meta,"2024");strcpy(item.genero,"Drama science fiction thriller");
  strcpy(item.sinopse,"Long synopsis with enough words to exercise the actual bounded detail panel.");
  strcpy(item.poster,"poster");strcpy(item.backdrop,"art");
  aberta=1;timeline=0;anim=1;orderN=-1;ondaArmada=ondaEm=0;tabFocus=0;
  int pages=0;
  for(int d=0;d<4;d++)for(int r=0;r<2;r++)for(int f=0;f<2;f++)for(int family=0;family<4;family++) {
    nv_layout_w=dims[d][0];nv_layout_h=dims[d][1];rail=r?140:0;fonte=f?30:12;
    strcpy(titulo,"A long collection or ordinary addon catalog heading");
    collection=family?&pasta:NULL;
    strcpy(pasta.group,family==1?"Genres":family==2?"Streaming":"Awards");
    strcpy(pasta.title,"A collection with a longer title");pasta.nSources=0;
    ranked=family==3;source=tabCursor=0;tabRol=0;
    for(int end=0;end<2;end++) {
      foco=end?15:0;scrollY=end?toqueVertudoMax():0;nAlvos=nCards=nPainel=0;cortando=0;
      vertudo_desenhar(500);assert(nAlvos>0 && !cortando);pages++;
      /* Targets are visible, and focusing a visible card preserves the route. */
      int found=0;
      for(int i=0;i<nAlvos;i++)if(alvos[i].focar==ponteiroCartaz){
        alvos[i].focar(alvos[i].a,alvos[i].b);assert(foco==alvos[i].a);found=1;break;
      }
      assert(found);
    }
  }
  printf("vertudo_grid_review: %d ordinary/genre/streaming/ranked pages at both scroll ends, target bounds and focus OK\n",pages);
}
