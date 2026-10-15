/* Real full detail handler and real More menu; no source extraction. */
#include DETAIL_SOURCE
#define main reader_matrix_main
#include "reader_probe.c"
#undef main
static CatItem route_item;
const CatItem *cat_item(int i){return i==0?&route_item:NULL;}
int recomenda_ativo(void){return 0;}
int trailer_suportado(void){return 0;}
int extras_n_trailers(void){return 0;}
float trocaarte_visivel(void){return 0;}
int reacao_aberta(void){return 0;}
int trailer_cheia(void){return 0;}
int trocaarte_aberto(void){return 0;}
int amtui_aberta(void){return 0;}
int ajustes_animacoes_reduzidas(void){return 1;}
void gfx_icone(GfxRect r,const char*n,float a,float b,float c,float d){(void)r;(void)n;(void)a;(void)b;(void)c;(void)d;}
static void route_key(Uint32 type,SDL_Keycode k){SDL_Event e;SDL_zero(e);e.type=type;e.key.keysym.sym=k;detail_evento(&e);}
static void more_key(Uint32 type,SDL_Keycode k){SDL_Event e;SDL_zero(e);e.type=type;e.key.keysym.sym=k;int a=detmais_evento(&e);if(a>=0)executarMais(a);}
int main(void){int disp[DMAIS_N];layout_tela_definir(1080,2340);layout_modo_definir(1);ponteiro_iniciar();ponteiro_teste_toque(1);idx=0;aberto=1;nivel=0;botao=4;scrollY=95;strcpy(route_item.titulo,"Full route title");strcpy(route_item.tipo,"movie");strcpy(route_item.imdb,"tt0000477");
maisDisponiveis(disp);check(!disp[DMAIS_SINOPSE],"title without metadata safely omits reader action");
strcpy(route_item.sinopse,"Complete metadata synopsis FINAL_SENTENCE_477");maisDisponiveis(disp);check(disp[DMAIS_SINOPSE],"arriving metadata exposes real reader action");
detmais_abrir(disp);check(detmais_aberto(),"real More menu opens");
more_key(SDL_KEYUP,SDLK_RETURN);check(detmais_aberto()&&!sinopse_aberta(),"release that opened More cannot accidentally activate reader");
for(int i=1;i<detmais_n();i++)more_key(SDL_KEYDOWN,SDLK_DOWN);check(detmais_acao(detmais_foco())==DMAIS_SINOPSE,"appended real action reachable without changing earlier IDs");
more_key(SDL_KEYDOWN,SDLK_RETURN);check(!sinopse_aberta(),"More only activates on armed release");more_key(SDL_KEYUP,SDLK_RETURN);check(sinopse_aberta()&&!detmais_aberto(),"real More selection opens copied full synopsis reader");
frame();int old_level=nivel,old_button=botao;float old_scroll=scrollY;route_key(SDL_KEYDOWN,SDLK_DOWN);check(nivel==old_level&&botao==old_button&&scrollY==old_scroll,"detail event routes exclusively to reader without moving underlying title");
route_key(SDL_KEYDOWN,SDLK_AC_BACK);check(!sinopse_aberta()&&aberto&&!saindo,"Back closes reader while keeping title page open");check(nivel==old_level&&botao==old_button&&scrollY==old_scroll,"reader return retains underlying title focus and scroll");
sinopse_abrir(route_item.titulo,route_item.sinopse);session++;check(!sinopse_aberta(),"profile session invalidates open reader");
idx=-1;maisDisponiveis(disp);check(!disp[DMAIS_SINOPSE],"invalidated title never dereferences absent synopsis");
printf("PASS real detail/More route checks=%d\n",checks);return 0;}
