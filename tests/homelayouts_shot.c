// CAPTURA DA HOME NOS TRES LAYOUTS (Moderna, Padrao, Dinamica), SEM REDE.
//
// As artes sao as de deploy/app/art: fundo 16:9 (NN.jpg), cartaz 2:3
// (poster/NN.jpg) e logo (logo/NN.png). As fileiras imitam o que a descoberta
// publica de verdade: "Continuar assistindo", um catalogo de destaque, um "Em
// alta", generos e um "Top 10" — os SINAIS pelos quais o layout Dinamica
// escolhe a forma de cada fileira (home.c, dinTipoDaFileira).
//
// Uso (o .sh compila e converte para PNG):
//   bash tests/homelayouts_shot.sh /tmp/nv-home-layouts-shots [camadas]
//
// `camadas` = digitos dos layouts a capturar, "012" por padrao. Para cada layout
// roda duas passadas, com a Interface de vidro desligada e ligada, e em cada
// uma fotografa o repouso no destaque e o foco em CADA fileira. O nome do
// arquivo diz tudo: <prefixo>-L<layout>-g<vidro>-<n>-<fileira>.bmp.
//
// Ao final imprime, por captura, o preenchimento (gfx_fill, em telas cheias) e
// o numero de retangulos do quadro — o que da para medir no Mac do custo de
// desenho; o custo em ms de GPU so existe na TV.
#include "ajustes.h"
#include "artehero.h"
#include "catalogo.h"
#include "colecoes.h"
#include "corviva.h"
#include "ctxmenu.h"
#include "dados.h"
#include "fileiras.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "telefoneui.h"
#include "menu.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include "posterprov.h"
#include "trakt.h"
#include "socialvis.h"
#include "ponteiro.h"
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NA 40
static const char *NOMES[] = {
  "O Diabo Veste Vermelho", "A Ilha do Farol", "Mata Fechada", "Sofá no Deserto",
  "Preto e Branco", "Ensaio Seis", "Noite de Verão", "O Último Trem",
  "Cidade Cinza", "Rio Acima", "Sal e Luz", "A Casa do Lago",
  "Vento Norte", "Fronteira", "Depois da Chuva", "Cartas de Inverno",
  "Ouro Velho", "O Jardim", "Marés", "Sem Volta",
};
#define NN (int)(sizeof NOMES / sizeof *NOMES)

typedef struct { const char *chave, *titulo, *tipo; int n, catalogo; } Fil;
static const Fil FILS[] = {
  { "continue_watching", "Continuar assistindo", "movie",  6, 0 },
  { "pop_movie",  "Popular - Filme",         "movie",  10, 1 },
  { "trend_series", "Em alta - Série",       "series", 10, 1 },
  { "drama_movie", "Drama - Filme",          "movie",  10, 1 },
  { "top10_hoje",  "Top 10 · Filmes hoje",   "movie",  10, 1 },
  { "comedia_movie", "Comédia - Filme",      "movie",  10, 1 },
  { "ficcao_movie", "Ficção científica - Filme", "movie", 10, 1 },
};
#define NF (getenv("NV_COL") ? 3 : (int)(sizeof FILS / sizeof *FILS))

static SDL_Window *janela;
static const char *dirDados;
static double fillUlt, fillVisUlt, modoUlt[GFX_NMODOS]; static int rectUlt;
#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
static int heroPaginaConfere;
static float heroPaginaEsperada[3];
static void heroPaginaVerificar(void);
static int pastasComparando;
#endif

static void gravar(const char *bmp) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(pix && s);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  assert(SDL_SaveBMP(s, bmp) == 0);
  SDL_FreeSurface(s);
  free(pix);
}

static void quadros(int n, const char *bmp) {
  int i;
  for (i = 0; i < n; i++) {
    Uint32 agora = SDL_GetTicks();
#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
    if (getenv("NV_HERO_SWIPE")) ponteiro_quadro(agora);
    if (pastasComparando) ponteiro_quadro(agora);
#endif
    SDL_PumpEvents();
    tex_bombear(8);
    home_atualizar(1.0f / 60.0f, agora);
    corviva_quadro(1.0f / 60.0f, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
    txt_novo_quadro();
    tex_novo_quadro();
    gfx_novo_quadro();
    gfx_ambiente_preparar();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gfx_ambiente(1.0f);
    home_desenhar(agora);
#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
    if (heroPaginaConfere) heroPaginaVerificar();
#endif
    ctx_atualizar(1.0f / 60.0f, agora);
    ctx_desenhar(agora);
    // NV_MENU=1: a barra por cima, como app.c (no Dinamica, a pilula do topo).
    if (getenv("NV_MENU")) {
      menu_pilula_mostrar(home_topo_fracao());
      menu_atualizar(1.0f / 60.0f, agora);
      menu_desenhar(agora);
    }
#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
    if (getenv("NV_HERO_SWIPE")) ponteiro_desenhar();
    if (pastasComparando) ponteiro_desenhar();
#endif
    if (i == n - 1) { fillUlt = gfx_fill; fillVisUlt = gfx_fill_vis; rectUlt = gfx_n_rect;
                      memcpy(modoUlt, gfx_fill_modo, sizeof modoUlt); }
    if (bmp && i == n - 1) gravar(bmp);
    SDL_GL_SwapWindow(janela);
    SDL_Delay(8);
  }
}

// SEGURAR OK de verdade: KEYDOWN, quadros ate passar NV_HOLD_MS (o relogio e o
// SDL_GetTicks real), KEYUP. E o caminho da home que abre o menu do cartaz.
static char meioBmp[512];   // se preenchido: captura no meio do segurar
static void segurarOk(void) {
  SDL_Event e;
  Uint32 ini = SDL_GetTicks();
  int meioFeito = 0;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
  home_evento(&e);
  while (SDL_GetTicks() - ini < NV_HOLD_MS + 150) {
    quadros(1, NULL);
    if (meioBmp[0] && !meioFeito && SDL_GetTicks() - ini > NV_HOLD_MS / 2) {
      meioFeito = 1; quadros(1, meioBmp); meioBmp[0] = 0;
    }
  }
  e.type = SDL_KEYUP;
  if (ctx_aberto()) ctx_evento(&e); else home_evento(&e);
}
static void teclaCtx(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  ctx_evento(&e);
  e.type = SDL_KEYUP;
  ctx_evento(&e);
}

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  home_evento(&e);
  e.type = SDL_KEYUP;
  home_evento(&e);
}

