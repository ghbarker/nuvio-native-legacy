// posplay.c com catalogo, video e UI simulados.
#include "../src/posplay.c"
#include <assert.h>
static CatItem titulo;
static CatEp eps[3];
static int autoNext=1,aprendeu;
static int atualT=1,atualE=1,nEps=3,vel=100;
int ajustes_auto_proximo(void){return autoNext;}
const CatItem *cat_item(int i){return i==0?&titulo:NULL;}
int cat_indice_vivo(int i,const char *id){return i==0&&!strcmp(id,titulo.imdb)?0:-1;}
int cat_n_episodios(int i){return i==0?nEps:0;}
const CatEp *cat_episodio(int i,int e){return i==0&&e>=0&&e<3?eps+e:NULL;}
int cat_indice_por_imdb(const char *id){(void)id;return -1;}
void player_episodio_atual(int *t,int *e){*t=atualT;*e=atualE;}
void player_aprender_creditos(void){aprendeu++;}
int player_velocidade_efetiva(void){return vel;}
int extras_n_relacionados(void){return 0;}
const char *extras_relacionado_imdb(int i){(void)i;return "";}
void desc_pedir_titulo(const char *id){(void)id;assert(0);}
double video_creditos(void){return 0;}
double intro_creditos_seg(void){return 0;}
static void quadro(unsigned agora,double ui,int estado,double confirmado){
  posplay_automatico(estado,confirmado);
  posplay_atualizar(0.016f,agora,ui,120,1,0,1);
}
static void abrir(void){
  posplay_fechar();
  for(int i=0;i<10;i++)posplay_atualizar(1,100,0,120,1,0,0);
  quadro(1000,90,1,90);assert(posplay_visivel());
}
static int tecla(int k){SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;return posplay_evento(&e);}
int main(void){
  strcpy(titulo.imdb,"tt123");strcpy(titulo.tipo,"series");titulo.temporada=1;titulo.episodio=1;
  for(int i=0;i<3;i++){eps[i].temporada=i==2?2:1;eps[i].episodio=i==2?1:i+1;}
  int t,e;
  autoNext=0;abrir();quadro(1001,119,1,119);quadro(9000,120,2,120);
  assert(posplay_visivel()&&!posplay_pediu_episodio(&t,&e));
  assert(tecla(SDLK_RETURN)==1&&posplay_pediu_episodio(&t,&e)&&t==1&&e==2&&aprendeu==1);
  assert(!posplay_pediu_episodio(&t,&e));
  autoNext=1;abrir();quadro(1001,120,0,90);quadro(9000,120,0,90); // UI no fim antes da confirmacao do seek
  assert(posplay_visivel()&&!posplay_pediu_episodio(&t,&e));
  quadro(10000,117,1,117);quadro(14000,120,1,119); // Contagem vencida antes do fim confirmado
  assert(!posplay_pediu_episodio(&t,&e));
  quadro(14001,120,2,119);assert(posplay_pediu_episodio(&t,&e)&&t==1&&e==2);
  assert(!posplay_pediu_episodio(&t,&e));
  abrir();quadro(1100,117,1,117);quadro(6000,117,0,117); // Pausa/buffer cancela a contagem
  assert(!posplay_pediu_episodio(&t,&e));
  quadro(6100,118,1,118);quadro(6200,120,1,120);assert(!posplay_pediu_episodio(&t,&e));
  quadro(8200,120,2,120);assert(posplay_pediu_episodio(&t,&e));
  abrir();quadro(1100,119,1,119);autoNext=0;quadro(6000,120,2,120);
  assert(posplay_visivel()&&!posplay_pediu_episodio(&t,&e)); // Desligar cancela a contagem pendente
  assert(tecla(SDLK_RETURN)&&posplay_pediu_episodio(&t,&e));
  autoNext=1;abrir();quadro(1100,119,1,119);quadro(2200,120,2,120);autoNext=0;
  assert(!posplay_pediu_episodio(&t,&e)&&posplay_visivel()); // Desligar cancela o pedido automatico enfileirado
  assert(tecla(SDLK_RETURN)&&posplay_pediu_episodio(&t,&e));
  autoNext=1;abrir();quadro(1100,119,1,119);assert(tecla(SDLK_AC_BACK));quadro(9000,120,2,120);
  assert(!posplay_pediu_episodio(&t,&e)&&!posplay_visivel());
  vel=150;abrir();quadro(1100,114,1,114);quadro(5200,120,2,120);
  assert(posplay_pediu_episodio(&t,&e));vel=100;
  atualE=2;abrir();assert(tecla(SDLK_RETURN)&&posplay_pediu_episodio(&t,&e)&&t==2&&e==1);
  atualT=2;atualE=1;posplay_fechar();
  for(int i=0;i<10;i++)posplay_atualizar(1,100,0,120,1,0,0);
  quadro(1000,119,1,119);quadro(9000,120,2,120);
  assert(!posplay_visivel()&&!posplay_pediu_episodio(&t,&e)); // ultimo episodio
  atualT=1;atualE=1;nEps=0;posplay_fechar();
  for(int i=0;i<10;i++)posplay_atualizar(1,100,0,120,1,0,0);
  quadro(1000,100,1,100);assert(!posplay_visivel());
  nEps=3;quadro(1100,100,1,100);assert(posplay_visivel()); // lista tardia
  puts("posplay_auto: off/manual, confirmed position/end, pause, queued cancellation, Back passed");
  return 0;
}
