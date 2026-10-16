/* Review-only draw-contract fixture. Complete player.c is included unchanged;
 * actual artehero.c policy and abrecine.c state machine are linked. Async
 * textures, detail ownership and drawing are typed boundary fakes. No GL or
 * networking occurs. Unreached dependencies abort if accidentally called. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "catalogo.h"
char *SDL_GetBasePath(void);
void SDL_free(void *);
#include PLAYER_SOURCE

static int clearStyle, reduced, smallRequests, blurRequests, pictures, snaps;
static int checks, failures, textureReady=1, snapshotPoster;
static char snapshot[2048], requested[2048], failed[2048];
static GfxRect lastPicture;
static float lastAlpha, shadeAlpha, fakeAspect=16.0f/9.0f;
float gfx_tex_aspect_atual;
#ifdef NV_TOUCH_UI
float nv_layout_w=1920, nv_layout_h=1080;
int layout_modo_mobile(void) { return 0; }
#endif

#ifdef JOINED_DETAIL
void fixture_detail_set(const CatItem *, const char *, int);
void fixture_detail_backdrop(const char *);
void fixture_detail_poster(const char *);
void fixture_detail_ownership_checks(void (*)(int,const char *));
#else
int detail_arte_titulo(const CatItem *c,char *out,size_t n,int *poster) {
  (void)c;
  if(poster)*poster=snapshotPoster;
  if(!snapshot[0] || strlen(snapshot)>=n) {if(n)out[0]=0;return 0;}
  snprintf(out,n,"%s",snapshot);return 1;
}
#endif
int ajustes_abertura_fundo_nitido(void) {return clearStyle;}
int ajustes_animacoes_reduzidas(void) {return reduced;}
int ajustes_hero_fonte(void) {return ARTEHERO_CATALOGO;}
int ajustes_hero_arte_diferente(void) {return 0;}
void ajustes_acento(float *r,float *g,float *b) {*r=.3f;*g=.6f;*b=1;}
int tex_falhou(const char *u) {return u && failed[0] && !strcmp(u,failed);}
GLuint tex_obter_hero(const char *u) {snprintf(requested,sizeof requested,"%s",u);return textureReady?11:0;}
GLuint tex_obter_larg(const char *u,float w) {(void)u;assert(w==480);smallRequests++;return textureReady?12:0;}
GLuint tex_obter_larg_qualquer(const char *u,float w) {(void)u;(void)w;return 0;}
int tex_marca_escura(const char *u) {(void)u;return 0;}
float tex_aspecto(const char *u) {(void)u;return fakeAspect;}
GLuint gfx_desfocado(GLuint t,const char *u) {assert(t==12&&u&&*u);blurRequests++;return 13;}
void gfx_rect(GfxRect r,GLuint tex,GfxModo mode,float f,float x,float y,float radius,float red,float green,float blue,float alpha) {
  (void)f;(void)x;(void)y;(void)radius;(void)red;(void)green;(void)blue;
  if(mode==GFX_CARD && tex==11){pictures++;lastPicture=r;lastAlpha=alpha;}
  if(mode==GFX_SNAP && tex==13)snaps++;
}
void gfx_cor(GfxRect r,float radius,float red,float green,float blue,float alpha) {
  (void)radius;
  if(r.w==1920&&r.h==1080&&red==0&&green==0&&blue==0)shadeAlpha=alpha;
}
TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float w) {
  (void)e;(void)s;(void)r;(void)g;(void)b;(void)a;(void)w;return(TxtLinha){.w=200,.h=50};
}
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  return txt_linha_corta(e,s,r,g,b,a,200);
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) {(void)l;(void)x;(void)y;(void)a;}
const char *i18n(const char *s) {return s;}
int video_pronto(void) {return 0;}
int video_falhou(void) {return 0;}
static void verify(int ok,const char *s) {checks++;printf("%s %s\n",ok?"PASS":"FAIL",s);if(!ok)failures++;}
static void resetDraw(void) {smallRequests=blurRequests=pictures=snaps=0;requested[0]=0;shadeAlpha=lastAlpha=0;}
static void begin(const CatItem *c) {
#ifdef JOINED_DETAIL
  fixture_detail_set(c,snapshot,snapshotPoster);
#endif
#ifdef ISSUE508
  cinemaArteIniciar(c);
#else
  (void)c;
#endif
  cine=(AbreCine){.viva=1,.veu=1,.foco=1,.desde=1000};
  aberto=1;esperandoFonte=1;comVideo=0;reduced=0;textureReady=1;failed[0]=0;fakeAspect=16.0f/9.0f;
  resetDraw();
}
static void draw(const CatItem *c) {desenharCinema(2000,c,0);}
int main(void) {
  CatItem c={0};snprintf(c.imdb,sizeof c.imdb,"tt1234567");snprintf(c.tipo,sizeof c.tipo,"movie");
  snprintf(c.backdrop,sizeof c.backdrop,"https://example.invalid/catalog.jpg");
  snprintf(c.backdropCatalogo,sizeof c.backdropCatalogo,"%s",c.backdrop);
  snprintf(c.poster,sizeof c.poster,"https://example.invalid/poster.jpg");
  artehero_definir_falhou(tex_falhou);
  clearStyle=0;snapshot[0]=0;begin(&c);draw(&c);
  verify(smallRequests==1&&blurRequests==1&&snaps==1,"default Cinema keeps cached blur rendering");
  clearStyle=1;begin(&c);draw(&c);
  verify(!smallRequests&&!blurRequests&&!snaps&&pictures==1,"clear Cinema skips blur and keeps a full image after settling");
  verify(lastPicture.w==1920&&lastPicture.h==1080&&lastAlpha==1,"clear landscape artwork fills the screen");
  verify(shadeAlpha>.54f&&shadeAlpha<.56f,"clear Cinema retains readable darkening");
  snprintf(snapshot,sizeof snapshot,"https://example.invalid/selected-art.jpg");begin(&c);draw(&c);
  verify(!strcmp(requested,snapshot),"Play reuses the exact selected title-page URL");
  snapshot[0]=0;c.backdrop[0]=c.backdropCatalogo[0]=0;begin(&c);draw(&c);
  verify(strstr(requested,"metahub.space/background/")!=NULL,"empty backdrop still uses same-title IMDb artwork fallback");
  c.imdb[0]=0;begin(&c);fakeAspect=2.0f/3.0f;draw(&c);
  verify(!strcmp(requested,c.poster)&&lastPicture.w==720&&lastPicture.h==1080&&lastPicture.x==600,"portrait fallback is contained without distortion");
  snapshotPoster=1;snprintf(snapshot,sizeof snapshot,"%s",c.poster);begin(&c);fakeAspect=2.0f/3.0f;draw(&c);
  verify(lastPicture.w==720&&lastPicture.x==600,"captured title-page poster keeps its portrait layout");
#ifdef JOINED_DETAIL
  snprintf(c.imdb,sizeof c.imdb,"tt1234567");
  snprintf(c.backdrop,sizeof c.backdrop,"%s",c.poster);
  snprintf(c.backdropCatalogo,sizeof c.backdropCatalogo,"%s",c.poster);
  begin(&c);draw(&c);
  fixture_detail_backdrop("https://example.invalid/late-real-backdrop.jpg");resetDraw();draw(&c);
  verify(!strcmp(requested,"https://example.invalid/late-real-backdrop.jpg"),"temporary poster captured at Play upgrades when same-title metadata arrives");
  fixture_detail_backdrop("https://example.invalid/later-different-backdrop.jpg");resetDraw();draw(&c);
  verify(!strcmp(requested,"https://example.invalid/late-real-backdrop.jpg"),"accepted real loading backdrop stays frozen during later metadata changes");
  c.imdb[0]=c.backdrop[0]=c.backdropCatalogo[0]=0;
#endif
  snapshotPoster=0;snapshot[0]=0;c.poster[0]=0;begin(&c);draw(&c);
  verify(!requested[0]&&!pictures&&!smallRequests&&!blurRequests,"missing artwork stays neutral without delaying the loading screen");
  snprintf(snapshot,sizeof snapshot,"https://example.invalid/previous-title.jpg");begin(&c);
  snapshot[0]=0;snprintf(c.backdrop,sizeof c.backdrop,"https://example.invalid/next-title.jpg");
  snprintf(c.backdropCatalogo,sizeof c.backdropCatalogo,"%s",c.backdrop);begin(&c);draw(&c);
  verify(!strcmp(requested,c.backdrop),"Next/direct opening discards previous title-page snapshot");
  snprintf(snapshot,sizeof snapshot,"https://example.invalid/dead-snapshot.jpg");begin(&c);
  snprintf(failed,sizeof failed,"%s",snapshot);draw(&c);
  verify(!strcmp(requested,c.backdrop),"failed captured artwork falls through same-title artwork policy");
  snapshot[0]=0;begin(&c);textureReady=0;draw(&c);
  verify(!pictures&&!smallRequests,"clear loading tolerates an asynchronous texture miss");
#ifdef ISSUE508
  snprintf(snapshot,sizeof snapshot,"https://example.invalid/private-title.jpg");cinemaArteIniciar(&c);
  cinemaArteIniciar(NULL);verify(!cinemaArte[0]&&!cinemaPoster,"session teardown clears copied artwork state");
#endif
  clearStyle=0;begin(&c);cine.foco=.25f;draw(&c);
  verify(pictures==1&&snaps==1&&lastAlpha==.75f,"default sharp-to-blurred transition is unchanged");
  verify(gfx_tex_aspect_atual==0,"renderer restores shared texture aspect state");
#ifdef JOINED_DETAIL
  clearStyle=1; snapshotPoster=1;
  snprintf(c.imdb,sizeof c.imdb,"local:owned");
  c.backdrop[0]=c.backdropCatalogo[0]=0;
  snprintf(c.poster,sizeof c.poster,"https://example.invalid/original-portrait.jpg");
  snprintf(snapshot,sizeof snapshot,"%s",c.poster); begin(&c);
  fixture_detail_poster("https://example.invalid/new-poster-only.jpg");
  fakeAspect=2.0f/3.0f; draw(&c);
  verify(!strcmp(requested,snapshot)&&cinemaPoster&&lastPicture.w==720&&lastPicture.x==600,
         "loading keeps frozen old poster contained after poster-only enrichment");
  fixture_detail_backdrop("https://example.invalid/promoted-landscape.jpg"); resetDraw(); draw(&c);
  verify(!strcmp(requested,"https://example.invalid/promoted-landscape.jpg")&&!cinemaPoster,
         "loading promotes the same temporary portrait when a real backdrop arrives");
  fixture_detail_ownership_checks(verify);
#endif
  printf("RESULT checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
