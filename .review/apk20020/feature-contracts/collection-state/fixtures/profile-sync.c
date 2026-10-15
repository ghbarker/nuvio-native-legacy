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
float gfx_escala_ui(void){return 1;}
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
int perfis_n(void){return 3;}
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
static int activeProfile=1; int perfis_ativo(void){return activeProfile;}
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

static unsigned contextChanges,cacheClears,historyClears;
void cat_historico_contexto(const char *u,int perfil){assert(u && !strcmp(u,"offline-fixture-account"));assert(perfil==activeProfile);contextChanges++;}
void cachearte_limpar_referencias(void){cacheClears++;}
const char *sessao_usuario(void){return "offline-fixture-account";}
void syncprog_esquecer(void){historyClears++;}
#ifdef PUBLISHED493_DEFAULTS
/* The composed Settings module already contains the real published493 defaults producer. */
#endif
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"CHECK failed line %d: %s\n",__LINE__,#x);abort();}}while(0)
static void current(int p,int rows,int layout){activeProfile=p;valor[AJ_COLECAO_EXIBICAO]=rows;valor[AJ_QUALIDADE]=layout;}
static void moveTo(int target){int old=activeProfile;activeProfile=target;sync_trocar_perfil(old);CHECK(contextChanges==cacheClears);
#ifdef PUBLISHED493_DEFAULTS
CHECK(cacheClears==historyClears);
#else
CHECK(historyClears==0);
#endif
}
static void assertFile(int p,int rows){char name[64],key[100];snprintf(name,sizeof name,"ajustes-p%d.txt",p);char*t=dados_ler(name);CHECK(t!=NULL);snprintf(key,sizeof key,"%s %d\n",CHAVE[AJ_COLECAO_EXIBICAO],rows);CHECK(strstr(t,key)!=NULL);free(t);}
int main(void){
 char tmp[]="/tmp/nuvio362-profile-XXXXXX";CHECK(mkdtemp(tmp)!=NULL);CHECK(!setenv("NUVIO_DADOS",tmp,1));dados_iniciar(tmp);snprintf(dirAjustes,sizeof dirAjustes,"%s",tmp);
 CHECK(dePerfil(AJ_COLECAO_EXIBICAO));CHECK(somenteDesteAparelho(AJ_COLECAO_EXIBICAO));CHECK(valorPadrao[AJ_COLECAO_EXIBICAO]==0);
 CHECK(OPCOES[AJ_COLECAO_EXIBICAO].n==VT_LAY_N);CHECK(VT_LAY_N==8);
 const char *labels[]={"Automático","Grade","Linha do tempo","Ranking","Saga","Décadas","Mural","Vitrine"};
 for(int v=0;v<VT_LAY_N;v++){CHECK(!strcmp(OPCOES[AJ_COLECAO_EXIBICAO].valores[v],labels[v]));valor[AJ_COLECAO_EXIBICAO]=v;CHECK(ajustes_colecao_exibicao()==v);ajustes_perfil_guardar(1);assertFile(1,v);valor[AJ_COLECAO_EXIBICAO]=77;CHECK(ajustes_perfil_restaurar(1));CHECK(ajustes_colecao_exibicao()==v);}
 int badValues[]={-1,8,99};for(unsigned i=0;i<sizeof badValues/sizeof *badValues;i++){valor[AJ_COLECAO_EXIBICAO]=badValues[i];CHECK(ajustes_colecao_exibicao()==VT_LAY_AUTO);}
 current(1,1,2);ajustes_perfil_guardar(1);assertFile(1,1);
 moveTo(2);CHECK(ajustes_colecao_exibicao()==0);
 #ifndef PUBLISHED493_DEFAULTS
 CHECK(valor[AJ_QUALIDADE]==2); /* preserve existing primary inheritance for old unrelated setting */
 #else
 CHECK(valor[AJ_QUALIDADE]==valorPadrao[AJ_QUALIDADE]);
 #endif
 current(2,1,1);moveTo(3);CHECK(ajustes_colecao_exibicao()==0);
 #ifndef PUBLISHED493_DEFAULTS
 CHECK(valor[AJ_QUALIDADE]==2); /* exact formerly failing secondary -> missing third -> primary fallback */
 #else
 CHECK(valor[AJ_QUALIDADE]==valorPadrao[AJ_QUALIDADE]);
 #endif
 assertFile(2,1);moveTo(2);CHECK(ajustes_colecao_exibicao()==1);CHECK(valor[AJ_QUALIDADE]==1);assertFile(3,0);
 moveTo(3);CHECK(ajustes_colecao_exibicao()==0);moveTo(2);CHECK(ajustes_colecao_exibicao()==1);
 char old[256];snprintf(old,sizeof old,"%s 0\n",CHAVE[AJ_QUALIDADE]);CHECK(dados_gravar("ajustes-p3.txt",old));
 moveTo(3);CHECK(ajustes_colecao_exibicao()==0);CHECK(valor[AJ_QUALIDADE]==0);moveTo(2);CHECK(ajustes_colecao_exibicao()==1);assertFile(3,0);
 /* Explicit inactive-primary restore is the legacy fallback route: only this new key stays with active target. */
 current(3,0,0);CHECK(ajustes_perfil_restaurar(1));CHECK(ajustes_colecao_exibicao()==0);CHECK(valor[AJ_QUALIDADE]==2);
 current(3,0,0);moveTo(2);CHECK(ajustes_colecao_exibicao()==1);moveTo(3);CHECK(ajustes_colecao_exibicao()==0);
 /* Missing local key is migration default even when the previous profile chose Rows. */
 CHECK(dados_gravar("ajustes-p3.txt",old));moveTo(2);CHECK(ajustes_colecao_exibicao()==1);moveTo(3);CHECK(ajustes_colecao_exibicao()==0);
 /* A malformed stored value must behave as the new default, never inherit the departing profile. */
 for(unsigned i=0;i<sizeof badValues/sizeof *badValues;i++){
   current(2,1,1);char invalid[256];snprintf(invalid,sizeof invalid,"%s %d\n%s 0\n",CHAVE[AJ_COLECAO_EXIBICAO],badValues[i],CHAVE[AJ_QUALIDADE]);CHECK(dados_gravar("ajustes-p3.txt",invalid));moveTo(3);CHECK(ajustes_colecao_exibicao()==VT_LAY_AUTO);moveTo(2);CHECK(ajustes_colecao_exibicao()==1);
 }
 current(3,1,0);ajustes_perfil_guardar(3);ajustes_perfil_esquecer();CHECK(ajustes_colecao_exibicao()==0);CHECK(dados_ler("ajustes-p1.txt")==NULL);CHECK(dados_ler("ajustes-p2.txt")==NULL);CHECK(dados_ler("ajustes-p3.txt")==NULL);
 printf("PASS actual sync/profile storage: %u assertions, %u profile changes\n",checks,contextChanges);return 0;
}