// Ajustes pelo mesmo arquivo que a TV le. Idioma 0 = pt; animacoes NORMAIS
// (as molas assentam em ~90 quadros); trailer desligado (sem rede).
static void ajusta(int layout, int vidro) {
  char cam[700];
  FILE *a;
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w");
  assert(a);
  fprintf(a, "idioma 0\ntrailerHero 1\nhomeLayoutLocal %d\nvidroLocal %d\n"
             "modernLandscapePostersEnabled 1\nselected_theme %d\n",
          layout, vidro ? 0 : 1, getenv("NV_TEMA") ? atoi(getenv("NV_TEMA")) : 0);
  if (getenv("NV_AJ")) fprintf(a, "%s\n", getenv("NV_AJ"));   // ex.: "heroSectionEnabled 1"
#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
  // ajustes.txt stores the option index: 1 selects "Desligado".
  if (getenv("NV_HERO_SWIPE")) fprintf(a,"modernHeroFullScreenBackdropEnabled 1\n");
#endif
  fclose(a);
  ajustes_dir(dirDados);
  // O roteamento de app.c (app_atualizar), que este teste nao roda.
  artehero_fundo_addon(ajustes_fundo_addon());
  posterprov_preferir_addon(ajustes_poster_addon());
  col_arte_conta(ajustes_col_arte_conta());
}

#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
static int pastaFocar(TipoFileira tipo,const char *titulo) {
  home_ir_topo();quadros(120,NULL);
  for(int i=0;i<40;i++) {
    int row,n,t,col;
    if(sscanf(home_rastro_foco(),"f%d/%d tipo=%d col=%d",&row,&n,&t,&col)==4 &&
       t==(int)tipo && strstr(home_rastro_foco(),titulo)) return row;
    tecla(SDLK_DOWN);quadros(110,NULL);
  }
  fprintf(stderr,"folder comparison row missing: %s\n",titulo);assert(0);return -1;
}
static PonteiroAlvo pastaAlvo(PonteiroFn fn,int row) {
  const PonteiroAlvo *v;
  int n=ponteiro_teste_lista(&v);
  for(int i=0;i<n;i++)if(v[i].focar==fn&&v[i].a==row&&v[i].b==0)return v[i];
  fprintf(stderr,"folder comparison target missing: row %d\n",row);assert(0);
  return (PonteiroAlvo){0};
}
static void pastaDedo(Uint32 tipo,float x,float y) {
  SDL_Event e={0};e.type=tipo;e.tfinger.touchId=71;e.tfinger.fingerId=1;
  e.tfinger.x=x/NV_TELA_W;e.tfinger.y=y/NV_TELA_H;
  assert(ponteiro_evento(&e,home_evento));
}
static void pastasComparar(const char *saida,const char *camadas) {
  const char *nomes[]={"landscape","square","poster"};
  const int tipos[]={FIL_TIPO_COLECAO,FIL_TIPO_DESTAQUE_QUADRADO,FIL_TIPO_CARTAZ};
  char bmp[900];
  int tipoCol=fil_tipo("collection_cs"),tipoPoster=fil_tipo("trend_series");
  pastasComparando=1;ponteiro_iniciar();ponteiro_teste_toque(1);
  assert(fil_definir_tipo("trend_series",FIL_TIPO_CARTAZ));
  for(const char *L=camadas;*L;L++) {
    int lay=*L-'0';
    /* Dynamic moves Streaming into navigation; these comparisons concern
       the actual Home folder row in Modern and Standard. */
    if(lay==HOME_LAYOUT_DINAMICA)continue;
    for(int vidro=0;vidro<2;vidro++) {
      ajusta(lay,vidro);
      int rowPoster=pastaFocar(FILEIRA_NORMAL,"Em alta");
      HomeItem normal;assert(home_item_focado(&normal));
      float zoom=ajustes_borda_foco()?1:1.06f;
      float escalaPoster=fil_escala("trend_series");
      float pw=normal.rect.w/zoom/escalaPoster,ph=normal.rect.h/zoom/escalaPoster;
      PonteiroFn fn=NULL;
      const PonteiroAlvo *v;int n=ponteiro_teste_lista(&v);
      for(int i=0;i<n;i++)if(v[i].a==rowPoster&&v[i].b==0&&v[i].focar &&
        fabsf(v[i].x-normal.rect.x)<.1f&&fabsf(v[i].w-normal.rect.w)<.1f)fn=v[i].focar;
      assert(fn);
      snprintf(bmp,sizeof bmp,"%s-folders-L%d-g%d-0-ordinary-posters.bmp",saida,lay,vidro);
      quadros(1,bmp);
      for(int forma=0;forma<3;forma++) {
        assert(fil_definir_tipo("collection_cs",tipos[forma]));quadros(120,NULL);
        int row=pastaFocar(FILEIRA_CATALOGOS,"Streaming");
        assert(row==rowPoster+1);
        PonteiroAlvo alvo=pastaAlvo(fn,row);
        float escalaCol=fil_escala("collection_cs");
        float w=(forma==2?pw:forma==1?ph:ph*360/203)*escalaCol,h=ph*escalaCol;
        if(NV_TELA_H/NV_TELA_W>=1.7f) {
          float x=ajustes_rail_largura_fixa()>0?fmaxf(48,ajustes_conteudo_x()):48;
          float cap=(NV_TELA_W-2*x)*.9f;
          if(w>cap){h*=cap/w;w=cap;}
        }
        assert(fabsf(alvo.w-w)<.15f&&fabsf(alvo.h-h)<.15f);
        /* Place adjacent rows through the real vertical gesture. Short
           landscape viewports keep the complete folder plus the visible
           part of the poster; the separate reference captures it whole. */
        float gap=NV_PAD_FILEIRA_GAP;
        if(lay==HOME_LAYOUT_MODERNA)gap=ajustes_posteres_deitados()?NV_FILEIRA_GAP_LAND:NV_FILEIRA_GAP;
        gap*=ajustes_espaco_fileiras();
        float copia=ajustes_rotulos_poster()?NV_POSTER_COPY_H:0;
        float top=fminf(156+2*NV_LEGACY_ROW_HEAD_H+ph*escalaPoster+copia+gap,NV_TELA_H-h-24);
        float x=alvo.x+alvo.w*.5f,y=alvo.y+fminf(alvo.h*.5f,80),delta=top-alvo.y;
        pastaDedo(SDL_FINGERDOWN,x,y);SDL_Delay(100);
        pastaDedo(SDL_FINGERMOTION,x,y+delta);quadros(1,NULL);
        SDL_Delay(100);pastaDedo(SDL_FINGERUP,x,y+delta);quadros(4,NULL);
        alvo=pastaAlvo(fn,row);
        PonteiroAlvo poster=pastaAlvo(fn,rowPoster);
        assert(fabsf(alvo.w-w)<.15f&&fabsf(alvo.h-h)<.15f);
        float posterTop=alvo.y-NV_LEGACY_ROW_HEAD_H-ph*escalaPoster-copia-gap;
        float posterVis=ph*escalaPoster-fmaxf(0,132-posterTop);
        assert(fabsf(poster.w-pw*escalaPoster)<.15f&&fabsf(poster.h-posterVis)<.15f);
        assert(poster.h>=ph*escalaPoster*.7f);
        assert(poster.y>=132&&poster.y+poster.h<alvo.y);
        assert(alvo.y+alvo.h<=NV_TELA_H+.15f);
        snprintf(bmp,sizeof bmp,"%s-folders-L%d-g%d-%d-%s-beside-posters.bmp",saida,lay,vidro,forma+1,nomes[forma]);
        quadros(1,bmp);
        printf("[shot] actual Home folder %s %.3fx%.3f, ordinary poster %.3fx%.3f (adjacent visible height %.3f), aspect %.6f, UI%.0f: measured draw targets passed\n",
               nomes[forma],alvo.w,alvo.h,pw*escalaPoster,ph*escalaPoster,poster.h,alvo.w/alvo.h,gfx_escala_ui()*100);
      }
    }
  }
  assert(fil_definir_tipo("collection_cs",tipoCol));
  assert(fil_definir_tipo("trend_series",tipoPoster));
  pastasComparando=0;
}

