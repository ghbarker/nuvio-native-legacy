#include <SDL2/SDL.h>
char *SDL_GetBasePath(void);void SDL_free(void*);
#include PLAYER_SOURCE
#include <assert.h>
static int writes,applies;static char first[2048];
char *SDL_GetBasePath(void){return NULL;}void SDL_free(void*p){free(p);}
void dados_marcar_sujo(int i){(void)i;writes++;}
void video_legenda_estilo(const VideoLegendaEstilo*e){(void)e;applies++;}
void test_player_seed(const char*dir){player_dir(dir);FILE*f=fopen(prefsArquivo(),"w");assert(f);fputs("aspecto 2\nleg_tamanho 150\nleg_cor_borda 4\nleg_tocado 3\n",f);fclose(f);}
void test_player_unwritten(void){assert(writes==0&&applies==0);}
void test_player_preserved(void){assert(aspecto==2&&legTocado==3);assert(writes==0||writes==1);}
void test_player_reload(void){assert(writes==1&&applies==1);legEstilo.corBorda=0;prefsLer();assert(legEstilo.corBorda==5);}
