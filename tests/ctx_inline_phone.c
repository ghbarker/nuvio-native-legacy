/* Production inline action geometry, clipping and focus callback. The
 * metadata/font draw boundaries are inert; no account or provider calls. */
#define main context_style_fixture_main
#include "ctxmenu_retrato.c"
#undef main
static float infoHeight=200;
int cat_n(void) { return 0; }
int cat_indice_por_imdb(const char *id) { (void)id;return -1; }
const CatItem *cat_item(int i) { (void)i;return NULL; }
int cat_historico_estado_id(const char *id,const char *tipo) { (void)id;(void)tipo;return 0; }
int salvos_tem(const char *id) { (void)id;return 0; }
int cat_imdb_na_lista(const char *id) { (void)id;return 0; }
int simkl_ativo(void) { return 0; }
int simkl_na_plantowatch(const char *id) { (void)id;return 0; }
float ctxinfo_compacto(const CatItem *ci,const CtxInfoEstado *state,float x,float y,float w,float a,int draw) {
  (void)ci;(void)state;(void)a;if(draw)desenho((GfxRect){x,y,w,infoHeight});return infoHeight;
}
int logotitulo_desenhar(const CatItem *ci,const char *s,TxtEstilo e,float x,float y,float maxw,float maxh,float fallback,float a) {
  (void)ci;(void)s;(void)e;(void)maxw;(void)maxh;(void)a;desenho((GfxRect){x,y,fallback,52});return 1;
}
int main(void) {
  const float sizes[][2]={{1080,1920},{1080,2340},{2160,1080},{2520,1080}},scales[]={1,1.2f,1.3f,1.5f};
  for(int t=0;t<4;t++)for(int s=0;s<4;s++) {
    nv_layout_w=sizes[t][0];nv_layout_h=sizes[t][1];ui=escala=scales[s];fonte=1;
    aberto=inlineOn=doPainel=1;pagina=doSocial=doLista=soFileira=0;anim=1;
    memset(&copiaPainel,0,sizeof copiaPainel);snprintf(copiaPainel.imdb,sizeof copiaPainel.imdb,"tt-fixture");
    snprintf(copiaPainel.tipo,sizeof copiaPainel.tipo,"movie");nOps=3;
    ops[0]=(typeof(ops[0])){"Remover",OP_LISTA};ops[1]=(typeof(ops[0])){"Assistido",OP_ASSISTIDO};ops[2]=(typeof(ops[0])){"Categoria",OP_CATEGORIA};
    float w=fminf(724,NV_TELA_W-100),x=(NV_TELA_W-w)*.5f,y=220,faixa=150;
    GfxRect rows[3];for(int i=0;i<3;i++){foco=i;assert(ctx_inline_foco_rect(w,faixa,&rows[i])==i+1);}
    GfxRect clip={x, y+rows[0].y, w, 50};ctx_inline_recorte(clip);
    gfx_recorte(clip.x,clip.y,clip.w,clip.h);nAlvos=0;validar=1;
    ctx_inline_desenhar(x,y,w,faixa,1);assert(nAlvos==3&&!inlineRecorteValido);
    for(int i=0;i<3;i++) {
      GfxRect actual={alvos[i].x,alvos[i].y,alvos[i].w,alvos[i].h};
      perto(actual.x,x+rows[i].x);perto(actual.y,y+rows[i].y);perto(actual.w,rows[i].w);perto(actual.h,rows[i].h);
      dentro(actual,clip);assert(alvos[i].a==i&&alvos[i].focar==ponteiroCtxOpcao);
      alvos[i].focar(i,0);assert(foco==i);
    }
    nAlvos=0;ctx_inline_desenhar(x,y,w,faixa,1);assert(!nAlvos); // no stale clip reuse
    clip.h=17;ctx_inline_recorte(clip);nAlvos=0;ctx_inline_desenhar(x,y,w,faixa,1);
    assert(nAlvos==3);for(int i=0;i<3;i++){perto(alvos[i].h,17);dentro((GfxRect){alvos[i].x,alvos[i].y,alvos[i].w,alvos[i].h},clip);}
    pagina=1;assert(!ctx_inline_painel_ativo()&&!ctx_inline_foco_rect(w,faixa,&rows[0]));
    gfx_sem_recorte();
  }
  puts("ctx_inline_phone: production focused geometry equals clipped rendered targets, current page and clip lifetime PASS");
}
