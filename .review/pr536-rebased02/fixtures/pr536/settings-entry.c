#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif
#include SETTINGS_SOURCE
#include <assert.h>
#include <sys/stat.h>

static float scale=1; float gfx_opacidade_grupo=1;
static int auditClipOn;static GfxRect auditClip;
static GfxRect auditControl[2];static int auditSeen[2],auditClipFailures,auditOcclusions;
static void auditFrameBegin(void){memset(auditSeen,0,sizeof auditSeen);auditClipOn=0;}
static int contains(GfxRect a,GfxRect b){return b.x>=a.x-1&&b.y>=a.y-1&&b.x+b.w<=a.x+a.w+1&&b.y+b.h<=a.y+a.h+1;}
static GfxRect scaled(GfxRect r){return (GfxRect){r.x*scale,r.y*scale,r.w*scale,r.h*scale};}
static void auditOpaque(GfxRect r,float a){r=scaled(r);if(a*gfx_opacidade_grupo<.99f)return;for(int i=0;i<2;i++)if(auditSeen[i]&&contains(r,auditControl[i]))auditOcclusions++;}
static void auditText(TxtLinha t,float x,float y,float a){if(t.tex<9001||t.tex>9002)return;int i=(int)t.tex-9001;GfxRect r=scaled((GfxRect){x,y,t.w,t.h});auditSeen[i]=1;auditControl[i]=r;if(auditClipOn&&!contains(auditClip,r))auditClipFailures++;assert(a>0);}

float gfx_escala(void){return scale;} void gfx_escala_sair(float s){scale=s;}
static float ui=1;float gfx_escala_ui(void){return ui;}
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
void teclado_contexto(const char*s){(void)s;}
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

