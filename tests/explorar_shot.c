// Captura a tela Explorar 2.0 SEM janela visivel: a janela GL nasce escondida,
// o desenho vai para um FBO e o quadro sai por glReadPixels, em PNG.
//
// So o caminho LOCAL (sem chave do TMDB, sem rede): e o que a tela tem de
// mostrar sozinha. Catalogo sintetico com cartazes do pacote
// (deploy/app/art/poster); nomes de pessoas sao de exemplo.
//
// Quadros:
//   1-climas     a grade de climas (portal)
//   2-clima      o primeiro clima aberto
//   3-vizinhanca a toca no primeiro titulo do clima
//   4-trilha     depois de descer dois degraus (trilha com tres passos)
//   5-subiu      Voltar uma vez: um degrau acima
//   6-detalhe    entrada pelo Detalhe (explorar_abrir_titulo)
// Alem das capturas, confere pelo retrato publicado (mapa_vizinhos_copiar)
// que cada OK desceu de fato, que Voltar subiu e que Voltar no primeiro degrau
// da entrada pelo Detalhe pede a pagina do titulo de volta.
#include "explorar.h"
#include "mapa.h"
#include "catalogo.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "ajustes.h"
#include "rail_shot.h"
#include "dados.h"
#include "ponteiro.h"
#include "telefoneui.h"
#include "idioma.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint fbo, fboTex;
static const char *saida = "/tmp/nuvio-explorar";
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
int explorar_teste_clima_card(int indice,float valores[9]);
int explorar_teste_climas_rolagem(float valores[2]);
int explorar_teste_subview(float v[8]);
int explorar_teste_subview_card(int linha,int col,float v[14]);
int explorar_teste_subview_botao(float v[4]);
int explorar_teste_subview_hero(float v[4]);
int explorar_teste_subview_cabecalho(float v[4]);
int explorar_teste_subview_resumo(float v[6]);
int explorar_teste_subview_alvo(const PonteiroAlvo *a);
int explorar_teste_subview_texto(int linha,int col,int tipo,char *s,size_t n);
static unsigned climasMedidos;
static int pendentesAntesQuadro;
static void confereTextoQuadro(void) {
  /* txt_pendentes counts refusals over the whole process. Only this draw
     and its comparison probes must fit the current frame's text budget. */
  if(txt_pendentes!=pendentesAntesQuadro)
    fprintf(stderr,"[shot] current frame deferred %d text requests (lifetime %d)\n",
            txt_pendentes-pendentesAntesQuadro,txt_pendentes);
  assert(txt_pendentes==pendentesAntesQuadro);
}
static void confereCards(void) {
  if(!telefoneui_ativo())return;
  static MapaClimas cl; unsigned rev=0;
  assert(mapa_climas_copiar(&cl,&rev));
  int medidos=0;
  for(int i=0;i<cl.n;i++) {
    float card[9]; if(!explorar_teste_clima_card(i,card))continue;
    medidos++;
    float margem=NV_TELA_H>NV_TELA_W ? ajustes_conteudo_x() : 80;
    float antigo=(NV_TELA_W-margem-ajustes_conteudo_x()-18*3)/4;
    assert(card[2]>antigo*1.3f && card[3]>=384);
    assert(card[4]<=card[3]-48+.1f);
    float largura=card[2]-48;
    assert(fabsf(card[5]-txt_bloco(TXT_TITULO3,mapa_clima_nome(cl.c[i].id),246,246,248,0,0,largura,58,0,0))<.1f);
    assert(fabsf(card[6]-txt_bloco(TXT_HEADLINE,mapa_clima_descricao(cl.c[i].id),186,190,202,0,0,largura,48,0,0))<.1f);
    char contagem[96];
    if(cl.c[i].n>0)
      snprintf(contagem,sizeof contagem,i18n(cl.c[i].total==1 ? "%d título · você viu %d" : "%d títulos · você viu %d"),cl.c[i].total,cl.c[i].vistos);
    else snprintf(contagem,sizeof contagem,"%s",i18n("Nada deste clima no seu catálogo ainda"));
    int tr=cl.c[i].n>0?196:150,tg=cl.c[i].n>0?200:154,tb=cl.c[i].n>0?212:168;
    assert(fabsf(card[7]-txt_bloco(TXT_HEADLINE,contagem,tr,tg,tb,0,0,largura,48,0,0))<.1f);
    if(cl.c[i].n>0 && cl.c[i].afinidade>0) {
      snprintf(contagem,sizeof contagem,i18n("%d%% do que você viu"),cl.c[i].afinidade);
      float ar,ag,ab;ajustes_acento(&ar,&ag,&ab);
      tr=(int)((ar*.55f+.45f)*255);tg=(int)((ag*.55f+.45f)*255);tb=(int)((ab*.55f+.45f)*255);
      assert(fabsf(card[8]-txt_bloco(TXT_HEADLINE,contagem,tr,tg,tb,0,0,largura,48,0,0))<.1f);
    }
    TxtLinha maior=txt_linha(TXT_TITULO3,"Ag",255,255,255,255);
    TxtLinha antiga=txt_linha(TXT_HEADLINE,"Ag",255,255,255,255);
    assert(maior.h>antiga.h && maior.h>=48);
    assert(txt_linha(TXT_HEADLINE,"Ag",255,255,255,255).h>=38);
    assert(txt_linha(TXT_HEADLINE,"Ag",255,255,255,255).h>
           txt_linha(TXT_CAPTION2,"Ag",255,255,255,255).h);
    climasMedidos|=1u<<i;
    printf("[shot] full-label phone card %d: actual box %.1fx%.1f, full wrapped copy %.1f, title glyph height %d\n",
           i,card[2],card[3],card[4],maior.h);
  }
  assert(medidos>0);
  confereTextoQuadro();
}
static void dedoClima(Uint32 tipo,float x,float y) {
  SDL_Event e={0};e.type=tipo;e.tfinger.touchId=41;e.tfinger.fingerId=1;
  e.tfinger.x=x/NV_TELA_W;e.tfinger.y=y/NV_TELA_H;
  assert(ponteiro_evento(&e,explorar_evento));
}
static void rolarClimas(int capturar);
static void rolarSubview(const char *meio,const char *fim);
#endif

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  explorar_evento(&e);
}

