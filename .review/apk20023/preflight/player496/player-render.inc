/* Actual production renderer with typed recording GPU/text/media boundaries.
 * Text has deterministic conservative metrics; this does not claim raster/font fidelity. */
static void seed(void);
static void key(SDL_Keycode code);
static float fixtureScale=1.f;
static PonteiroAlvo targets[128];static int nTargets;
static int fixtureTextDraws;static GfxRect fixtureTextRects[128];
float gfx_escala_ui(void){return fixtureScale;}
float gfx_escala(void){return fixtureScale;}
#ifndef HAVE_REAL_POINTER
int ponteiro_ativo(void){return 1;}
int ponteiro_tem_toque(void){return 1;}
int ponteiro_toque(void){return 1;}
void ponteiro_alvo(float x,float y,float w,float h,PonteiroFn f,PonteiroFn a,int u,int v){assert(nTargets<128);targets[nTargets++]=(PonteiroAlvo){.x=x,.y=y,.w=w,.h=h,.focar=f,.ativar=a,.a=u,.b=v};}
void ponteiro_alvo_arrastavel(void){}
#else
float gfx_opacidade_grupo=1.f;
#endif
int ajustes_seekr_ligado(void){return 0;}
double video_buffer_fim(void){return 180.;}
void plrui_barra(float x,float y,float w,float frac,float buf,int foco,const float *caps,int n,float a){(void)x;(void)y;(void)w;(void)frac;(void)buf;(void)foco;(void)caps;(void)n;(void)a;}
void plrui_pilula_foco(GfxRect r,float a){(void)r;(void)a;}
void plrui_disco_osd(GfxRect r,float a){(void)r;(void)a;}
int plrui_tinta(void){return 0;}
void plrui_tempo(char *s,size_t n,double t){snprintf(s,n,"%d:%02d",(int)t/60,(int)t%60);}
int txt_largura(TxtEstilo e,const char *s){(void)e;return (int)strlen(s)*12;}
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a){(void)r;(void)g;(void)b;(void)a;return (TxtLinha){.w=txt_largura(e,s),.h=e==TXT_G22M?22:21};}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w){TxtLinha l=txt_linha(e,s,r,g,b,a);if(l.w>w)l.w=(int)w;return l;}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a){if(a>.3&&fixtureTextDraws<128)fixtureTextRects[fixtureTextDraws++]=(GfxRect){x,y,l.w,l.h};}
void gfx_icone(GfxRect r,const char *s,float a,float b,float c,float d){(void)r;(void)s;(void)a;(void)b;(void)c;(void)d;}
void gfx_recorte(float x,float y,float w,float h){(void)x;(void)y;(void)w;(void)h;}
void gfx_sem_recorte(void){}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float x,float y,float rd,float cr,float cg,float cb,float ca){(void)r;(void)t;(void)m;(void)f;(void)x;(void)y;(void)rd;(void)cr;(void)cg;(void)cb;(void)ca;}
void gfx_cor(GfxRect r,float rd,float cr,float cg,float cb,float ca){(void)r;(void)rd;(void)cr;(void)cg;(void)cb;(void)ca;}
void plrilha_relogio(float a,double f){(void)a;(void)f;}
int plrilha_direita(void){return 1;}
int video_largura(void){return 0;}
int video_altura(void){return 0;}
int video_tem_dolby_vision(void){return 0;}
int video_tem_atmos(void){return 0;}
const char *video_hdr(void){return "none";}
int video_dts_estado(void){return VIDEO_DTS_NENHUM;}
int video_audio_atual(void){return -1;}
const VideoFaixa *video_audio(int i){(void)i;return NULL;}
int cacheboost_ganho_estado(void){return 0;}
#ifdef HAVE_REAL_POINTER
static void real_tap(PonteiroAlvo p){
  SDL_Event e={0};e.type=SDL_FINGERDOWN;e.tfinger.touchId=13;e.tfinger.fingerId=17;
  e.tfinger.x=(p.x+p.w*.5f)*fixtureScale/NV_LAYOUT_REAL_W;e.tfinger.y=(p.y+p.h*.5f)*fixtureScale/NV_LAYOUT_REAL_H;
  assert(ponteiro_evento(&e,player_evento));fixtureClock+=30;e.type=SDL_FINGERUP;assert(ponteiro_evento(&e,player_evento));
}
#endif
static void render_matrix(void){
  const float scales[]={1,1.25f,1.5f};int count=0;
  for(int mobile=0;mobile<2;mobile++)for(int portrait=0;portrait<3;portrait++)for(int sc=0;sc<3;sc++)for(int kind=0;kind<3;kind++)for(int focus=0;focus<PLR_NBTNS;focus++){
#ifndef NV_TOUCH_UI
    if(mobile)continue;
#endif
    if(!mobile&&portrait)continue;int caseW=portrait==1?1080:portrait==2?1320:2400,caseH=portrait==1?2400:1080;
    seed();
    if(kind){epT=epE=0;snprintf(fixtureItem.tipo,sizeof fixtureItem.tipo,"movie");fixtureExtras=kind==2;}
    if(!transporteHabilitado(focus))continue;
#ifdef HAVE_REAL_POINTER
    ponteiro_teste_toque(1);ponteiro_teste_janela(caseW,caseH);ponteiro_quadro(fixtureClock);
#endif
    layout_modo_definir(mobile);layout_tela_definir(caseW,caseH);fixtureScale=scales[sc];
    entrada=anim=cheio=fileira=1;duracaoSeg=2400;posSeg=posVis=120;posVisSolto=0;linhaEp[0]=0;
    for(int j=0;j<PLR_NBTNS;j++)focoB[j]=j==focus?1.f:0.f;botao=focus;nTargets=fixtureTextDraws=0;
    desenharOsdCorpo(fixtureClock,1,1,NULL);
#ifdef HAVE_REAL_POINTER
    ponteiro_desenhar();const PonteiroAlvo *actual=NULL;nTargets=ponteiro_teste_lista(&actual);assert(nTargets<128);
    for(int j=0;j<nTargets;j++){targets[j]=actual[j];targets[j].x/=fixtureScale;targets[j].y/=fixtureScale;targets[j].w/=fixtureScale;targets[j].h/=fixtureScale;}
#endif
    int buttons=0;float right=0;
    for(int j=0;j<nTargets;j++)if(targets[j].ativar==ponteiroAtivarBotao||targets[j].ativar==ponteiroBotaoInativo){
      PonteiroAlvo *p=&targets[j];assert(p->x>=0&&p->y>=0&&p->x+p->w<=NV_VTELA_W+.01&&p->y+p->h<=NV_VTELA_H+.01);assert(p->w>=44&&p->h>=44);
      if(mobile){float dp=fixtureScale*360.f/1080.f;assert(p->w*dp>=47.99f&&p->h*dp>=47.99f);}if(p->x+p->w>right)right=p->x+p->w;
      for(int k=0;k<j;k++)if(targets[k].ativar==ponteiroAtivarBotao||targets[k].ativar==ponteiroBotaoInativo){PonteiroAlvo q=targets[k];assert(p->x>=q.x+q.w-.01||q.x>=p->x+p->w-.01||p->y>=q.y+q.h-.01||q.y>=p->y+p->h-.01);}
      buttons++;
    }
    assert(buttons==(kind==0?9:kind==1?6:7));
    for(int j=0;j<fixtureTextDraws;j++){GfxRect r=fixtureTextRects[j];assert(r.x>=0&&r.y>=0&&r.x+r.w<=NV_VTELA_W+.01&&r.y+r.h<=NV_VTELA_H+.01);}
    #ifdef HAVE_REAL_POINTER
    if(mobile&&focus==0&&kind==0){
      if(portrait){botao=PLR_PLAY;key(SDLK_DOWN);assert(botao==PLR_AUDIO&&!barraFoco);key(SDLK_UP);assert(botao==PLR_PLAY&&!barraFoco);}
      PonteiroAlvo next={0},previous={0},reload={0};for(int j=0;j<nTargets;j++){PonteiroAlvo p=targets[j];if(p.ativar==ponteiroAtivarBotao){if(p.a==PLR_PROXIMO)next=p;if(p.a==PLR_ANTERIOR)previous=p;if(p.a==PLR_RECARREGAR)reload=p;}}
      assert(next.w>0&&previous.w>0&&reload.w>0);fixtureClock+=500;real_tap(next);int t,e;assert(player_pediu_proximo(&t,&e)&&t==1&&e==3);player_episodio_pedido_liberar();
      fixtureClock+=500;real_tap(previous);assert(player_pediu_proximo(&t,&e)&&t==1&&e==1);player_episodio_pedido_liberar();
      fixtureModal=1;fixtureClock+=500;real_tap(next);assert(!player_transporte_pedido_pendente());fixtureModal=0;
      fixtureClock+=500;real_tap(reload);assert(player_pediu_recarregar()&&!player_pediu_recarregar());
      for(int endpoint=0;endpoint<2;endpoint++){
        epE=endpoint?3:1;trocaEpisodio=0;ponteiro_quadro(fixtureClock);desenharOsdCorpo(fixtureClock,1,1,NULL);ponteiro_desenhar();
        const PonteiroAlvo *ends=NULL;int total=ponteiro_teste_lista(&ends),found=0;
        for(int k=0;k<total;k++)if(ends[k].a==(endpoint?PLR_PROXIMO:PLR_ANTERIOR)&&ends[k].ativar==ponteiroBotaoInativo){
          PonteiroAlvo p=ends[k];p.x/=fixtureScale;p.y/=fixtureScale;p.w/=fixtureScale;p.h/=fixtureScale;fixtureClock+=500;real_tap(p);assert(!player_transporte_pedido_pendente());found++;
        }assert(found==1);
      }
    }
#endif
    printf("GEOMETRY kind=%d mobile=%d portrait=%d scale=%.2f focus=%d buttons=%d width=%.2f right=%.2f touch_height_dp=%.2f\n",kind,mobile,portrait,fixtureScale,focus,buttons,NV_VTELA_W,right,mobile?transporteCelulaH()*fixtureScale*414.f/1080.f:0.f);count++;
  }
  printf("PASS %d actual rendered transport layouts, every focus at three UI scales\n",count);
}
double vel_tempo_real(double t,int v){return v>0?t*100./v:t;}
float gfx_escala_entrar(void){return fixtureScale;}
void gfx_escala_sair(float e){(void)e;}
