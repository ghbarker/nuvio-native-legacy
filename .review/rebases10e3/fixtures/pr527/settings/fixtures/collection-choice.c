#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include SETTINGS_SOURCE
#include <assert.h>
#include <sys/stat.h>

static float scale=1,requestedUI=1; float gfx_opacidade_grupo=1;
static int auditClipOn;static GfxRect auditClip;
static GfxRect auditControl[2];static int auditSeen[2],auditClipFailures,auditOcclusions;
static void auditFrameBegin(void){memset(auditSeen,0,sizeof auditSeen);auditClipOn=0;}
static int contains(GfxRect a,GfxRect b){return b.x>=a.x-1&&b.y>=a.y-1&&b.x+b.w<=a.x+a.w+1&&b.y+b.h<=a.y+a.h+1;}
static GfxRect scaled(GfxRect r){return (GfxRect){r.x*scale,r.y*scale,r.w*scale,r.h*scale};}
static void auditOpaque(GfxRect r,float a){r=scaled(r);if(a*gfx_opacidade_grupo<.99f)return;for(int i=0;i<2;i++)if(auditSeen[i]&&contains(r,auditControl[i]))auditOcclusions++;}
static void auditText(TxtLinha t,float x,float y,float a){if(t.tex<9001||t.tex>9002)return;int i=(int)t.tex-9001;GfxRect r=scaled((GfxRect){x,y,t.w,t.h});auditSeen[i]=1;auditControl[i]=r;if(auditClipOn&&!contains(auditClip,r))auditClipFailures++;assert(a>0);}