static void quadros(int n, const char *nome) {
  int i;
  rail_shot_aplicar();
  for (i = 0; i < n; i++) {
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
    ponteiro_quadro(SDL_GetTicks());
#endif
    SDL_PumpEvents();
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
    pendentesAntesQuadro=txt_pendentes;
#endif
    txt_novo_quadro();
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
    // Warm the comparison probes over ordinary frames, so native assertions
    // never spend a fresh rasterization budget after the captured frame.
    if(telefoneui_ativo()) {
      (void)txt_linha(TXT_TITULO3,"Ag",255,255,255,255);
      (void)txt_linha(TXT_HEADLINE,"Ag",255,255,255,255);
      (void)txt_linha(TXT_BODY,"Ag",255,255,255,255);
      (void)txt_linha(TXT_CAPTION2,"Ag",255,255,255,255);
    }
#endif
    tex_novo_quadro();
    tex_bombear(10);
    gfx_novo_quadro();
    explorar_atualizar(1.0f / 60.0f, SDL_GetTicks());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.051f, 0.051f, 0.051f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    explorar_desenhar(SDL_GetTicks());
    rail_shot_desenhar(MENU_EXPLORAR);
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
    ponteiro_desenhar();
#endif
    glFinish();
    if (nome && i == n - 1) {
      unsigned char *pix = malloc(1920 * 1080 * 4);
      SDL_Surface *s;
      char cam[700];
      int y;
      assert(pix);
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      assert(s);
      for (y = 0; y < 1080; y++)
        memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      snprintf(cam, sizeof cam, "%s-%s.png", saida, nome);
      assert(IMG_SavePNG(s, cam) == 0);
      SDL_FreeSurface(s);
      free(pix);
      printf("%s  desenhos=%d\n", cam, gfx_n_rect);
    }
    SDL_Delay(2);
  }
}