int home_teste_hero_deslocamento(float valores[5]);
int home_teste_pagina(float valores[3]);
static void heroPaginaVerificar(void) {
  float atual[3]; assert(home_teste_pagina(atual));
  assert(fabsf(atual[0]-heroPaginaEsperada[0])<.1f);
  assert(fabsf(atual[1]-heroPaginaEsperada[1])<.1f);
}
static void heroDelta(float esperado) {
  float desenhado[5]; assert(home_teste_hero_deslocamento(desenhado));
  for(int i=0;i<4;i++)assert(fabsf(desenhado[i]-esperado)<.1f);
  printf("[shot] actual hero draw: finger %.3f, old art/copy %.3f/%.3f, next art/copy %.3f/%.3f, width %.3f\n",
         esperado,desenhado[0],desenhado[1],desenhado[2],desenhado[3],desenhado[4]);
}
static void heroDedo(Uint32 tipo, float x, float y) {
  SDL_Event e = {0}; e.type = tipo; e.tfinger.touchId=41; e.tfinger.fingerId=1;
  e.tfinger.x=x/NV_TELA_W; e.tfinger.y=y/NV_TELA_H;
  assert(ponteiro_evento(&e,home_evento));
}
static void heroCapturar(const char *saida,int layout,const char *estado) {
  char bmp[900]; snprintf(bmp,sizeof bmp,"%s-swipe-L%d-%s.bmp",saida,layout,estado);
  quadros(1,bmp);
}
static void heroSwipes(const char *saida) {
  ponteiro_iniciar(); ponteiro_teste_toque(1);
  for(int lay=0;lay<3;lay++) {
    heroPaginaConfere=0;
    ajusta(lay,0); home_ir_topo(); quadros(120,NULL);
    assert(!ajustes_hero_cheio());
    if(lay==HOME_LAYOUT_MODERNA && NV_TELA_W>NV_TELA_H) {
      tecla(SDLK_DOWN); quadros(120,NULL);
      float px,py,pw,ph; home_hero_rect(&px,&py,&pw,&ph);
      assert(pw<NV_TELA_W*.75f);
      printf("[shot] narrow initial hero: x %.3f width %.3f, canvas %.3f\n",px,pw,NV_TELA_W);
    }
    HomeItem origem, atual;
    assert(home_item_focado(&origem));
    float x0=NV_TELA_W*.75f, x1=NV_TELA_W*.39f, y=300;
    heroCapturar(saida,lay,"0-inicial");
    assert(home_teste_pagina(heroPaginaEsperada)); heroPaginaConfere=1;
    heroDedo(SDL_FINGERDOWN,x0,y); SDL_Delay(80);
    heroDedo(SDL_FINGERMOTION,x1,y); heroCapturar(saida,lay,"1-arrasto-esquerda");
    heroDelta(x1-x0);
    assert(!home_pediu_abrir() && !home_pediu_tocar());
    heroDedo(SDL_FINGERUP,x1,y); quadros(4,NULL);
    heroCapturar(saida,lay,"2-assentando-esquerda"); quadros(40,NULL);
    heroCapturar(saida,lay,"3-proximo");
    assert(home_item_focado(&atual) && atual.indice!=origem.indice);
    assert(strstr(atual.arte,cat_item(atual.indice)->imdb) ||
           !strcmp(atual.arte,cat_item(atual.indice)->backdrop));
    assert(!home_pediu_abrir() && !home_pediu_tocar());
    x0=NV_TELA_W*.3f; x1=NV_TELA_W*.66f;
    heroDedo(SDL_FINGERDOWN,x0,y); SDL_Delay(80);
    heroDedo(SDL_FINGERMOTION,x1,y); heroCapturar(saida,lay,"4-arrasto-direita");
    heroDelta(x1-x0);
    heroDedo(SDL_FINGERUP,x1,y); quadros(40,NULL);
    heroCapturar(saida,lay,"5-anterior");
    assert(home_item_focado(&atual) && atual.indice==origem.indice);
    x0=NV_TELA_W*.6f; x1=NV_TELA_W*.53f;
    heroDedo(SDL_FINGERDOWN,x0,y); SDL_Delay(100);
    heroDedo(SDL_FINGERMOTION,x1,y); SDL_Delay(120);
    heroDedo(SDL_FINGERUP,x1,y); quadros(40,NULL);
    heroCapturar(saida,lay,"6-curto-retorna");
    assert(home_item_focado(&atual) && atual.indice==origem.indice);
    heroDedo(SDL_FINGERDOWN,NV_TELA_W*.75f,y); SDL_Delay(80);
    heroDedo(SDL_FINGERMOTION,NV_TELA_W*.39f,y); quadros(1,NULL);
    ponteiro_cancelar_toque(); heroDedo(SDL_FINGERUP,NV_TELA_W*.39f,y);
    quadros(40,NULL); heroCapturar(saida,lay,"7-cancelado");
    assert(home_item_focado(&atual) && atual.indice==origem.indice);
    assert(!home_pediu_abrir() && !home_pediu_tocar());
    printf("[shot] horizontal hero L%d: page offset %.3f and drawn first-row Y %.3f unchanged through left/right, short return and cancel\n",
           lay,heroPaginaEsperada[0],heroPaginaEsperada[1]);
    heroPaginaConfere=0;
    float pagina[3]; assert(home_teste_pagina(pagina) && pagina[2]==1);
    tecla(SDLK_UP);
    assert(home_teste_pagina(pagina) && pagina[2]==0);
    heroDedo(SDL_FINGERDOWN,NV_TELA_W*.55f,y);
    heroDedo(SDL_FINGERUP,NV_TELA_W*.55f,y);
    assert(home_pediu_abrir());
    HomeItem aberto; assert(home_item_focado(&aberto) && aberto.indice==atual.indice);
    assert(aberto.titulo && !strcmp(aberto.titulo,cat_item(atual.indice)->titulo));
    printf("[shot] hero swipe L%d: left/right identity, short return, cancel, TV key recovery and tap passed\n",lay);
  }
}
#endif

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-home-layouts-shots/h";
  const char *camadas = argc > 2 ? argv[2] : "012";
  static CatItem itens[200];
  static CatFileira fils[8];
  SDL_GLContext gl;
  char bmp[800], cache[700];
  int i, k, total = 0, ini = 0;

  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (!dirDados || !strstr(dirDados, "nuvio-homelayouts-shot")) {
    fprintf(stderr, "recuse: NUVIO_DADOS tem de ser a pasta temporaria do teste\n");
    return 1;
  }
  (void)argc;
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: layouts da home", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  // NV_EFEITOS=1 leves, 2 minimos (os niveis de gpunivel.h sem o 720p).
  if (getenv("NV_EFEITOS")) {
    int e = atoi(getenv("NV_EFEITOS"));
    gfx_definir_efeitos_leves(e >= 1);
    gfx_definir_efeitos_minimos(e >= 2);
  }
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_borrao_iniciar(480, 270);
  artehero_definir_falhou(tex_falhou);
  snprintf(cache, sizeof cache, "%s/cache", dirDados);
  tex_cache_dir(cache);
  gfx_icones_dir("deploy/app/art");
  ajusta(0, 0);
  assert(home_iniciar("deploy/app/art"));

  memset(itens, 0, sizeof itens);
  memset(fils, 0, sizeof fils);
  for (k = 0; k < NF; k++) {
    CatFileira *f = &fils[k];
    snprintf(f->chave, sizeof f->chave, "%s", FILS[k].chave);
    snprintf(f->titulo, sizeof f->titulo, "%s", FILS[k].titulo);
    snprintf(f->tipo, sizeof f->tipo, "%s", FILS[k].tipo);
    if (FILS[k].catalogo) {
      snprintf(f->base, sizeof f->base, "https://addon.invalid/x");
      snprintf(f->catId, sizeof f->catId, "%s", FILS[k].chave);
    }
    // NV_FIL_N=<n>: cada CATALOGO com n itens (o "Itens por fileira" 12/18/24
    // da issue #201); sem ele, o n da tabela.
    { int nk = FILS[k].n;
      if (FILS[k].catalogo && getenv("NV_FIL_N")) nk = atoi(getenv("NV_FIL_N"));
      if (nk > 24) nk = 24;
      f->ini = ini; f->n = nk; }
    for (i = 0; i < f->n; i++) {
      CatItem *c = &itens[ini + i];
      int a = (ini + i) % NA;
      snprintf(c->imdb, sizeof c->imdb, "tt90%05d", ini + i);
      snprintf(c->tipo, sizeof c->tipo, "%s", FILS[k].tipo);
      snprintf(c->titulo, sizeof c->titulo, "%s", NOMES[(ini + i) % NN]);
      snprintf(c->genero, sizeof c->genero, "%s · Drama", strcmp(FILS[k].tipo, "series") ? "Filme" : "Série");
      snprintf(c->meta, sizeof c->meta, "%d · 2 h 04 min", 2018 + (ini + i) % 8);
      snprintf(c->classificacao, sizeof c->classificacao, "%s", (i % 3) ? "14" : "16");
      snprintf(c->sinopse, sizeof c->sinopse,
               "Sinopse de enchimento, comprida o bastante para ocupar as linhas "
               "que o destaque reserva para ela, como num titulo de verdade.");
      snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", a);
      snprintf(c->backdropCatalogo, sizeof c->backdropCatalogo, "%s", c->backdrop);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", a);
      // NV_SEM_LOGO=1: nenhum titulo com logo (o cabecalho em texto do menu).
      if (a < 10 && !getenv("NV_SEM_LOGO")) snprintf(c->logo, sizeof c->logo, "deploy/app/art/logo/%02d.png", a);
      // NV_ARTE_ADDON=1: todo item vem de um addon (origem; inclusive o da
      // primeira fileira, que abre o destaque) e tambem traz um fundo "do
      // TMDB" — outra foto, local —, para a captura separar "Background do
      // hero" = TMDB de "Fundo do destaque do addon".
      if (getenv("NV_ARTE_ADDON")) {
        snprintf(c->origem, sizeof c->origem, "%s", "xperience");
        snprintf(c->backdropTmdb, sizeof c->backdropTmdb, "deploy/app/art/%02d.jpg", (a + 17) % NA);
      }
      c->nota = 68 + (ini + i) % 25;
      if (k == 0) { c->progresso = 20 + i * 12; c->restanteMin = 90 - i * 10; }
    }
    ini += f->n;
    total = ini;
  }
  if (getenv("NV_COL")) {   // NV_COL=1: um grupo de colecao no fim, com as quatro cadeias de arte
    // NV_COL_FORMA=POSTER|LANDSCAPE|SQUARE: o tileShape das quatro pastas
    // (ausente = sem o campo, que o web le como quadrado).
    char fonte[400];
    char js[4600];
    snprintf(fonte, sizeof fonte, "%s%s%s\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]",
             getenv("NV_COL_FORMA") ? "\"tileShape\":\"" : "",
             getenv("NV_COL_FORMA") ? getenv("NV_COL_FORMA") : "",
             getenv("NV_COL_FORMA") ? "\"," : "");
    snprintf(js, sizeof js,
      "{\"collections\":[{\"id\":\"cs\",\"title\":\"Streaming\",\"backdropImageUrl\":\"deploy/app/art/07.jpg\",\"folders\":["
      "{\"id\":\"a\",\"title\":\"Com hero e capa\",\"heroBackdropUrl\":\"deploy/app/art/03.jpg\",\"coverImageUrl\":\"deploy/app/art/poster/12.jpg\",%s},"
      "{\"id\":\"b\",\"title\":\"So capa\",\"coverImageUrl\":\"deploy/app/art/05.jpg\",%s},"
      "{\"id\":\"c\",\"title\":\"So fundo da colecao\",%s},"
      "{\"id\":\"d\",\"title\":\"Com hero sem capa\",\"heroBackdropUrl\":\"deploy/app/art/09.jpg\",%s}]}]}",
      fonte, fonte, fonte, fonte);
    // NV_COL_PACOTE=1: o pacote traz as pastas "a" e "b" com arte PROPRIA
    // (caminho absoluto: localiza nao mexe), para a captura de "Arte das
    // pastas da conta". Desligado vence o pacote; ligado, a conta.
    if (getenv("NV_COL_PACOTE")) {
      char cam[800], cwd[500], pk[2400];
      FILE *f;
      assert(getcwd(cwd, sizeof cwd));
      snprintf(pk, sizeof pk,
        "{\"groups\":[{\"id\":\"cs\",\"title\":\"Streaming\",\"folders\":["
        "{\"id\":\"a\",\"title\":\"Com hero e capa\",\"cover\":\"%s/deploy/app/art/poster/30.jpg\",\"hero\":\"%s/deploy/app/art/21.jpg\","
          "\"sources\":[{\"title\":\"M\",\"base\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catId\":\"k\"}]},"
        "{\"id\":\"b\",\"title\":\"So capa\",\"cover\":\"%s/deploy/app/art/25.jpg\","
          "\"sources\":[{\"title\":\"M\",\"base\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catId\":\"k\"}]}]}]}",
        cwd, cwd, cwd);
      snprintf(cam, sizeof cam, "%s/collections.json", dirDados);
      f = fopen(cam, "w"); assert(f); fputs(pk, f); fclose(f);
      assert(col_carregar(dirDados) == 2);
    }
    assert(col_definir_json(js) == 4);
  }
  cat_definir_tudo(itens, total, fils, NF);
  // NV_AMIGOS=1: amigos falsos nos tres primeiros titulos de "Popular" (feed externo).
  if (getenv("NV_AMIGOS")) {
    SvEvento e[6]; int k = 0, i;
    memset(e, 0, sizeof e);
    for (i = 0; i < 6; i++) { e[i].pct = -1; e[i].restanteMin = -1; e[i].quando = 1700000000LL - i;
      snprintf(e[i].imdb, sizeof e[i].imdb, "%s", itens[fils[1].ini + (i < 3 ? 0 : i - 2)].imdb); }
    snprintf(e[k].pessoaId, 96, "nuvio:fabi"); snprintf(e[k].pessoaNome, 64, "Fabi"); e[k].acao = SV_REACAO; e[k++].reacao = SV_REAC_GOSTOU;
    snprintf(e[k].pessoaId, 96, "nuvio:rafa"); snprintf(e[k].pessoaNome, 64, "Rafa"); e[k++].acao = SV_FIM;
    snprintf(e[k].pessoaId, 96, "nuvio:mari"); snprintf(e[k].pessoaNome, 64, "Mari"); e[k++].acao = SV_FIM;
    snprintf(e[k].pessoaId, 96, "nuvio:mari"); snprintf(e[k].pessoaNome, 64, "Mari"); e[k++].acao = SV_FIM;
    snprintf(e[k].pessoaId, 96, "nuvio:rafa"); snprintf(e[k].pessoaNome, 64, "Rafa"); e[k++].acao = SV_FIM;
    snprintf(e[k].pessoaId, 96, "nuvio:leo"); snprintf(e[k].pessoaNome, 64, "Leo"); e[k++].acao = SV_FIM;
    socialvis_definir_feed(e, 6);
  }
  quadros(60, NULL);