extern void test_player_reload(void);
static void key(SDL_Keycode k){SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;ajustes_evento(&e);tick+=80;}
static char *readfile(const char *name){char p[700];snprintf(p,sizeof p,"%s/%s",dirAjustes,name);FILE*f=fopen(p,"rb");if(!f)return strdup("");assert(!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);char*b=calloc(n+1,1);assert(b);assert(fread(b,1,n,f)==(size_t)n);fclose(f);return b;}
static void samefile(const char*name,const char*before){char*after=readfile(name);assert(!strcmp(before,after));free(after);}
static void openrow(int op){ajustes_abrir_opcao(op);assert(ajustes_iniciar());settle();assert(focoOp==op);key(SDLK_RETURN);settle();}
static void draftstep(int op){key(OPCOES[op].tipo==OP_NUMERO||ajEmLinha(op)?SDLK_RIGHT:SDLK_DOWN);assert(uxPendente!=uxOriginal);settle();}
static void finish(int confirm,int mobile){if(mobile){float x,y;assert(find(confirm?ajToqueConfirmar:ajToqueCancelar,0,&x,&y));tap(x,y);}else key(confirm?SDLK_RETURN:SDLK_ESCAPE);settle();assert(!uxEditor);}
static void reloadsettings(void){char dir[sizeof dirAjustes];snprintf(dir,sizeof dir,"%s",dirAjustes);ajustes_dir(dir);}
static void edit(int op,int mobile){
 int before=uxValorAtual(op);const char*file=ajLegendaPrincipal(op)?"player.txt":"ajustes.txt";char *bytes=readfile(file);
 openrow(op);
 if(ehInterruptor(op)){assert(!uxEditor);assert(uxValorAtual(op)!=before);free(bytes);return;}
 assert(uxEditor);draftstep(op);assert(uxValorAtual(op)==before);samefile(file,bytes);finish(0,mobile);assert(uxValorAtual(op)==before);samefile(file,bytes);
 openrow(op);draftstep(op);int want=uxPendente;finish(1,mobile);assert(uxValorAtual(op)==want);free(bytes);
 if(ajLegendaPrincipal(op)){test_player_reload();assert(uxValorAtual(op)==want);}else{valor[op]=valorPadrao[op];reloadsettings();assert(uxValorAtual(op)==want);}
 if(op==AJ_LEG2_ATRASO)assert(legenda2_offset_manual()==ajustes_leg2_atraso());
}
static void migration(void){
 static const int old[]={0,60,80,100,120,140,160,170,180,190,200,210,220,230,240,250,260,270,280,290,300};
 char p[700];snprintf(p,sizeof p,"%s/ajustes.txt",dirAjustes);
 for(int i=0;i<21;i++){FILE*f=fopen(p,"w");assert(f);fprintf(f,"legenda2TamanhoLocal %d\n",i);fclose(f);valor[AJ_LEG2_TAMANHO]=0;reloadsettings();assert(ajustes_leg2_tamanho()==old[i]);}
 for(int reverse=0;reverse<2;reverse++){FILE*f=fopen(p,"w");assert(f);fputs(reverse?"legenda2EscalaLocal 2\nlegenda2TamanhoLocal 20\n":"legenda2TamanhoLocal 20\nlegenda2EscalaLocal 2\n",f);fclose(f);reloadsettings();assert(ajustes_leg2_tamanho()==60);}
 for(int i=1;i<27;i++){valor[AJ_LEG2_TAMANHO]=i;assert(ajustes_leg2_tamanho()==40+i*10);}
}
int main(int argc,char**argv){
 assert(argc==5);int mobile=atoi(argv[4]);ui=atof(argv[3]);char tmp[]="/tmp/nuvio-subtitle-parity-XXXXXX";assert(mkdtemp(tmp));setenv("NUVIO_DADOS",tmp,1);dados_iniciar(tmp);snprintf(dirAjustes,sizeof dirAjustes,"%s",tmp);player_dir(tmp);fontefalha_dir(tmp);player_leg_estilo_carregar();legenda2_vincular_atraso(ajustes_leg2_atraso(),ajustes_leg2_definir_atraso);
 valor[AJ_INTERFACE_MODO]=mobile;layout_modo_definir(mobile);layout_tela_definir(atoi(argv[1]),atoi(argv[2]));valor[AJ_TAMANHO_AJUSTES]=2;valor[AJ_ANIM]=0;ponteiro_iniciar();ponteiro_teste_toque(1);
 // Preexisting appearance/ASS override is loaded, and merely entering Settings never writes it.
 FILE*f;char path[700];snprintf(path,sizeof path,"%s/player.txt",tmp);f=fopen(path,"w");assert(f);fputs("aspecto 2\nleg_tamanho 120\nleg_cor 0\nleg_cor_borda 0\nleg_tocado 0\n",f);fclose(f);test_player_reload();char*unwritten=readfile("player.txt");assert(ajustes_iniciar());settle();samefile("player.txt",unwritten);free(unwritten);
 int one[]={AJ_LEG1_TAMANHO,AJ_LEG1_FONTE,AJ_LEG1_COR,AJ_LEG1_OPACIDADE,AJ_LEG1_FUNDO,AJ_LEG1_POSICAO,AJ_LEG1_BORDA,AJ_LEG1_COR_BORDA,AJ_LEG1_NEGRITO,AJ_LEG1_ATRASO};
 int two[]={AJ_LEG2_TAMANHO,AJ_LEG2_FONTE,AJ_LEG2_COR,AJ_LEG2_OPACIDADE,AJ_LEG2_FUNDO,AJ_LEG2_POSICAO,AJ_LEG2_BORDA,AJ_LEG2_COR_BORDA,AJ_LEG2_NEGRITO,AJ_LEG2_ATRASO};
 for(int i=0;i<10;i++){edit(one[i],mobile);edit(two[i],mobile);}assert(player_leg_estilo_tocado(PLR_LEG_COR));
 // Saved appearance remains device-local and never becomes profile/account data.
 for(int i=0;i<10;i++){assert(!dePerfil(one[i])&&!dePerfil(two[i]));assert(somenteDesteAparelho(one[i])&&somenteDesteAparelho(two[i]));}
 assert(ajustes_leg2_posicao()==0);assert(definirValorDireto(AJ_LEG2_POS,1));assert(ajustes_leg2_posicao()==-1&&ajustes_leg2_junto());
 openrow(AJ_LEG1_RESTAURAR);assert(!uxEditor);assert(!memcmp(player_leg_estilo(),player_leg_estilo_padrao(),sizeof(VideoLegendaEstilo)));assert(!player_leg_estilo_tocado(PLR_LEG_COR));assert(player_aspecto()==2);
 openrow(AJ_LEG2_RESTAURAR);assert(!uxEditor);for(int i=0;i<10;i++)assert(uxValorAtual(two[i])==valorPadrao[two[i]]);assert(!ajustes_leg2_junto());
 // Inherited fields resolve from current main settings; each new override is independent.
 VideoLegendaEstilo e={200,4,3,6,2,1000,2,TXT_FAMILIA_JETBRAINS,1,5},out;
 ajustes_leg2_estilo(&e,&out);assert(out.tamanho==180&&out.familia==e.familia&&out.opacidade==e.opacidade&&out.corBorda==e.corBorda&&out.negrito==e.negrito&&out.atrasoMs==0);
 assert(definirValorDireto(AJ_LEG2_FONTE,2));assert(definirValorDireto(AJ_LEG2_OPACIDADE,1));assert(definirValorDireto(AJ_LEG2_COR_BORDA,2));assert(definirValorDireto(AJ_LEG2_NEGRITO,2));ajustes_leg2_estilo(&e,&out);assert(out.familia==1&&out.opacidade==0&&out.corBorda==1&&out.negrito==0);assert(!memcmp(&e,&(VideoLegendaEstilo){200,4,3,6,2,1000,2,TXT_FAMILIA_JETBRAINS,1,5},sizeof e));
 // Failed main disk commit rolls back actual style and authored override flags.
 VideoLegendaEstilo before=*player_leg_estilo();player_dir("/nonexistent/nuvio-parity-dir");assert(!ajLegendaDefinir(AJ_LEG1_COR,2));assert(!memcmp(&before,player_leg_estilo(),sizeof before));assert(!player_leg_estilo_tocado(PLR_LEG_COR));player_dir(tmp);
 assert(ajustes_leg2_definir_atraso(500));assert(legenda2_offset_manual()==500);legenda2_definir_offset_manual(900);assert(ajustes_leg2_atraso()==900&&legenda2_offset_manual()==900);legenda2_definir_offset_auto(1000);assert(legenda2_offset_total()==1900);legenda2_reiniciar();assert(legenda2_offset_manual()==900&&legenda2_offset_total()==900);char gooddir[sizeof dirAjustes];snprintf(gooddir,sizeof gooddir,"%s",dirAjustes);snprintf(dirAjustes,sizeof dirAjustes,"/nonexistent/nuvio-settings-dir");assert(!ajustes_leg2_definir_atraso(1200));legenda2_definir_offset_manual(1400);assert(ajustes_leg2_atraso()==900&&legenda2_offset_manual()==900);snprintf(dirAjustes,sizeof dirAjustes,"%s",gooddir);assert(ajustes_leg2_definir_atraso(60000));assert(legenda2_offset_manual()==60000);assert(ajustes_leg2_definir_atraso(-60001));assert(legenda2_offset_manual()==-60000);assert(ajustes_leg2_definir_atraso(0));
 char*aj=readfile("ajustes.txt");assert(!strstr(aj,"legenda1"));free(aj);migration();
 printf("PASS complete Settings parity10+10 controls, cancel/confirm/persist/reload/reset, migration21 and canonical precedence, ASS flags, inheritance, failed-write rollback mobile=%d ui=%.2f\n",mobile,ui);return 0;
}