#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
static void rolarClimas(int capturar) {
  if(!telefoneui_ativo())return;
  climasMedidos=0;
  confereCards();
  float r[2];assert(explorar_teste_climas_rolagem(r)&&r[1]>0);
  float alvo=r[1]*.5f;
  while(r[0]<alvo-.1f) {
    float passo=fmaxf(40,fminf(alvo-r[0],NV_TELA_H*.5f)),x=NV_TELA_W*.6f,y=NV_TELA_H*.75f;
    dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(100);
    dedoClima(SDL_FINGERMOTION,x,y-passo);
    float durante[2];assert(explorar_teste_climas_rolagem(durante));
    assert(fabsf(durante[0]-fminf(r[0]+passo,r[1]))<.1f);
    SDL_Delay(160);
    dedoClima(SDL_FINGERUP,x,y-passo);quadros(2,NULL);
    float novo[2];assert(explorar_teste_climas_rolagem(novo));assert(novo[0]>r[0]+.1f);
    assert(fabsf(novo[0]-durante[0])<.1f);
    memcpy(r,novo,sizeof r);
  }
  quadros(60,capturar ? "1a-climas-meio" : NULL);confereCards();
  for(int i=0;i<8 && r[0]<r[1]-.1f;i++) {
    float x=NV_TELA_W*.6f,y=NV_TELA_H*.75f;
    dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(100);
    dedoClima(SDL_FINGERMOTION,x,y-NV_TELA_H*.5f);SDL_Delay(160);
    dedoClima(SDL_FINGERUP,x,y-NV_TELA_H*.5f);quadros(2,NULL);
    assert(explorar_teste_climas_rolagem(r));
  }
  assert(fabsf(r[0]-r[1])<.1f);
  quadros(60,capturar ? "1b-climas-fim" : NULL);confereCards();
  assert(climasMedidos==(1u<<MAPA_CLIMA_N)-1);
  int abriu=-1;assert(!explorar_pediu_abrir(&abriu));
  explorar_iniciar();quadros(60,NULL);
}
static void shotIdioma(int ingles) {
  char caminho[700];snprintf(caminho,sizeof caminho,"%s/ajustes.txt",dados_dir());
  FILE *f=fopen(caminho,"w");assert(f);
  const char *tema=getenv("NUVIO_SHOT_THEME"),*reduz=getenv("NUVIO_SHOT_REDUZ");
  fprintf(f,"idioma %d\nselected_theme %d\nanimacoes %d\n",ingles,tema&&*tema?atoi(tema):2,reduz&&*reduz=='1');
  fclose(f);ajustes_dir(dados_dir());assert(ajustes_idioma_ingles()==ingles);
}
#endif

// --- catalogo sintetico ---------------------------------------------------------