float gfx_escala(void){return scale;} void gfx_escala_sair(float s){scale=s;}
float gfx_escala_ui(void){return requestedUI;}
void gfx_cor(GfxRect r,float z,float a,float b,float c,float d){auditOpaque(r,d);(void)z;(void)a;(void)b;(void)c;}
void gfx_anel(GfxRect r,float z,float w,float a,float b,float c,float d){(void)r;(void)z;(void)w;(void)a;(void)b;(void)c;(void)d;}
void gfx_recorte(float x,float y,float w,float h){auditClipOn=1;auditClip=scaled((GfxRect){x,y,w,h});}
void gfx_sem_recorte(void){auditClipOn=0;}
int txt_largura(TxtEstilo e,const char*s){(void)e;return (int)strlen(s)*13;}
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)r;(void)g;(void)b;(void)a;return (TxtLinha){.tex=!strcmp(s,"Confirmar")?9001:!strcmp(s,"Cancelar")?9002:0,.w=txt_largura(e,s),.h=30};}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float max){TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>max)t.w=max;return t;}
void txt_desenhar(TxtLinha t,float x,float y){auditText(t,x,y,1);}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a){auditText(t,x,y,a);}
const char*i18n(const char*s){return s;}
int gpun_efeitos_automaticos(void){return 0;}
float txt_bloco(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float leading,float a,int max){(void)r;(void)g;(void)b;(void)x;(void)y;(void)a;int n=(int)ceilf(txt_largura(e,s)/w);if(n>max&&max>0)n=max;return n*leading;}
float txt_bloco_corta(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float w,float leading,float a,int max){return txt_bloco(e,s,r,g,b,x,y,w,leading,a,max);}
void corviva_acento(float*r,float*g,float*b){if(r)*r=1;if(g)*g=1;if(b)*b=1;}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float px,float py,float ra,float a,float b,float c,float d){(void)r;(void)t;(void)m;(void)f;(void)px;(void)py;(void)ra;(void)a;(void)b;(void)c;(void)d;}
CorvivaTextura nv_textura_viva;
void corviva_tokens(const float fill[3],CorvivaTokens*t){(void)fill;memset(t,0,sizeof *t);t->tintaBranca=0;}
float txt_tracking(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float a,float tracking){(void)r;(void)g;(void)b;(void)a;(void)x;(void)y;return txt_largura(e,s)+tracking*strlen(s);}
void gfx_icone(GfxRect r,const char*n,float a,float b,float c,float d){(void)r;(void)n;(void)a;(void)b;(void)c;(void)d;}
static PosterProvCfg posterCfg;
const PosterProvCfg*posterprov_cfg(void){return &posterCfg;}
void posterprov_configurar(const PosterProvCfg*c){posterCfg=*c;}
void sync_proteger_ajustes_locais(void){}
void ajlog_mudou(const char*k,int old,int n,AjlogOrigem o,unsigned t){(void)k;(void)old;(void)n;(void)o;(void)t;}
void gfx_sombra_sob(GfxRect s,float f,float px,float ra,float a,float b,float c,float d,GfxRect p,float rp,float ap){(void)s;(void)f;(void)px;(void)ra;(void)a;(void)b;(void)c;(void)d;(void)p;(void)rp;(void)ap;}
float marca_formato_largura(FormatoMarca f,float h){(void)f;return h*1.5f;}
float marca_formato(FormatoMarca f,float x,float y,float h,float r,float g,float b,float a){(void)x;(void)y;(void)r;(void)g;(void)b;(void)a;return marca_formato_largura(f,h);}
static int mdblistKey;
int extras_mdblist_tem_chave(void){return mdblistKey;}
int trailerfonte_tizen(void){return 0;}
int trailerfonte_com_som(int t){return !t;}
int selospacote_ativo(void){return -1;}
int selospacote_n(void){return 0;}
int apoiador_ativo(void){return 1;}
int recomenda_ativo(void){return 1;}
int addons_n(void){return 1;}
int addons_ativo(int i){return i==0;}
const char*addons_base(int i){(void)i;return "https://v3-cinemeta.strem.io";}
const char*addons_nome(int i){(void)i;return "Cinemeta";}
const char*xtream_servidor_curto(void){return "";}
int apoio_n(void){return 0;}
int tex_teto_ram_mb(void){return 512;}
void tex_estatisticas(int*i,int*p,long*b,int*d,long*q){if(i)*i=0;if(p)*p=0;if(b)*b=0;if(d)*d=0;if(q)*q=0;}
int iconeapp_desenhar(int i,GfxRect r,int l,float a){(void)i;(void)r;(void)l;(void)a;return 0;}
int logoapp_miniatura(int i,GfxRect r,int l,float a){(void)i;(void)r;(void)l;(void)a;return 0;}
static int engineAvailable;
int p2pmotor_disponivel(void){return engineAvailable;}
int plugins_disponivel(void){return 1;}
int perfis_n(void){return 1;}
const char*xtream_usuario(void){return "";}
long tex_orcamento_bytes(void){return 128L*1024*1024;}
int cat_n(void){return 0;}
const char*atualizacao_nova(void){return "";}
void gfx_luz_canto(GfxRect r,float ra,float cx,float cy,float al,float cr,float cg,float cb,float ca){(void)r;(void)ra;(void)cx;(void)cy;(void)al;(void)cr;(void)cg;(void)cb;(void)ca;}
void txt_definir_fonte_interface(TxtFamilia f){(void)f;}
int plugins_ligado(void){return 1;}
static ContaPerfil dummyProfile={.indice=1,.nome="Fixture",.corHex="#ffffff",.primario=1};
const ContaPerfil*perfis_item(int i){return i==0?&dummyProfile:NULL;}
const char*xtream_senha_mascarada(void){return "";}
void gfx_anel_fora(GfxRect r,float ra,float fo,float es,float a,float b,float c,float d){(void)r;(void)ra;(void)fo;(void)es;(void)a;(void)b;(void)c;(void)d;}
void gfx_veu_css(GfxRect r,int b,float c,float f,float a){(void)r;(void)b;(void)c;(void)f;(void)a;}
static int searchable=1;
int recomenda_pesquisavel(void){return searchable;}
int pessoas_definir_pesquisavel(int v){searchable=v;return 1;}
int atualizacao_busca_achou(void){return 0;}
int atualizacao_aberta(void){return 0;}
int pessoas_aberto(void){return 0;}
int teclado_aberto(void){return 0;}
int teclado_resultado(void){return TECLADO_NADA;}
int registro_envio_aberto(void){return 0;}
TraEstado traktauth_estado(void){return TRA_PARADO;}
SmkEstado simklauth_estado(void){return SMK_PARADO;}
DisEstado discord_estado(void){return DIS_PARADO;}
void gfx_transformar(float x,float y,float s,float dx,float dy){(void)x;(void)y;(void)s;(void)dx;(void)dy;}
void gfx_sem_transformar(void){}
void fundo_desenhar_modo(int mode,GfxRect area,float r,const char*u,float a){(void)mode;(void)area;(void)r;(void)u;(void)a;}
int plugins_n_repos(void){return 0;}
int perfis_ativo(void){return 1;}
int xtream_configurado(void){return 0;}
int atualizacao_busca(void){return 0;}
void gfx_mini_liberar(GfxMini*m){memset(m,0,sizeof *m);}
int stalker_configurado(void){return 0;}
int jellyfin_disponivel(void){return 1;}
int plex_disponivel(void){return 1;}
int gpun_nivel(void){return 0;}
int perfis_ativo_addons(void){return 1;}
void ajlog_vazar_tudo(void){}
void arte_fonte_chave_fanart(const char*k){(void)k;}
void seekr_definir_chave(const char*k){(void)k;}
void seekr_chave_propria(int p){(void)p;}
void android_interface_modo(int m){(void)m;}
int android_interface_mobile_auto(void){return 1;}
void gfx_escala_ui_definir(float s){(void)s;}
void artehero_fanart_disponivel(int sim){(void)sim;}
const char *apoio_nome (int a0) {return "";}
int apoio_qual (int a0) {return 0;}
int avisos_envio_info (AvisosEnvio * a0) {return 0;}
const CatItem *cat_item (int a0) {static CatItem value; return &value;}
int corviva_paleta (const char * a0, CorvivaPaleta * a1) {return 0;}
const char *debrid_chave_mascarada (const char * a0, char * a1, unsigned int a2) {return "";}
int debrid_origem (const char * a0) {return 0;}
void gfx_vidro_aro (GfxRect a0, float a1, float a2, float a3, float a4, float a5, float a6) {}
void gfx_vidro_fosco (GfxRect a0, float a1, float a2) {}
void gfx_vidro_miolo (GfxRect a0, float a1, float a2, float a3, float a4, float a5, float a6) {}
float gfx_vidro_opacidade (void) {return 0;}
int home_item_focado (HomeItem * a0) {return 0;}
unsigned int p2pmotor_teto_mb (void) {return 512;}
int recomenda_perfil (RecPerfil * a0) {return 0;}
int registro_ja_aberto (void) {return 0;}
int seekr_estado (void) {return 0;}
const char *seekr_estado_rotulo (int a0) {return "";}
int seekr_horario_local (long long int a0, char * a1, size_t a2) {return 0;}
void seekr_uso (SeekrUso * a0) {}
int sessao_logada (void) {return 1;}
SyncEstado sync_estado (void) {return 0;}
const char *sync_resumo (void) {return "";}
int sync_servidor_fora (void) {return 0;}
int xtream_conta (XtreamConta * a0) {return 0;}
int xtream_conta_aviso (const XtreamConta * a0, long long int a1) {return 0;}
unsigned int cat_revisao (void) {return 0;}
void gfx_brilho_topo (GfxRect a0, float a1, float a2, float a3, float a4, float a5, float a6) {}
void gfx_veu_base (GfxRect a0, float a1, float a2, float a3) {}
int logoapp_atual (void) {return 0;}
const char *logoapp_caminho (int a0, int a1) {return "";}
void novidades170_cena_desenhar (int a0, float a1, float a2, float a3) {}
float novidades170_previa_altura (void) {return 0;}
void novidades180_cena_desenhar (int a0, float a1) {}
void registro_envio_desenhar (void) {}
int simklauth_restante_s (void) {return 0;}
const char *simklauth_token (void) {return "";}
const char *simklauth_url (void) {return "";}
int spot_visivel (void) {return 0;}
float tex_aspecto (const char * a0) {return 0;}
long int tex_cache_disco_bytes (void) {return 0;}
int tex_falhou (const char * a0) {return 0;}
int tex_historico (long int * a0, int a1) {return 0;}
GLuint tex_obter_larg (const char * a0, float a1) {return 0;}
void tex_orcamento_info (int * a0, long int * a1, int * a2, int * a3) {}
const char *traktauth_url (void) {return "";}
void teclado_desenhar (Uint32 a0) {}
int discord_disponivel(void){return 1;}
void desc_refazer_continuar(void){}
void desc_repetir(void){}
void seguro_mudou(const char*k,int a,int n,long t,unsigned u){(void)k;(void)a;(void)n;(void)t;(void)u;}
void seguro_ajustou(const char*k,int n,int r){(void)k;(void)n;(void)r;}
static int seenWarnings;
int seguro_aviso_visto(int b){return (seenWarnings&b)==b;}
void seguro_aviso_marcar(int b){seenWarnings|=b;}
void tex_definir_orcamento_mb(int m){(void)m;}
void sync_iniciar(void){}
JfEstado jellyfin_estado(char*d,size_t n){if(d&&n)*d=0;return JF_EST_SEM_SERVIDOR;}
void enquete_definir_optout(int v){(void)v;}
void iconeapp_aplicar_plataforma(void){}
void addons_legendas_reiniciar(void){}
const char*jellyfin_servidor_curto(void){return "";}
JfEstado emby_estado(char*d,size_t n){if(d&&n)*d=0;return JF_EST_SEM_SERVIDOR;}
PxEstado plex_estado(char*d,size_t n){if(d&&n)*d=0;return PX_EST_SEM_CONTA;}
const char*emby_servidor_curto(void){return "";}
const char*plex_servidor_nome(void){return "";}
static Uint32 tick=1000;
Uint32 SDL_GetTicks(void){return tick;}
int SDL_GetNumTouchDevices(void){return 1;}
SDL_Window*SDL_GL_GetCurrentWindow(void){return NULL;}
SDL_Window*SDL_GetWindowFromID(Uint32 id){(void)id;return NULL;}
SDL_Window*SDL_GetMouseFocus(void){return NULL;}
Uint32 SDL_GetMouseState(int*x,int*y){if(x)*x=0;if(y)*y=0;return 0;}
void SDL_GetWindowSize(SDL_Window*w,int*x,int*y){(void)w;*x=1080;*y=2340;}
void SDL_GL_GetDrawableSize(SDL_Window*w,int*x,int*y){SDL_GetWindowSize(w,x,y);}
int SDL_ShowCursor(int q){(void)q;return 0;}
static void unexpectedEvent(const SDL_Event*e){fprintf(stderr,"unexpected fallback event %u\n",e->type);abort();}
static void frame(void){auditFrameBegin();ponteiro_teste_janela((int)NV_LAYOUT_REAL_W,(int)NV_LAYOUT_REAL_H);ponteiro_quadro(tick);ajustes_atualizar(.05f,tick);ajustes_desenhar(tick);ponteiro_desenhar();tick+=50;}
static void settle(void){for(int i=0;i<80;i++)frame();}
static int find(PonteiroFn fn,int a,float*x,float*y){const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);for(int i=0;i<n;i++)if(v[i].ativar==fn&&v[i].a==a){*x=v[i].x+v[i].w/2;*y=v[i].y+v[i].h/2;assert(ponteiro_achar(v,n,*x,*y)==i);return 1;}return 0;}
static void tap(float x,float y){SDL_Event e;SDL_zero(e);e.type=SDL_FINGERDOWN;e.tfinger.touchId=1;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y/NV_LAYOUT_REAL_H;ponteiro_evento(&e,ajustes_evento);tick+=60;e.type=SDL_FINGERUP;ponteiro_evento(&e,ajustes_evento);tick+=400;}
static int findFocus(PonteiroFn fn,int a,float*x,float*y){const PonteiroAlvo*v;int n=ponteiro_teste_lista(&v);for(int i=0;i<n;i++)if(v[i].focar==fn&&v[i].a==a){*x=v[i].x+v[i].w/2;*y=v[i].y+v[i].h/2;assert(ponteiro_achar(v,n,*x,*y)==i);return 1;}return 0;}
static int findChoice(int v,float*x,float*y){return find(ajToqueInlineValor,v,x,y)||findFocus(ajPontEdValor,v,x,y);}
static void rowTap(void){float x,y;for(int z=0;z<120;z++){frame();if(findFocus(ajPonteiroItem,focoItem,&x,&y)){tap(x,y);return;}}fprintf(stderr,"row unreachable %d\n",focoOp);abort();}
static int actualSwipes;
static void swipeEditor(int down){actualSwipes++;
 AjToqueRegiao*r=NULL;for(int i=0;i<ajToqueN;i++)if(ajToqueRegioes[i].id==AJT_EDITOR)r=&ajToqueRegioes[i];assert(r);
 float x=r->r.x+r->r.w/2,y0=r->r.y+r->r.h*(down?.2f:.8f),y1=r->r.y+r->r.h*(down?.8f:.2f);SDL_Event e;SDL_zero(e);e.type=SDL_FINGERDOWN;e.tfinger.touchId=1;e.tfinger.fingerId=1;e.tfinger.x=x/NV_LAYOUT_REAL_W;e.tfinger.y=y0/NV_LAYOUT_REAL_H;ponteiro_evento(&e,ajustes_evento);
 for(int i=1;i<=20;i++){tick+=20;e.type=SDL_FINGERMOTION;e.tfinger.y=(y0+(y1-y0)*i/20)/NV_LAYOUT_REAL_H;ponteiro_evento(&e,ajustes_evento);frame();}
 e.type=SDL_FINGERUP;ponteiro_evento(&e,ajustes_evento);settle();
}
static void choiceTap(int value){float x,y;for(int z=0;z<80;z++){frame();if(findChoice(value,&x,&y)){tap(x,y);return;}if(!ajPontInline)swipeEditor(value<uxPendente);}fprintf(stderr,"choice unreachable op%d value%d inline%d editor%d\n",uxOp,value,ajPontInline,uxEditor);abort();}
static void commit(void){float x,y;settle();assert(auditSeen[0]&&!auditClipFailures&&!auditOcclusions);assert(find(ajToqueConfirmar,0,&x,&y));tap(x,y);if(uxAvisoRisco){settle();assert(findFocus(ajPontConfirmar,1,&x,&y));tap(x,y);}assert(!uxEditor);}
static void cancel(void){float x,y;settle();assert(auditSeen[1]&&!auditClipFailures&&!auditOcclusions);assert(find(ajToqueCancelar,0,&x,&y));tap(x,y);assert(!uxEditor);}
static void reloaded(int op,int expected,const char*tmp){
 int saved=uxValorAtual(op);assert(saved==expected);if(CHAVE[op]&&CHAVE[op][0]!='-')valor[op]=expected==0?1:0;ajustes_encerrar();ajustes_dir(tmp);assert(ajustes_iniciar());settle();assert(uxValorAtual(op)==expected);
 printf("PASS persisted-reloaded %d value%d\n",op,expected);
}

