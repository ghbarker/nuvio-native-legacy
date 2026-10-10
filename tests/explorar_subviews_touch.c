/* Real Explore render/update/pointer paths; offline map and font doubles.
 * Native real-font/GL assertions in explorar_shot.c verify the same pages. */
#define EXPLORAR_LANDING_EMBED 1
#include "explorar_landing_touch.c"

static void teclaSubview(SDL_Keycode k) {
  SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;explorar_evento(&e);quadro();
}
static void semearSubviews(void) {
  climas.n=MAPA_CLIMA_N;
  for(int i=0;i<climas.n;i++) {
    MapaClima *c=&climas.c[i];c->id=i;c->n=24;
    for(int j=0;j<24;j++){obra(&c->itens[j],j);c->visto[j]=j>=12;}
  }
  memset(&viz,0,sizeof viz);obra(&viz.foco,99);strcpy(viz.generos,"Drama · Misterio · Ficcao cientifica");
  for(int g=0;g<MAPA_VIZ_GRUPOS;g++) {
    MapaVizGrupo *grupo=&viz.g[g];grupo->n=MAPA_VIZ_ITENS;
    grupo->tipo=g==3?MAPA_GR_AMIGOS:g==2?MAPA_GR_REC:g==1?MAPA_GR_GENERO:MAPA_GR_PESSOA;
    strcpy(grupo->sub,"Um genero ou amigo com nome completo");
    for(int i=0;i<grupo->n;i++) {obra(&grupo->itens[i].obra,10*g+i);strcpy(grupo->itens[i].motivo,"Uma pessoa com sobrenome comprido");}
  }
}
static void abrirSubview(int m,float w,float h,float s) {
  uiScale=s;preparar(w,h,1);semearSubviews();
  modo=m;clAberto=0;caLinha=caCol[0]=caCol[1]=0;climaSeparar();
  nTrilha=1;origem=ORIGEM_CLIMA;trilha[0].obra=viz.foco;
  vzLinha=vzCol=0;memset(toqueVz,0,sizeof toqueVz);memset(toqueVzOffset,0,sizeof toqueVzOffset);
  memset(toqueCa,0,sizeof toqueCa);caRolar[0]=caRolar[1]=0;
  detalhePedido[0]=0;memset(&passoPedido,0,sizeof passoPedido);quadro();
}
static void confereAlvos(void) {
  const PonteiroAlvo *lista;int n=ponteiro_teste_lista(&lista);assert(n>0);
  for(int i=0;i<n;i++) {
    assert(lista[i].x>=0 && lista[i].y>=0);
    assert(lista[i].x+lista[i].w<=NV_TELA_W+.1f && lista[i].y+lista[i].h<=NV_TELA_H+.1f);
    if(lista[i].focar==ponteiroClima || lista[i].focar==ponteiroViz) {
      assert(lista[i].y>=exPlano.topo*uiScale-.1f);
      assert(lista[i].y+lista[i].h<=exPlano.baixo*uiScale+.1f);
    }
  }
}
static int tocar(PonteiroFn fn,int l,int c) {
  const PonteiroAlvo *lista;int n=ponteiro_teste_lista(&lista);
  for(int i=0;i<n;i++)if(lista[i].focar==fn && lista[i].a==l && lista[i].b==c) {
    float x=lista[i].x+lista[i].w*.5f,y=lista[i].y+lista[i].h*.5f;
    dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=20;dedo(SDL_FINGERUP,x,y,1);quadro();return 1;
  }
  return 0;
}
static void publicarVizinhos(const MapaVizinhos *v) {publicarViz=*v;publicarVizPendente=1;}
static void publicarClima(const MapaClimas *c) {publicarClimas=*c;publicarClimasPendente=1;}
static void cancelarTituloSubstituido(int m,float w,float h,float s) {
  abrirSubview(m,w,h,s);exPagina=toquePagina.maximo;toquePagina.livre=1;quadro();
  const PonteiroAlvo *lista;int n=ponteiro_teste_lista(&lista),achou=0;PonteiroAlvo alvo={0};
  PonteiroFn f=m==MODO_CLIMAS?ponteiroClimas:m==MODO_CLIMA?ponteiroClima:ponteiroViz;
  for(int i=0;i<n;i++)if(lista[i].focar==f && lista[i].a>=0){alvo=lista[i];achou=1;break;}
  assert(achou);
  float x=alvo.x+alvo.w*.5f,y=alvo.y+alvo.h*.5f,salvo=exPagina;
  dedo(SDL_FINGERDOWN,x,y,1);
  if(m==MODO_VIZ) {
    publicarViz=viz;int g=exPlano.grupos[alvo.a];
    strcpy(publicarViz.g[g].itens[alvo.b].obra.imdb,"tt-new-publication");
    strcpy(publicarViz.g[g].itens[alvo.b].obra.titulo,"Outro titulo publicado");publicarVizPendente=1;
  } else if(m==MODO_CLIMA) {
    publicarClimas=climas;int c=caIdx[alvo.a][alvo.b];
    strcpy(publicarClimas.c[0].itens[c].imdb,"tt-new-publication");
    strcpy(publicarClimas.c[0].itens[c].titulo,"Outro titulo publicado");publicarClimasPendente=1;
  } else {
    publicarClimas=climas;publicarClimas.c[alvo.a].id=MAPA_CLIMA_N-1;publicarClimasPendente=1;
  }
  quadro();relogioTeste+=20;dedo(SDL_FINGERUP,x,y,1);quadro();
  assert(modo==m && nTrilha==1 && !passoPedido.imdb[0] && !detalhePedido[0] && abriuClima<0);
  /* New shorter copy may change legal bounds; cancellation itself does not
     reset the retained scroll or force the old TV focus target. */
  perto(exPagina,fminf(salvo,toquePagina.maximo));
}
static int camadaCliques,camadaFocos;
static ToqueRolagem camadaRolar;
static float camadaY;
static void focoCamada(int a,int b){(void)a;(void)b;camadaFocos++;}
static void ativarCamada(int a,int b){(void)a;(void)b;camadaCliques++;}
static int rolarCamada(const PonteiroRolagem *e){return toquerol_evento(&camadaRolar,e);}
static void quadroCamada(int tipo) {
  relogioTeste+=16;ponteiro_quadro(relogioTeste);clipped=0;phase=0;
  explorar_atualizar(1.0f/60,relogioTeste);explorar_desenhar(relogioTeste);
  if(tipo!=2)ponteiro_camada();
  if(tipo==3) {
    toquerol_vincular(&camadaRolar,(GfxRect){200,300,500,1000},1,0,1000,1,&camadaY);
    ponteiro_rolagem(rolarCamada);
  }
  if(tipo==2)ponteiro_alvo(15,200,65,600,focoCamada,ativarCamada,0,0);
  else ponteiro_alvo(200,300,500,1000,tipo==1?NULL:focoCamada,ativarCamada,0,0);
  ponteiro_desenhar();
}
static void publicarModo(int m) {
  if(m==MODO_VIZ)publicarVizinhos(&viz);else publicarClima(&climas);
}
static void preservarCamada(int m,int tipo) {
  abrirSubview(m,1080,2340,1.5f);
  camadaCliques=camadaFocos=0;camadaY=0;camadaRolar=(ToqueRolagem){0};quadroCamada(tipo);
  float x=tipo==2?40:450,y=tipo==2?450:900;
  dedo(SDL_FINGERDOWN,x,y,1);
  if(tipo==3) {
    relogioTeste+=100;dedo(SDL_FINGERMOTION,x,y-140,1);perto(camadaY,140);
  }
  publicarModo(m);quadroCamada(tipo);
  if(tipo==3) {
    perto(camadaY,140);relogioTeste+=100;dedo(SDL_FINGERMOTION,x,y-240,1);perto(camadaY,240);
    relogioTeste+=160;dedo(SDL_FINGERUP,x,y-240,1);quadroCamada(tipo);
    perto(camadaY,240);assert(!camadaCliques);
  } else {
    relogioTeste+=20;dedo(SDL_FINGERUP,x,y,1);quadroCamada(tipo);
    assert(camadaCliques==1);
  }
  assert(modo==m && nTrilha==1 && !passoPedido.imdb[0] && !detalhePedido[0]);
}
static void preservarArrastoExplore(int m,int eixoY) {
  abrirSubview(m,1080,1920,1.5f);
  if(!eixoY) {exPagina=toquePagina.maximo;toquePagina.livre=1;quadro();}
  int l=m==MODO_VIZ?exPlano.n-1:1;
  float s=uiScale,x=700,y=1200;
  float *offset=&exPagina;
  if(!eixoY) {
    y=(fmaxf(exPlano.y[l]+exPlano.cabH[l]+16-exPagina,exPlano.topo)+
       fminf(exPlano.y[l]+exPlano.cabH[l]+16+exPlano.cardH[l]-exPagina,exPlano.baixo))*.5f*s;
    offset=m==MODO_VIZ?&toqueVzOffset[exPlano.grupos[l]]:&caRolar[l];
  }
  float antes=*offset;dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=100;
  dedo(SDL_FINGERMOTION,x-(eixoY?0:120),y-(eixoY?120:0),1);perto(*offset,antes+120/s);
  publicarModo(m);quadro();perto(*offset,antes+120/s);
  relogioTeste+=100;dedo(SDL_FINGERMOTION,x-(eixoY?0:220),y-(eixoY?220:0),1);perto(*offset,antes+220/s);
  relogioTeste+=160;dedo(SDL_FINGERUP,x-(eixoY?0:220),y-(eixoY?220:0),1);quadro();perto(*offset,antes+220/s);
  assert(modo==m && nTrilha==1 && !passoPedido.imdb[0] && !detalhePedido[0]);
}
static void voltarPublicacao(int m,int interagir) {
  abrirSubview(m,1080,1920,1.5f);
  exPagina=toquePagina.maximo;toquePagina.livre=1;quadro();float salvo=exPagina;assert(salvo>100);
  static MapaVizinhos pai;static MapaClimas climaCompleto;
  pai=viz;climaCompleto=climas;
  if(m==MODO_VIZ)descer(&viz.g[0],&viz.g[0].itens[0]);
  else comecarToca(&climas.c[0].itens[0],ORIGEM_CLIMA);
  publicarViz=pai;publicarViz.carregando=1;for(int g=0;g<4;g++)publicarViz.g[g].n=0;publicarVizPendente=1;
  publicarClimas=climaCompleto;publicarClimas.c[0].carregando=1;publicarClimas.c[0].n=0;publicarClimasPendente=1;
  quadro();teclaSubview(SDLK_ESCAPE);
  assert(modo==m && exPaginaRestaurar && exPaginaDesejada==salvo && exPagina<salvo-100);
  float provisoria=exPagina;
  for(int i=0;i<5;i++)quadro();perto(exPagina,provisoria);assert(exPaginaRestaurar);
  if(interagir==1 || interagir==2) {
    if(interagir==1)teclaSubview(SDLK_DOWN);
    else arrastar(700,1000,0,-120);
    assert(!exPaginaRestaurar);
  }
  if(interagir==3) {for(int g=0;g<4;g++)pai.g[g].n=0;climaCompleto.c[0].n=0;}
  publicarVizinhos(&pai);publicarClima(&climaCompleto);quadro();
  assert(!exPaginaRestaurar);
  if(!interagir || interagir==3)perto(exPagina,fminf(salvo,toquePagina.maximo));
  else assert(exPagina<salvo-100);
}
int main(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};int casos=0;
  for(int t=0;t<4;t++)for(int u=0;u<2;u++)for(int m=MODO_CLIMA;m<=MODO_VIZ;m++) {
    float w=telas[t][0],h=telas[t][1],s=u?1.5f:1;abrirSubview(m,w,h,s);
    assert(toquePagina.maximo>=0);if(m==MODO_VIZ)assert(toquePagina.maximo>0);confereAlvos();
    assert(exPlano.w*s==EX_DIR-ajustes_conteudo_x());
    if(m==MODO_VIZ) {
      assert(exPlano.y[0]>exPlano.poster.y+exPlano.poster.h);
      assert(exPlano.botao.h*s>=128 && exPlano.textoW>0);
      assert(telefoneCardPasso()-24>=246*1.5f);
      assert(exPlano.tituloH[0][0]+12+exPlano.motivoH[0][0]+24<=exPlano.cardH[0]+.1f);
    } else assert(telefoneClimaPasso()-EX_CAR_GAP>EX_CB_W*1.5f || s>1);
    casos++;
    float x=w*.65f,y=h*.72f,antes=exPagina;
    dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=100;dedo(SDL_FINGERMOTION,x,y-150,1);
    perto(exPagina,fminf(antes+150/s,toquePagina.maximo));quadro();
    relogioTeste+=100;dedo(SDL_FINGERMOTION,x,y-300,1);perto(exPagina,fminf(antes+300/s,toquePagina.maximo));
    relogioTeste+=160;dedo(SDL_FINGERUP,x,y-300,1);quadro();
    assert(modo==m && nTrilha==1 && !teclasTeste && !passoPedido.titulo[0] && !detalhePedido[0]);casos++;
    float guardado=exPagina;for(int i=0;i<30;i++)quadro();perto(exPagina,guardado);casos++;
    dedo(SDL_FINGERDOWN,x,y,1);relogioTeste+=100;dedo(SDL_FINGERMOTION,x,y-120,1);
    guardado=exPagina;ponteiro_cancelar_toque();dedo(SDL_FINGERUP,x,y-120,1);quadro();
    perto(exPagina,guardado);assert(modo==m && nTrilha==1 && !teclasTeste);casos++;
    for(int i=0;i<20;i++)arrastar(x,h*.75f,0,-h*.45f);
    perto(exPagina,toquePagina.maximo);confereAlvos();casos++;
    int ultima=m==MODO_VIZ?exPlano.n-1:1;
    if(m==MODO_VIZ)assert(toqueVz[exPlano.grupos[ultima]].offset);
    else assert(toqueCa[ultima].offset);
    float *offset=m==MODO_VIZ?&toqueVzOffset[exPlano.grupos[ultima]]:&caRolar[ultima];
    float fy=(fmaxf(exPlano.y[ultima]+exPlano.cabH[ultima]+16-exPagina,exPlano.topo)+
              fminf(exPlano.y[ultima]+exPlano.cabH[ultima]+16+exPlano.cardH[ultima]-exPagina,exPlano.baixo))*.5f*s;
    antes=*offset;guardado=exPagina;
    dedo(SDL_FINGERDOWN,w*.65f,fy,1);relogioTeste+=100;dedo(SDL_FINGERMOTION,w*.65f-160,fy,1);
    perto(*offset,antes+160/s);perto(exPagina,guardado);quadro();
    relogioTeste+=160;dedo(SDL_FINGERUP,w*.65f-160,fy,1);quadro();
    assert(modo==m && nTrilha==1 && !teclasTeste && !passoPedido.titulo[0]);casos++;
    teclaSubview(SDLK_UP);assert(!toquePagina.livre);
    int selecionada=m==MODO_VIZ?vzLinha:caLinha;
    assert(selecionada==(ultima>0?ultima-1:0));
    assert(exPlano.y[selecionada]-exPagina>=exPlano.topo-.1f ||
           exPlano.cabH[selecionada]+16+exPlano.cardH[selecionada]>exPlano.baixo-exPlano.topo);
    casos++;
    /* Middle/end pointer targets remain actionable; the selected payload is
       checked against the exact map item, not an invented key step. */
    exPagina=toquePagina.maximo;toquePagina.livre=1;quadro();
    int col= m==MODO_VIZ ? (int)(*offset/telefoneCardPasso()) : (int)(*offset/telefoneClimaPasso());
    if(m==MODO_VIZ) {
      MapaObra esperado=viz.g[exPlano.grupos[ultima]].itens[col].obra;
      assert(tocar(ponteiroViz,ultima,col));assert(nTrilha==2 && !strcmp(passoPedido.imdb,esperado.imdb));
      teclaSubview(SDLK_ESCAPE);assert(nTrilha==1);perto(exPagina,guardado);
    } else {
      MapaObra esperado=climas.c[0].itens[caIdx[ultima][col]];
      assert(tocar(ponteiroClima,ultima,col));assert(modo==MODO_VIZ && !strcmp(passoPedido.imdb,esperado.imdb));
      teclaSubview(SDLK_ESCAPE);assert(modo==MODO_CLIMA);perto(exPagina,guardado);
    }
    casos++;
    abrirSubview(m,w,h,s);arrastar(15,h*.5f,150,0);assert(menusTeste==1 && !teclasTeste);casos++;
    /* Rotation and UI-size changes clamp retained page and revoke old targets. */
    exPagina=toquePagina.maximo;toquePagina.livre=1;
    nv_layout_w=h;nv_layout_h=w;ponteiro_teste_janela((int)h,(int)w);uiScale=u?1:1.5f;quadro();
    assert(exPagina>=0 && exPagina<=toquePagina.maximo);confereAlvos();casos++;
    cancelarTituloSubstituido(m,w,h,s);casos++;
  }
  abrirSubview(MODO_VIZ,1080,2340,1);assert(tocar(ponteiroViz,-1,0));
  assert(!strcmp(detalhePedido,viz.foco.imdb));casos++;
  uiScale=1.5f;nv_layout_w=1920;nv_layout_h=1080;assert(!telefoneui_ativo());
  assert(vzX()==524 && EX_VZ_PW==84 && EX_VZ_CARD==246 && EX_CB_W==140);casos++;
  for(int m=MODO_CLIMA;m<=MODO_VIZ;m++)for(int i=0;i<4;i++){voltarPublicacao(m,i);casos++;}
  for(int m=MODO_CLIMAS;m<=MODO_VIZ;m++)for(int camada=0;camada<4;camada++){preservarCamada(m,camada);casos++;}
  for(int m=MODO_CLIMA;m<=MODO_VIZ;m++)for(int eixo=0;eixo<2;eixo++){preservarArrastoExplore(m,eixo);casos++;}
  for(int t=0;t<4;t++)for(int u=0;u<2;u++){cancelarTituloSubstituido(MODO_CLIMAS,telas[t][0],telas[t][1],u?1.5f:1);casos++;}
  printf("explorar_subviews_touch: %d actual draw/update/pointer/axis/tap/cancel/Back/rotation/TV cases, %d violations\n",casos,failures);
  return failures?1:0;
}