typedef struct {
  const char *titulo, *genero, *meta, *poster, *tipo, *direcao, *ator;
  int nota, progresso;
} Linha;
static const Linha CAT[] = {
  { "The Prestige", "Filme  ·  Drama  ·  Mistério", "2006  ·  130 min", "02", "movie", "Lena Hart", "Tomás Weber", 85, 96 },
  { "Frequency", "Filme  ·  Crime  ·  Drama", "2000  ·  118 min", "05", "movie", "Iris Moon", "Paulo Vidal", 72, 96 },
  { "Prisoners", "Filme  ·  Crime  ·  Drama", "2013  ·  153 min", "09", "movie", "Iris Moon", "Tomás Weber", 81, 45 },
  { "The Martian", "Filme  ·  Aventura  ·  Drama", "2015  ·  141 min", "12", "movie", "Rui Sato", "Ana Kowalski", 80, 96 },
  { "3 Body Problem", "Série  ·  Ficção científica  ·  Mistério", "2024", "15", "series", "", "Ana Kowalski", 76, 45 },
  { "Lost", "Série  ·  Mistério  ·  Aventura", "2004", "21", "series", "", "Paulo Vidal", 83, 0 },
  { "The Mist", "Série  ·  Ficção científica  ·  Mistério", "2017", "19", "series", "", "Tomás Weber", 64, 0 },
  { "Maniac", "Série  ·  Comédia  ·  Drama", "2018", "39", "series", "", "Ana Kowalski", 77, 0 },
  { "Project Hail Mary", "Filme  ·  Aventura  ·  Ficção científica", "2026  ·  157 min", "13", "movie", "Rui Sato", "Paulo Vidal", 84, 0 },
  { "From", "Série  ·  Drama  ·  Terror", "2022", "04", "series", "", "Paulo Vidal", 77, 0 },
  { "Space/Time", "Filme  ·  Ficção científica  ·  Mistério", "2025  ·  90 min", "14", "movie", "Lena Hart", "Ana Kowalski", 61, 0 },
  { "Mr. K", "Filme  ·  Drama  ·  Mistério", "2025  ·  96 min", "38", "movie", "Lena Hart", "Tomás Weber", 66, 0 },
  { "Locke & Key", "Série  ·  Fantasia  ·  Drama", "2020", "27", "series", "", "Tomás Weber", 73, 0 },
  { "WandaVision", "Série  ·  Ficção científica  ·  Mistério", "2021", "31", "series", "", "Ana Kowalski", 79, 0 },
  { "Fallout", "Série  ·  Ação  ·  Aventura", "2024", "00", "series", "", "Paulo Vidal", 82, 0 },
  { "Extrapolations", "Série  ·  Drama  ·  Ficção científica", "2023", "17", "series", "", "Ana Kowalski", 60, 0 },
  { "The Umbrella Academy", "Série  ·  Ação  ·  Ficção científica", "2019", "35", "series", "", "Paulo Vidal", 76, 0 },
  { "IT: Welcome to Derry", "Série  ·  Drama  ·  Mistério", "2025", "29", "series", "", "Tomás Weber", 78, 0 },
  { "Zodiac", "Filme  ·  Crime  ·  Mistério", "2007  ·  157 min", "07", "movie", "Iris Moon", "Paulo Vidal", 77, 0 },
  { "Severance", "Série  ·  Drama  ·  Mistério", "2022", "23", "series", "", "Ana Kowalski", 87, 0 },
  { "Dark Comedy Night", "Filme  ·  Comédia  ·  Crime", "2019  ·  101 min", "33", "movie", "Rui Sato", "Paulo Vidal", 70, 0 },
};
#define NCAT (int)(sizeof CAT / sizeof CAT[0])

static void semearCatalogo(void) {
  static CatItem itens[NCAT];
  int i;
  memset(itens, 0, sizeof itens);
  for (i = 0; i < NCAT; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt-exp-%02d", i);
    snprintf(c->tipo, sizeof c->tipo, "%s", CAT[i].tipo);
    snprintf(c->titulo, sizeof c->titulo, "%s", CAT[i].titulo);
    snprintf(c->genero, sizeof c->genero, "%s", CAT[i].genero);
    snprintf(c->meta, sizeof c->meta, "%s", CAT[i].meta);
    snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%s.jpg", CAT[i].poster);
    snprintf(c->direcao, sizeof c->direcao, "%s", CAT[i].direcao);
    snprintf(c->elenco[0].nome, sizeof c->elenco[0].nome, "%s", CAT[i].ator);
    c->nElenco = 1;
    c->nota = CAT[i].nota;
    c->progresso = CAT[i].progresso;
  }
  cat_definir_tudo(itens, NCAT, NULL, 0);
}

static void focoPublicado(char *dst, size_t n, int *grupos) {
  static MapaVizinhos v;
  unsigned rev = 0;
  int g;
  assert(mapa_vizinhos_copiar(&v, &rev));
  snprintf(dst, n, "%s", v.foco.titulo);
  *grupos = 0;
  for (g = 0; g < MAPA_VIZ_GRUPOS; g++) if (v.g[g].n > 0) (*grupos)++;
}