static int checks;static void expect(int ok,const char*why){checks++;if(!ok){fprintf(stderr,"FAIL %s pending%d saved%d editor%d inline%d\n",why,uxPendente,ajustes_colecao_exibicao(),uxEditor,ajPontInline);abort();}}
static void key(SDL_Keycode k){SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;ajustes_evento(&e);}
static void openCollection(void){focarOpcao(AJ_COLECAO_EXIBICAO);settle();expect(focoOp==AJ_COLECAO_EXIBICAO,"new collection setting can be focused in real Home screen category");rowTap();settle();expect(uxEditor==1&&uxOp==AJ_COLECAO_EXIBICAO,"real rendered row tap opens exact new setting");}
static char*disk(void){char*p=dados_ler("ajustes.txt");expect(p!=NULL,"real settings file exists");return p;}
static void assertButtons(void){float x,y;settle();expect(auditSeen[0]&&auditSeen[1],"visible Confirm and Cancel text drawn");expect(!auditClipFailures&&!auditOcclusions,"Confirm/Cancel text not clipped or occluded");expect(find(ajToqueConfirmar,0,&x,&y)&&find(ajToqueCancelar,0,&x,&y),"real pointer has separate visible Confirm and Cancel controls");}
int main(int argc,char**argv){assert(argc==5);setbuf(stdout,NULL);int portrait=atoi(argv[1]),settingsSize=atoi(argv[2]);requestedUI=atof(argv[3]);int mobile=atoi(argv[4]);
 char tmp[]="/tmp/nuvio-362-choice-XXXXXX";assert(mkdtemp(tmp));setenv("NUVIO_DADOS",tmp,1);dados_iniciar(tmp);snprintf(dirAjustes,sizeof dirAjustes,"%s",tmp);fontefalha_dir(tmp);
 dados_gravar("aparencia-20.txt","1\n");dados_gravar("profundidade-205.txt","1\n");dados_gravar("perfilfundo-20.txt","1\n");dados_gravar("envio-151.txt","1\n");dados_gravar("ajustesescala-203.txt","1\n");dados_gravar(RES_ARQ_MIGRADA,"1\n");
 valor[AJ_INTERFACE_MODO]=mobile;valor[AJ_AVANCADAS]=0;valor[AJ_TAMANHO_AJUSTES]=settingsSize;valor[AJ_TAMANHO_UI]=requestedUI>1.4?3:requestedUI>1.25?2:requestedUI>1.1?1:0;valor[AJ_ANIM]=0;
 layout_modo_definir(mobile);layout_tela_definir(portrait?1080:2340,portrait?2340:1080);ponteiro_iniciar();ponteiro_teste_toque(1);expect(ajustes_iniciar(),"actual Settings initialization");expect(gravar(),"actual initial settings save");
 expect(nValores(AJ_COLECAO_EXIBICAO)==8,"all eight existing collection modes available");const char*labels[]={"Automático","Grade","Linha do tempo","Ranking","Saga","Décadas","Mural","Vitrine"};for(int i=0;i<8;i++)expect(!strcmp(V_COLECAO_EXIBICAO[i],labels[i]),"existing view ID and label order preserved");
 // Keyboard highlighting uses pending state; visible Cancel must leave real saved bytes identical.
 for(int v=0;v<8;v++){int before=ajustes_colecao_exibicao();openCollection();assertButtons();char*old=disk();for(int tries=0;uxPendente!=v&&tries<16;tries++)key(uxPendente<v?SDLK_DOWN:SDLK_UP);expect(uxEditor==1&&uxPendente==v&&ajustes_colecao_exibicao()==before,"keyboard highlighting waits for explicit Confirm");cancel();expect(ajustes_colecao_exibicao()==before,"visible Cancel preserves saved view");char*now=disk();expect(!strcmp(old,now),"Cancel leaves actual saved bytes unchanged");free(old);free(now);}
 // The two far endpoints must be physically reachable through the actual editor scroll region when clipped.
 for(int endpoint=7;endpoint>=0;endpoint-=7){openCollection();float x,y;int initiallyVisible=findChoice(endpoint,&x,&y),swipes=actualSwipes;choiceTap(endpoint);expect(!uxEditor&&ajustes_colecao_exibicao()==endpoint,"far endpoint reached by actual tap/scroll");expect(initiallyVisible||actualSwipes>swipes,"clipped far endpoint requires real finger scroll");}
 // Non-inline option taps retain the existing immediate-apply behavior; all eight choices must scroll into reach.
 for(int v=0;v<8;v++){openCollection();choiceTap(v);expect(!uxEditor&&ajustes_colecao_exibicao()==v,"actual option tap immediately saves as existing choice-panel behavior");valor[AJ_COLECAO_EXIBICAO]=(v+1)%8;ajustes_encerrar();ajustes_dir(tmp);expect(ajustes_iniciar(),"actual Settings reinitialize after direct tap");expect(ajustes_colecao_exibicao()==v,"all eight tapped values survive disk reload");}
 // Keyboard pending selection also saves through the visible Confirm, then reloads and reopens.
 for(int v=0;v<8;v++){openCollection();for(int tries=0;uxPendente!=v&&tries<16;tries++)key(uxPendente<v?SDLK_DOWN:SDLK_UP);expect(uxPendente==v,"all eight values keyboard-reachable");assertButtons();commit();expect(ajustes_colecao_exibicao()==v,"visible Confirm applies exact chosen view");valor[AJ_COLECAO_EXIBICAO]=(v+1)%8;ajustes_encerrar();ajustes_dir(tmp);expect(ajustes_iniciar(),"actual Settings reinitialize reads real saved state");expect(ajustes_colecao_exibicao()==v,"all eight values survive disk reload");openCollection();expect(uxPendente==v,"reopened choice shows actual saved value");cancel();}
 printf("PASS global8choice checks=%d portrait=%d settingsSize=%d ui=%g mobile=%d actualSwipes=%d\n",checks,portrait,settingsSize,requestedUI,mobile,actualSwipes);return 0;}
