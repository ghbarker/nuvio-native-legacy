// #621: animated avatars keep the same cover crop as still profile pictures.
#include <assert.h>
#include "../src/perfilsel.c"

float gfx_tex_aspect_atual;
static GLuint foto = 7, animacao = 8, desenhada;
static float aspectoFoto, aspectoGif, aspectoDesenhado;
static int reduzidas, anima = 1;
int ajustes_animacoes_reduzidas(void) { return reduzidas; }
int gif_pode_animar(void) { return anima; }
int gif_animado(const char *p) { (void)p; return 1; }
void gif_parar(void) {}
GLuint gif_textura(const char *p, int w) { (void)p; (void)w; return animacao; }
float gif_aspecto(void) { return aspectoGif; }
GLuint tex_obter_larg(const char *p, float w) { (void)p; (void)w; return foto; }
const char *tex_arquivo(const char *p) { (void)p; return "avatar.gif"; }
float tex_aspecto(const char *p) { (void)p; return aspectoFoto; }
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float f, float x, float y,
              float raio, float cr, float cg, float cb, float ca) {
  (void)r; (void)f; (void)x; (void)y; (void)raio;
  (void)cr; (void)cg; (void)cb; (void)ca;
  if (m == GFX_AVATAR) { desenhada = t; aspectoDesenhado = gfx_tex_aspect_atual; }
}
TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a) {
  (void)e;(void)s;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){0};
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) {(void)l;(void)x;(void)y;(void)a;}

static void conferir(ContaPerfil *p,int foco,GLuint textura,float aspecto) {
  desenhada=0;aspectoDesenhado=-1;
  disco((GfxRect){0,0,240,240},p,1,1,foco);
  assert(desenhada==textura);
  assert(fabsf(aspectoDesenhado-aspecto)<0.00001f);
  assert(gfx_tex_aspect_atual==0);
}
int main(void) {
  ContaPerfil p={0};strcpy(p.avatarUrl,"https://example.invalid/avatar.gif");
  const float aspectos[]={500.0f/207.0f,280.0f/498.0f,1.0f};
  for (unsigned i=0;i<sizeof aspectos/sizeof aspectos[0];i++) {
    aspectoFoto=aspectoGif=aspectos[i];
    gifDono=NULL;gifTex=0;conferir(&p,1,animacao,aspectoGif);
    // A frame can arrive before the still image is uploaded to the cache.
    foto=0;aspectoFoto=0;conferir(&p,1,animacao,aspectoGif);
    foto=7;aspectoFoto=aspectos[i];
    conferir(&p,0,foto,aspectoFoto);
    reduzidas=1;conferir(&p,1,foto,aspectoFoto);reduzidas=0;
    anima=0;conferir(&p,1,foto,aspectoFoto);anima=1;
    animacao=0;gifDono=NULL;conferir(&p,1,foto,aspectoFoto);animacao=8;
  }
  puts("perfilsel avatar #621: wide, tall and square GIFs preserve cover aspect; pending/static/reduced-motion fallbacks pass");
}