#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
static int confereSubview(void) {
  float p[8];assert(explorar_teste_subview(p));
  assert(p[1]>=0 && p[1]<=p[2]+.1f && p[3]>=0 && p[4]<=NV_TELA_H);
  assert(fabsf(p[5]-gfx_escala_ui())<.01f);
  float cab[4];
  if(!menu_pilula_titulo()) {
    assert(explorar_teste_subview_cabecalho(cab));
    assert(cab[0]>=0 && cab[1]>=0 && cab[0]+cab[2]<=NV_TELA_W+.1f && cab[1]+cab[3]<p[3]);
  } else assert(!explorar_teste_subview_cabecalho(cab));
  int medidos=0;
  for(int l=0;l<4;l++)for(int i=0;i<24;i++) {
    float r[14];if(!explorar_teste_subview_card(l,i,r))continue;medidos++;
    assert(r[0]<r[13] && r[0]+r[2]>r[12] && r[1]<p[4] && r[1]+r[3]>p[3]);
    assert(r[9]>=38*p[5] && r[11]>0 && r[2]<=r[13]-r[12]+.1f);
    char titulo[512];assert(explorar_teste_subview_texto(l,i,0,titulo,sizeof titulo));
    float esperado=txt_bloco(TXT_HEADLINE,i18n(titulo),240,241,245,0,0,r[11]/p[5],48,0,0)*p[5];
    assert(fabsf(r[7]-esperado)<.1f);
    if((int)p[0]==2) {
      char pq[512];assert(explorar_teste_subview_texto(l,i,1,pq,sizeof pq));
      esperado=txt_bloco(TXT_HEADLINE,i18n(pq),170,176,196,0,0,r[11]/p[5],48,0,0)*p[5];
      assert(fabsf(r[8]-esperado)<.1f && r[7]+12*p[5]+r[8]+24*p[5]<=r[3]+.1f);
      assert(r[4]>=160*p[5] && r[5]>=240*p[5]);
    } else {
      assert(r[4]>140*p[5] && r[5]+16*p[5]+r[7]<=r[3]+.1f);
    }
  }
  if((int)p[0]==2) {
    float hero[4];assert(explorar_teste_subview_hero(hero));
    for(int k=0;k<3;k++) {
      char texto[512];assert(explorar_teste_subview_texto(-1,0,k,texto,sizeof texto));
      float h=txt_bloco(k==0?TXT_TITULO3:TXT_HEADLINE,i18n(texto),k==0?246:k==1?186:160,
                       k==0?246:k==1?190:166,k==0?248:k==1?202:182,0,0,hero[3]/p[5],k==0?58:48,0,0)*p[5];
      assert(fabsf(hero[k]-h)<.1f);
    }
  }
  const PonteiroAlvo *alvos;int n=ponteiro_teste_lista(&alvos);
  for(int i=0;i<n;i++)if(explorar_teste_subview_alvo(&alvos[i])) {
    assert(alvos[i].x>=0 && alvos[i].y>=p[3]-.1f && alvos[i].w>0 && alvos[i].h>0);
    assert(alvos[i].x+alvos[i].w<=NV_TELA_W+.1f && alvos[i].y+alvos[i].h<=p[4]+.1f);
  }
  confereTextoQuadro();
  return medidos;
}
static void moverPagina(float destino) {
  float p[8];assert(explorar_teste_subview(p));
  for(int i=0;i<20 && fabsf(destino-p[1])>.1f;i++) {
    float dy=fmaxf(-(p[4]-p[3])*.55f,fminf((p[4]-p[3])*.55f,p[1]-destino));
    float x=NV_TELA_W*.6f,y=(p[3]+p[4])*.5f-dy*.5f;
    dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(100);
    dedoClima(SDL_FINGERMOTION,x,y+dy);SDL_Delay(160);
    dedoClima(SDL_FINGERUP,x,y+dy);quadros(2,NULL);
    assert(explorar_teste_subview(p));
  }
  assert(fabsf(p[1]-destino)<.1f);
}
static void rolarSubview(const char *meio,const char *fim) {
  if(!telefoneui_ativo())return;
  float p[8],cabAntes[4];assert(explorar_teste_subview(p));confereSubview();
  int modoAntes=(int)p[0],grupos=0;char focoAntes[128]="";
  if(modoAntes==2)focoPublicado(focoAntes,sizeof focoAntes,&grupos);
  int temCab=explorar_teste_subview_cabecalho(cabAntes);
  if(p[2]>0) {
    float passo=fminf(150,p[2]-p[1]),x=NV_TELA_W*.6f,y=p[4]-60;
    float cardAntes[14],resumoAntes[6];int linha=-1,coluna=-1;
    for(int l=0;l<4 && linha<0;l++)for(int c=0;c<24;c++)if(explorar_teste_subview_card(l,c,cardAntes) &&
      cardAntes[1]+cardAntes[3]>p[3]+passo){linha=l;coluna=c;break;}
    int temResumo=explorar_teste_subview_resumo(resumoAntes);
    float antes=p[1];dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(100);
    dedoClima(SDL_FINGERMOTION,x,y-passo);
    float durante[8];assert(explorar_teste_subview(durante));assert(fabsf(durante[1]-antes-passo)<.1f);
    quadros(2,NULL);confereSubview();
    if(linha>=0) {
      float depois[14];assert(explorar_teste_subview_card(linha,coluna,depois));
      assert(fabsf(depois[1]-cardAntes[1]+passo)<.1f && fabsf(depois[0]-cardAntes[0])<.1f);
    }
    if(temResumo) {
      float depois[6];assert(explorar_teste_subview_resumo(depois));
      assert(fabsf(depois[1]-resumoAntes[1]+passo)<.1f && fabsf(depois[5]-resumoAntes[5]+passo)<.1f);
      assert(fabsf(depois[0]-resumoAntes[0])<.1f && fabsf(depois[4]-resumoAntes[4])<.1f);
    }
    SDL_Delay(160);dedoClima(SDL_FINGERUP,x,y-passo);quadros(3,NULL);
    assert(explorar_teste_subview(durante));assert(fabsf(durante[1]-antes-passo)<.1f && (int)durante[0]==modoAntes);
    if(temCab) {
      float cabDepois[4];assert(explorar_teste_subview_cabecalho(cabDepois));
      for(int i=0;i<4;i++)assert(fabsf(cabDepois[i]-cabAntes[i])<.1f);
    }
    float curto=fminf(80,p[2]-durante[1]),antesCancelar=durante[1];
    if(curto>0) {
      dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(100);dedoClima(SDL_FINGERMOTION,x,y-curto);
      ponteiro_cancelar_toque();dedoClima(SDL_FINGERUP,x,y-curto);quadros(3,NULL);
      assert(explorar_teste_subview(durante));
      assert(fabsf(durante[1]-antesCancelar-curto)<.1f && (int)durante[0]==modoAntes);
    }
  }
  moverPagina(p[2]*.5f);quadros(60,meio);confereSubview();
  moverPagina(p[2]);quadros(60,NULL);assert(confereSubview()>0);
  /* Test horizontal movement on the final visible row, using actual rendered
     coordinates and the real map row count; short rows have no overflow. */
  int ultima=-1;float r[14];
  for(int l=0;l<4;l++)if(explorar_teste_subview_card(l,0,r))ultima=l;
  if(ultima>=0 && modoAntes==2) {
    static MapaVizinhos v;unsigned rev=0;assert(mapa_vizinhos_copiar(&v,&rev));
    int linha=-1,n=0;for(int g=0;g<4;g++)if(v.g[g].n>0){if(++linha==ultima)n=v.g[g].n;}
    assert(explorar_teste_subview_card(ultima,0,r));
    float max=fmaxf(0,n*(r[2]+24*p[5])-24*p[5]-(r[13]-r[12]));
    if(max>120) {
      float x=(fmaxf(r[0],r[12])+fminf(r[0]+r[2],r[13]))*.5f;
      float y=(fmaxf(r[1],p[3])+fminf(r[1]+r[3],p[4]))*.5f,antesX=r[0];
      dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(100);dedoClima(SDL_FINGERMOTION,x-120,y);quadros(2,NULL);
      float novo[14];assert(explorar_teste_subview_card(ultima,0,novo));assert(fabsf(novo[0]-antesX+120)<.1f);
      SDL_Delay(160);dedoClima(SDL_FINGERUP,x-120,y);quadros(3,NULL);confereSubview();
      float novoP[8];assert(explorar_teste_subview(novoP));assert(novoP[1]==p[2] && (int)novoP[0]==modoAntes);
    }
  }
  quadros(60,fim);confereSubview();
  if(modoAntes==2) {char focoDepois[128];focoPublicado(focoDepois,sizeof focoDepois,&grupos);assert(!strcmp(focoAntes,focoDepois));}
  int abriu=-1;assert(!explorar_pediu_abrir(&abriu));
  const PonteiroAlvo *alvos;int n=ponteiro_teste_lista(&alvos),tocou=0;
  for(int i=0;i<n;i++)if(explorar_teste_subview_alvo(&alvos[i]) && alvos[i].a>=0 && alvos[i].h>=40) {
    PonteiroAlvo alvo=alvos[i];char esperado[512];assert(explorar_teste_subview_texto(alvo.a,alvo.b,0,esperado,sizeof esperado));
    float x=alvo.x+alvo.w*.5f,y=alvo.y+alvo.h*.5f;
    dedoClima(SDL_FINGERDOWN,x,y);SDL_Delay(20);dedoClima(SDL_FINGERUP,x,y);quadros(70,NULL);
    char focoDepois[128];focoPublicado(focoDepois,sizeof focoDepois,&grupos);assert(!strcmp(esperado,focoDepois));
    tecla(SDLK_ESCAPE);quadros(70,NULL);float voltou[8];assert(explorar_teste_subview(voltou));
    assert((int)voltou[0]==modoAntes && fabsf(voltou[1]-p[2])<.1f);
    if(modoAntes==2){focoPublicado(focoDepois,sizeof focoDepois,&grupos);assert(!strcmp(focoAntes,focoDepois));}
    tocou=1;break;
  }
  assert(tocou);
  moverPagina(0);quadros(3,NULL);confereSubview();
  printf("[shot] Explore phone subview mode%d ui%.2f: complete real-font labels, fixed header, vertical finger delta, horizontal row delta, release identity and middle/end captures passed (page max %.1f)\n",modoAntes,p[5],p[2]);
}
#endif