#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
  if (getenv("NV_HERO_SWIPE")) { heroSwipes(saida); goto feito; }
#endif
  // NV_VISTOS=1 (#212): o selo de visto pelo HISTORICO, sem progresso. Dois
  // filmes de "Popular" pelo leitor de /sync/watched/movies (o corpo do Trakt)
  // e uma serie de "Em alta" pelo historico de titulo — nenhum deles tem
  // progresso, que era a unica coisa que o selo lia.
  if (getenv("NV_VISTOS")) {
    char corpo[600];
    extern void cat_historico_definir_id(const char *imdb, const char *tipo, int visto);
    snprintf(corpo, sizeof corpo,
             "[{\"plays\":1,\"last_watched_at\":\"2026-01-01T00:00:00.000Z\",\"movie\":{\"title\":\"A\",\"year\":2020,"
             "\"ids\":{\"trakt\":1,\"slug\":\"a\",\"imdb\":\"%s\",\"tmdb\":1}}},"
             "{\"plays\":2,\"movie\":{\"title\":\"B\",\"ids\":{\"trakt\":2,\"imdb\":\"%s\"}}}]",
             itens[fils[1].ini].imdb, itens[fils[1].ini + 2].imdb);
    printf("[shot] filmes vistos lidos: %d\n", trakt_ler_filmes_vistos(corpo));
    cat_historico_definir_id(itens[fils[2].ini + 1].imdb, "series", 1);
    printf("[shot] cat_visto pop0=%d pop1=%d pop2=%d serie1=%d\n",
           cat_visto(&itens[fils[1].ini]), cat_visto(&itens[fils[1].ini + 1]),
           cat_visto(&itens[fils[1].ini + 2]), cat_visto(&itens[fils[2].ini + 1]));
    quadros(10, NULL);
  }
  // NV_TIPOS="chave=tipo,chave=tipo": a forma escolhida em "Estilo da fileira"
  // (o numero FilTipo de fileiras.h), como se a pessoa tivesse escolhido.
  if (getenv("NV_TIPOS")) {
    char buf[400], *p, *sv = NULL;
    snprintf(buf, sizeof buf, "%s", getenv("NV_TIPOS"));
    for (p = strtok_r(buf, ",", &sv); p; p = strtok_r(NULL, ",", &sv)) {
      char *ig = strchr(p, '=');
      if (!ig) continue;
      *ig = 0;
      printf("[shot] tipo %s=%s ok=%d\n", p, ig + 1, fil_definir_tipo(p, atoi(ig + 1)));
    }
    quadros(60, NULL);
  }

  // NV_SO_CTX=1: pula a volta pelos layouts e vai direto ao menu do cartaz
  // (NV_CTX), no layout do primeiro digito de `camadas`.
  if (!getenv("NV_SO_CTX")) { const char *L;
    for (L = camadas; *L; L++) {
      int layout = *L - '0', vidro;
      for (vidro = 0; vidro < 2; vidro++) {
        int r;
        ajusta(layout, vidro);
        assert(ajustes_home_layout() == layout);
        for (r = 0; r < 14; r++) tecla(SDLK_UP);
        quadros(120, NULL);
        snprintf(bmp, sizeof bmp, "%s-L%d-g%d-0-destaque.bmp", saida, layout, vidro);
        quadros(1, bmp);
        printf("[shot] L%d g%d destaque: fill=%.2f vis=%.2f rects=%d\n", layout, vidro, fillUlt, fillVisUlt, rectUlt);
        // NV_MENU_ABRIR=1 (com NV_MENU): a barra aberta sobre o destaque e,
        // descendo ate o fim, a secao de Streaming (NV_COL) rolada.
        if (getenv("NV_MENU_ABRIR") && getenv("NV_MENU")) {
          SDL_Event me;
          int d;
          menu_abrir();
          snprintf(bmp, sizeof bmp, "%s-L%d-g%d-0-menu-aberto.bmp", saida, layout, vidro);
          quadros(60, bmp);
          memset(&me, 0, sizeof me);
          me.type = SDL_KEYDOWN; me.key.keysym.sym = SDLK_DOWN;
          for (d = 0; d < 10; d++) menu_evento(&me);
          snprintf(bmp, sizeof bmp, "%s-L%d-g%d-0-menu-streaming.bmp", saida, layout, vidro);
          quadros(60, bmp);
          for (d = 0; d < 10; d++) menu_evento(&me);
          snprintf(bmp, sizeof bmp, "%s-L%d-g%d-0-menu-fim.bmp", saida, layout, vidro);
          quadros(60, bmp);
          menu_fechar();
          quadros(40, NULL);
        }
        if (getenv("NV_FILL_MODOS")) {   // quem preenche: modo=telas cheias
          int m; printf("[shot]   modos:");
          for (m = 0; m < GFX_NMODOS; m++) if (modoUlt[m] > 0.05) printf(" %d=%.2f", m, modoUlt[m]);
          printf("\n"); }
        // NV_TROCA=1: a troca do destaque pela seta (direita e depois esquerda),
        // fotografada a cada 4 quadros para pegar o MEIO do deslize/esmaecer.
        if (getenv("NV_TROCA") && !vidro) {
          int q, lado;
          for (lado = 0; lado < 2; lado++) {
            tecla(lado ? SDLK_LEFT : SDLK_RIGHT);
            for (q = 0; q < 10; q++) {
              quadros(3, NULL);
              snprintf(bmp, sizeof bmp, "%s-L%d-troca-%c-%02d.bmp", saida, layout,
                       lado ? 'e' : 'd', q);
              quadros(1, bmp);
            }
            quadros(60, NULL);
          }
        }
        // NV_SO_DESTAQUE=1: so o destaque (e o menu, com NV_MENU_ABRIR), sem
        // descer pelas fileiras — a conferencia do menu e do recuo do conteudo.
        if (getenv("NV_SO_DESTAQUE")) continue;
        for (r = 0; r < 9; r++) {
          tecla(SDLK_DOWN);
          quadros(110, NULL);
          snprintf(bmp, sizeof bmp, "%s-L%d-g%d-%d-fileira.bmp", saida, layout, vidro, r + 1);
          quadros(1, bmp);
          printf("[shot] L%d g%d fileira %d: fill=%.2f vis=%.2f rects=%d\n", layout, vidro, r + 1, fillUlt, fillVisUlt, rectUlt);
          if (getenv("NV_FILL_MODOS")) {
            int m; printf("[shot]   modos:");
            for (m = 0; m < GFX_NMODOS; m++) if (modoUlt[m] > 0.02) printf(" %d=%.2f", m, modoUlt[m]);
            printf("\n"); }
          // NV_OK_FIL=<r>: OK na fileira r (abre a pilha do Top 10) antes do
          // passo para o lado.
          if (getenv("NV_OK_FIL") && r == atoi(getenv("NV_OK_FIL"))) {
            tecla(SDLK_RETURN); quadros(60, NULL); }
          // Um passo para o lado: rolagem horizontal + foco. NV_LADO_FIL=<r>
          // escolhe a fileira (0 = a primeira abaixo do destaque).
          if (r == (getenv("NV_LADO_FIL") ? atoi(getenv("NV_LADO_FIL")) : 2)) {
            // NV_LADO=<n>: n passos em vez de dois (o "11", "24" do ranking).
            int d = getenv("NV_LADO") ? atoi(getenv("NV_LADO")) : 2, q;
            for (q = 0; q < d; q++) { tecla(SDLK_RIGHT); quadros(8, NULL); }
            quadros(110, NULL);
            snprintf(bmp, sizeof bmp, "%s-L%d-g%d-%d-fileira-lado.bmp", saida, layout, vidro, r + 1);
            quadros(1, bmp);
          }
        }
        /* MEIO DA ROLAGEM: 12 quadros depois de subir uma fileira. */
        tecla(SDLK_UP);
        quadros(10, NULL);
        snprintf(bmp, sizeof bmp, "%s-L%d-g%d-x-meio-da-rolagem.bmp", saida, layout, vidro);
        quadros(1, bmp);
      }
    } }

