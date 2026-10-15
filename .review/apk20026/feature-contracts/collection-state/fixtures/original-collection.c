#include "original-colecao-adapted.inc"
static unsigned checks, writes, loads, allLoads;
static int globalView, profile=1, itemCount=6;
static const ColFolder *lookup;
static CatItem items[6];
static Uint32 clockNow=1000;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"CHECK failed line%d: %s\n",__LINE__,#x);abort();}}while(0)

const ColFolder *col_por_catalogo(const char*b,const char*t,const char*i){(void)b;(void)t;(void)i;return lookup;}
const char*addons_base_por_id(const char*i){(void)i;return "";}
void col_nome_fonte(const ColSource*s,const char*m,char*d,unsigned n){(void)m;snprintf(d,n,"%s",s->title);}
const char*desc_nome_catalogo(const char*b,const char*t,const char*i){(void)b;(void)t;(void)i;return "Catalog";}
const char*desc_chave_tmdb(void){return "";}
const char*nuvem_trakt_cliente(void){return "";}
void desc_vertudo_fonte(const ColSource*s){(void)s;loads++;}
void desc_vertudo_filtro(const char*b,const char*t,const char*i,const char*g){CHECK(b&&*b&&t&&i&&g);loads++;}
void desc_vertudo_abrir(const char*b,const char*t,const char*i){CHECK(b&&*b&&t&&i);allLoads++;}
int desc_vertudo_n(void){return itemCount;}
int desc_vertudo_item(int i,CatItem*d){if(i<0||i>=itemCount)return 0;*d=items[i];return 1;}
int desc_vertudo_carregando(void){return 0;}
int desc_vertudo_fim(void){return 1;}
int desc_vertudo_erro(void){return 0;}
void desc_vertudo_mais(void){}
int cat_visto(const CatItem*c){(void)c;return 0;}
void tex_orcamento_info(int*m,long*mem,int*f,int*s){*m=256;*mem=4096;*f=0;*s=100;}
int ptv_gpu_fraca_atual(void){return 0;}
static float uiScale=1,drawScale=1;float gfx_escala_ui(void){return uiScale;}float gfx_escala(void){return drawScale;}void gfx_escala_sair(float x){drawScale=x;}
void col_cor(const ColFolder*f,float*r,float*g,float*b){(void)f;*r=.2f;*g=.3f;*b=.7f;}
void gfx_recorte(float x,float y,float w,float h){(void)x;(void)y;(void)w;(void)h;}void gfx_sem_recorte(void){}
const char*i18n(const char*s){return s;}
float gfx_opacidade_grupo=1;
void gfx_cor(GfxRect r,float a,float b,float c,float d,float e){(void)r;(void)a;(void)b;(void)c;(void)d;(void)e;}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float f,float x,float y,float ra,float a,float b,float c,float d){(void)r;(void)t;(void)m;(void)f;(void)x;(void)y;(void)ra;(void)a;(void)b;(void)c;(void)d;}
TxtLinha txt_linha(TxtEstilo e,const char*s,int r,int g,int b,int a){(void)e;(void)r;(void)g;(void)b;(void)a;return(TxtLinha){.w=strlen(s)*12,.h=24};}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float w){TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>w)t.w=w;return t;}
void txt_desenhar_alpha(TxtLinha t,float x,float y,float a){(void)t;(void)x;(void)y;(void)a;}
Uint32 SDL_GetTicks(void){return clockNow;}
int SDL_GetNumTouchDevices(void){return 1;}
SDL_Window*SDL_GL_GetCurrentWindow(void){return NULL;}
SDL_Window*SDL_GetWindowFromID(Uint32 id){(void)id;return NULL;}
SDL_Window*SDL_GetMouseFocus(void){return NULL;}
Uint32 SDL_GetMouseState(int*x,int*y){if(x)*x=0;if(y)*y=0;return 0;}
void SDL_GetWindowSize(SDL_Window*w,int*x,int*y){(void)w;*x=1080;*y=2340;}
void SDL_GL_GetDrawableSize(SDL_Window*w,int*x,int*y){SDL_GetWindowSize(w,x,y);}
int SDL_ShowCursor(int q){(void)q;return 0;}
int collection_real_dados_gravar(const char*n,const char*t);
int dados_gravar(const char*n,const char*t){writes++;return collection_real_dados_gravar(n,t);}

int cat_indice_por_imdb(const char*i){(void)i;return -1;}
int cat_acrescentar(const CatItem*i){(void)i;return -1;}

/* Offline global catalog is absent; the seeded raw snapshot is unchanged. */
unsigned cat_revisao_itens(void){return 1;}
unsigned desc_vertudo_revisao(void){return 1;}
int cat_copiar_por_id(const char*i,const char*t,CatItem*d){(void)i;(void)t;(void)d;return 0;}
int desc_vertudo_n_bruto(void){return shot_vt_n();}
int desc_vertudo_item_bruto(int i,CatItem*d){return shot_vt_item(i,d);}

/* New copied lookup/status boundary; retain the original fixture assertions. */
int addons_base_por_id_copiar(const char *id, char *dst, unsigned tam) {
  const char *base = addons_base_por_id(id);
  if (!tam) return 0;
  if (strlen(base) >= tam) { dst[0] = 0; return 0; }
  snprintf(dst, tam, "%s", base); return base[0] != 0;
}
AddonIdentidadeEstado addons_identidade_estado(const char *id) {
  (void)id; return ADD_IDENTIDADE_AUSENTE;
}

/* New request-owned collection metadata boundary. Existing appearance and
 * persistence assertions remain unchanged; network contents are fixed. */
int desc_colecao_inicial(const ColSource *source,const char *base,NvPerfilSessao epoch,CatItem *out,int limit,DescColecaoResultado *result){
 assert(source&&base&&*base&&source->type[0]&&source->catId[0]&&out&&limit>0&&result);(void)epoch;int count=limit<6?limit:6;for(int i=0;i<count;i++){memset(out+i,0,sizeof *out);snprintf(out[i].imdb,sizeof out[i].imdb,"tt%03d",i+1);snprintf(out[i].tipo,sizeof out[i].tipo,"%s",source->type[0]?source->type:"movie");snprintf(out[i].titulo,sizeof out[i].titulo,"Title %d",i+1);snprintf(out[i].meta,sizeof out[i].meta,"2001");}*result=(DescColecaoResultado){.n=count};return 1;
}
unsigned col_revisao(void){return 1;}
int col_n(void){return 0;}
const ColFolder *col_folder(int i){(void)i;return NULL;}
void col_chave_pasta(const ColFolder *f,char *dst,unsigned count){if(count)snprintf(dst,count,"collection_%s",f?(f->groupId[0]?f->groupId:f->group):"");}
unsigned addons_versao(void){return 1;}
int addons_base_desligada(const char *base){(void)base;return 0;}