int main(int argc, char **argv) {
  SDL_Window *win;
  SDL_GLContext gl;
  char raiz[128], passo1[128], passo2[128], volta[128];
  int grupos;
  if (argc > 1) saida = argv[1];
  // Idioma, acento e animacoes pelo caminho de verdade (ajustes.txt na pasta
  // de dados temporaria): NUVIO_SHOT_EN=1 para ingles, NUVIO_SHOT_THEME=<n>
  // para o acento, NUVIO_SHOT_REDUZ=1 para animacoes reduzidas.
  ajustes_iniciar();
  dados_iniciar("deploy/app/art");
  { char caminho[700]; FILE *f;
    const char *en = getenv("NUVIO_SHOT_EN"), *tema = getenv("NUVIO_SHOT_THEME");
    const char *reduz = getenv("NUVIO_SHOT_REDUZ");
    snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dados_dir());
    f = fopen(caminho, "w");
    assert(f);
    fprintf(f, "idioma %d\nselected_theme %d\nanimacoes %d\n",
            en && *en == '1', tema && *tema ? atoi(tema) : 2, reduz && *reduz == '1');
    fclose(f);
    ajustes_dir(dados_dir()); }
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  win = SDL_CreateWindow("explorar-shot", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(win);
  gl = SDL_GL_CreateContext(win);
  assert(gl);
  glGenTextures(1, &fboTex);
  glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(64);
  gfx_tex_esquecer(0);
  semearCatalogo();

  // Os climas saem do catalogo, sem rede: o primeiro (maior afinidade) tem de
  // ter titulos.
  { static MapaClimas cl;
    unsigned rev = 0;
    mapa_climas_pedir();
    assert(mapa_climas_copiar(&cl, &rev));
    assert(cl.n == MAPA_CLIMA_N && cl.c[0].n > 0 && cl.c[0].afinidade >= cl.c[1].afinidade); }

  explorar_iniciar();
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  ponteiro_iniciar();ponteiro_teste_toque(1);
#endif
  quadros(90, "1-climas");
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  rolarClimas(1);
  if(telefoneui_ativo()) {
    int ingles=ajustes_idioma_ingles();shotIdioma(!ingles);explorar_iniciar();quadros(90,NULL);
    rolarClimas(0);
    printf("[shot] Explore landing complete real-font labels and continuous drag passed in English and Portuguese\n");
    shotIdioma(ingles);explorar_iniciar();quadros(60,NULL);
  }
#endif
  tecla(SDLK_RETURN);
  quadros(70, "2-clima");
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  rolarSubview("2a-clima-meio","2b-clima-fim");
#endif

  tecla(SDLK_RETURN);
  quadros(70, "3-vizinhanca");
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  rolarSubview("3a-vizinhanca-meio","3b-vizinhanca-fim");
#endif
  focoPublicado(raiz, sizeof raiz, &grupos);
  assert(raiz[0] && grupos >= 2);

  // Dois degraus: o primeiro cartaz do primeiro grupo, depois o segundo cartaz.
  tecla(SDLK_RETURN);
  quadros(10, NULL);
  focoPublicado(passo1, sizeof passo1, &grupos);
  assert(strcmp(passo1, raiz) && grupos >= 1);
  tecla(SDLK_RIGHT);
  tecla(SDLK_RETURN);
  quadros(70, "4-trilha");
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  rolarSubview("4a-trilha-meio","4b-trilha-fim");
#endif
  focoPublicado(passo2, sizeof passo2, &grupos);
  assert(strcmp(passo2, passo1) && strcmp(passo2, raiz));

  tecla(SDLK_ESCAPE);
  quadros(50, "5-subiu");
  focoPublicado(volta, sizeof volta, &grupos);
  assert(!strcmp(volta, passo1));

  // Entrada pelo Detalhe: a toca comeca no titulo da pagina.
  explorar_iniciar();
  { MapaObra o;
    assert(mapa_obra_do_catalogo(2, &o));
    explorar_abrir_titulo(&o); }
  quadros(70, "6-detalhe");
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  rolarSubview("6a-detalhe-meio","6b-detalhe-fim");
#endif
  focoPublicado(volta, sizeof volta, &grupos);
  assert(!strcmp(volta, "Prisoners"));
  // Voltar no primeiro degrau devolve a pagina do titulo.
  tecla(SDLK_ESCAPE);
  { int idx = -1;
    assert(explorar_pediu_abrir(&idx) && idx == 2); }
#if defined(NV_TOUCH_UI) && defined(NV_SHOT_HOOKS)
  if(telefoneui_ativo()) {
    int ingles=ajustes_idioma_ingles();shotIdioma(!ingles);explorar_iniciar();quadros(70,NULL);
    tecla(SDLK_RETURN);quadros(70,NULL);rolarSubview(NULL,NULL);
    tecla(SDLK_RETURN);quadros(70,NULL);rolarSubview(NULL,NULL);
    shotIdioma(ingles);
    printf("[shot] Explore subviews complete real-font labels and continuous page/row gestures passed in English and Portuguese\n");
  }
#endif

  printf("explorar_shot: raiz=%s -> %s -> %s; capturas gravadas\n", raiz, passo1, passo2);
  explorar_encerrar();
  tex_encerrar();
  txt_encerrar();
  gfx_encerrar();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(win);
  IMG_Quit();
  SDL_Quit();
  return 0;
}