#if defined(NV_TOUCH_PREVIEW) && defined(NV_SHOT_HOOKS)
  if(getenv("NV_COL")&&telefoneui_ativo())pastasComparar(saida,camadas);
#endif

  // NV_CTX=<n>: menu do cartaz SEGURANDO OK na fileira n (contada do destaque
  // para baixo), a pagina de estilos, a escolha e a home depois dela.
  if (getenv("NV_CTX")) {
    int r, alvo = atoi(getenv("NV_CTX"));
    // NV_CTX_VIDRO=1: o menu sobre a Interface de vidro (o padrao e o solido).
    ajusta(camadas[0] - '0', getenv("NV_CTX_VIDRO") ? 1 : 0);
    for (r = 0; r < 14; r++) tecla(SDLK_UP);
    quadros(60, NULL);
    for (r = 0; r < alvo; r++) { tecla(SDLK_DOWN); quadros(40, NULL); }
    // NV_CTX_DIR=<n>: n cartoes a direita antes de segurar (um com logo).
    { int d = getenv("NV_CTX_DIR") ? atoi(getenv("NV_CTX_DIR")) : 0;
      for (r = 0; r < d; r++) { tecla(SDLK_RIGHT); quadros(8, NULL); } }
    quadros(80, NULL);
    snprintf(bmp, sizeof bmp, "%s-ctx-0-antes.bmp", saida); quadros(1, bmp);
    segurarOk();
    // NV_CTX_FOCO=<n>: o foco n linhas abaixo da primeira (o mockup acende a
    // segunda).
    { int d = getenv("NV_CTX_FOCO") ? atoi(getenv("NV_CTX_FOCO")) : 0;
      for (r = 0; r < d; r++) teclaCtx(SDLK_DOWN); }
    quadros(60, NULL);
    snprintf(bmp, sizeof bmp, "%s-ctx-1-menu.bmp", saida); quadros(1, bmp);
    printf("[shot] ctx aberto=%d\n", ctx_aberto());
    // NV_CTX_CONF=<n>: desce n linhas ate "Tirar de Continuar assistindo", OK
    // abre a confirmacao (capturada), e Voltar cancela de volta ao menu.
    if (getenv("NV_CTX_CONF")) {
      int d = atoi(getenv("NV_CTX_CONF"));
      for (r = 0; r < d; r++) teclaCtx(SDLK_DOWN);
      teclaCtx(SDLK_RETURN);
      quadros(60, NULL);
      snprintf(bmp, sizeof bmp, "%s-ctx-1b-confirma.bmp", saida); quadros(1, bmp);
      printf("[shot] confirmacao: menu aberto=%d\n", ctx_aberto());
      teclaCtx(SDLK_ESCAPE);
      quadros(20, NULL);
    }
    // Ja na pagina de estilos (colecao) o OK escolhe; no titulo, desce ate
    // "Estilo da fileira" (a ultima) e entra.
    if (!getenv("NV_CTX_COL")) {
      for (r = 0; r < 8; r++) teclaCtx(SDLK_DOWN);
      teclaCtx(SDLK_RETURN);
      quadros(40, NULL);
      snprintf(bmp, sizeof bmp, "%s-ctx-2-estilos.bmp", saida); quadros(1, bmp);
    }
    // Uma forma abaixo da atual e OK (NV_CTX_DESCE=<n>: n formas abaixo).
    { int d = getenv("NV_CTX_DESCE") ? atoi(getenv("NV_CTX_DESCE")) : 1;
      for (r = 0; r < d; r++) teclaCtx(SDLK_DOWN); }
    // NV_CTX_LADO=<n>: n vezes a direita na linha (o tamanho P M G).
    { int d = getenv("NV_CTX_LADO") ? atoi(getenv("NV_CTX_LADO")) : 0;
      for (r = 0; r < d; r++) teclaCtx(SDLK_RIGHT); }
    quadros(20, NULL);
    snprintf(bmp, sizeof bmp, "%s-ctx-3-escolha.bmp", saida); quadros(1, bmp);
    teclaCtx(SDLK_RETURN);
    quadros(120, NULL);
    printf("[shot] ctx depois da escolha aberto=%d\n", ctx_aberto());
    snprintf(bmp, sizeof bmp, "%s-ctx-4-depois.bmp", saida); quadros(1, bmp);
  }

  // NV_CTX_EXP=<n>: o menu do cartaz com a EXPANSAO do cartaz ligada (atraso de
  // 4 s). Captura: foco antes do menu, menu logo ao abrir, menu depois de o
  // atraso passar, logo apos fechar e fechado com o atraso cumprido.
  // NV_CTX_DIR=<n> cartoes a direita. Quadros no relogio real (SDL_GetTicks).
  if (getenv("NV_CTX_EXP")) {
    int r, alvo = atoi(getenv("NV_CTX_EXP"));
    Uint32 t0;
    setenv("NV_AJ", "focusedPosterBackdropExpandEnabled 1\nfocusedPosterBackdropExpandDelaySeconds 4", 1);
    ajusta(camadas[0] - '0', 0);
    for (r = 0; r < 14; r++) tecla(SDLK_UP);
    quadros(60, NULL);
    for (r = 0; r < alvo; r++) { tecla(SDLK_DOWN); quadros(40, NULL); }
    { int d = getenv("NV_CTX_DIR") ? atoi(getenv("NV_CTX_DIR")) : 0;
      for (r = 0; r < d; r++) { tecla(SDLK_RIGHT); quadros(8, NULL); } }
    quadros(30, NULL);
    snprintf(bmp, sizeof bmp, "%s-exp-a-foco.bmp", saida); quadros(1, bmp);
    snprintf(meioBmp, sizeof meioBmp, "%s-exp-a2-meio-hold.bmp", saida);
    segurarOk();
    quadros(30, NULL);
    snprintf(bmp, sizeof bmp, "%s-exp-b-menu-logo.bmp", saida); quadros(1, bmp);
    printf("[shot] exp aberto=%d\n", ctx_aberto());
    t0 = SDL_GetTicks();
    while (SDL_GetTicks() - t0 < 6500u) quadros(1, NULL);
    snprintf(bmp, sizeof bmp, "%s-exp-c-menu-apos-atraso.bmp", saida); quadros(1, bmp);
    teclaCtx(SDLK_ESCAPE); teclaCtx(SDLK_BACKSPACE);
    quadros(20, NULL);
    printf("[shot] exp fechado=%d\n", !ctx_aberto());
    snprintf(bmp, sizeof bmp, "%s-exp-d-fechado.bmp", saida); quadros(1, bmp);
    t0 = SDL_GetTicks();
    while (SDL_GetTicks() - t0 < 6500u) quadros(1, NULL);
    snprintf(bmp, sizeof bmp, "%s-exp-e-fechado-expandido.bmp", saida); quadros(1, bmp);
  }

  // NV_VISTO_CTX=<n> (#212): segurar OK no PRIMEIRO cartaz da fileira n (um
  // passo a direita: o segundo), descer NV_VISTO_DESCE linhas ate "Marcar como
  // assistido" e OK. Sem Trakt o espelho e na hora: a home depois tem de
  // mostrar o selo naquele cartaz.
  if (getenv("NV_VISTO_CTX")) {
    int r, alvo = atoi(getenv("NV_VISTO_CTX"));
    int d = getenv("NV_VISTO_DESCE") ? atoi(getenv("NV_VISTO_DESCE")) : 1;
    ajusta(camadas[0] - '0', 0);
    for (r = 0; r < 14; r++) tecla(SDLK_UP);
    quadros(60, NULL);
    for (r = 0; r < alvo; r++) { tecla(SDLK_DOWN); quadros(40, NULL); }
    tecla(SDLK_RIGHT); quadros(80, NULL);
    snprintf(bmp, sizeof bmp, "%s-visto-0-antes.bmp", saida); quadros(1, bmp);
    segurarOk();
    for (r = 0; r < d; r++) teclaCtx(SDLK_DOWN);
    quadros(60, NULL);
    snprintf(bmp, sizeof bmp, "%s-visto-1-menu.bmp", saida); quadros(1, bmp);
    teclaCtx(SDLK_RETURN);
    quadros(60, NULL);
    snprintf(bmp, sizeof bmp, "%s-visto-2-confirmado.bmp", saida); quadros(1, bmp);
    if (ctx_aberto()) { teclaCtx(SDLK_ESCAPE); teclaCtx(SDLK_BACKSPACE); }
    quadros(90, NULL);
    printf("[shot] visto ctx: menu aberto=%d\n", ctx_aberto());
    snprintf(bmp, sizeof bmp, "%s-visto-3-home.bmp", saida); quadros(1, bmp);
  }

feito:
  tex_encerrar();
  txt_encerrar();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(janela);
  SDL_Quit();
  return 0;
}
