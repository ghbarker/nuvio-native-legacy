// Real GPU module with inert GL calls: resize must not reset its selected level.
#define GL_APICALL
#define SDL_MAIN_HANDLED
#define NV_ANDROID 1
#include "../src/gpunivel.c"
#include <assert.h>
#include <math.h>
#ifndef GL_APIENTRY
#define GL_APIENTRY
#endif

static GLuint nextObject = 10;
static int freedFbo, freedTexture, targetW, targetH, viewportW, viewportH;
static GfxRect copied;
float gfx_tex_aspect_atual, gfx_opacidade_grupo;
char *dados_ler(const char *name) { (void)name; return NULL; }
int dados_gravar(const char *name, const char *data) { (void)name; (void)data; return 1; }
void android_etapa(const char *name) { (void)name; }
int ptv_gpu_fraca(const char *name) { (void)name; return 0; }
void ptv_definir_gpu_fraca(int value) { (void)value; }
int ptv_gpu_fraca_atual(void) { return 0; }
const GLubyte *GL_APIENTRY glGetString(GLenum value) { (void)value; return (const GLubyte *)"test"; }

void GL_APIENTRY glGenTextures(GLsizei n, GLuint *ids) { while (n--) *ids++ = nextObject++; }
void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint *ids) { while (n--) *ids++ = nextObject++; }
void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint *ids) { (void)ids; freedTexture += n; }
void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint *ids) { (void)ids; freedFbo += n; }
void GL_APIENTRY glBindTexture(GLenum t, GLuint id) { (void)t; (void)id; }
void GL_APIENTRY glBindFramebuffer(GLenum t, GLuint id) { (void)t; (void)id; }
void GL_APIENTRY glTexImage2D(GLenum t, GLint l, GLint f, GLsizei w, GLsizei h,
                            GLint b, GLenum format, GLenum type, const void *p) {
  (void)t; (void)l; (void)f; (void)b; (void)format; (void)type; (void)p;
  targetW = w; targetH = h;
}
void GL_APIENTRY glTexParameteri(GLenum t, GLenum p, GLint v) { (void)t; (void)p; (void)v; }
void GL_APIENTRY glGetIntegerv(GLenum p, GLint *value) { (void)p; *value = 0; }
void GL_APIENTRY glFramebufferTexture2D(GLenum t, GLenum a, GLenum tt, GLuint id, GLint l) {
  (void)t; (void)a; (void)tt; (void)id; (void)l;
}
GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum t) { (void)t; return GL_FRAMEBUFFER_COMPLETE; }
void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
  assert(x == 0 && y == 0); viewportW = w; viewportH = h;
}
void GL_APIENTRY glDisable(GLenum v) { (void)v; }
void GL_APIENTRY glEnable(GLenum v) { (void)v; }
void GL_APIENTRY glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { (void)r; (void)g; (void)b; (void)a; }
void GL_APIENTRY glClear(GLbitfield mask) { (void)mask; }
void gfx_tex_esquecer(GLuint id) { (void)id; }
void gfx_definir_efeitos_leves(int v) { (void)v; }
void gfx_definir_efeitos_minimos(int v) { (void)v; }
void gfx_tamanho_alvo(int w, int h) { (void)w; (void)h; }
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float x, float y,
              float raio, float cr, float cg, float cb, float ca) {
  (void)tex; (void)foco; (void)x; (void)y; (void)raio; (void)cr; (void)cg; (void)cb; (void)ca;
  assert(modo == GFX_COPIA); copied = r;
}

int main(void) {
  layout_modo_definir(1);
  // Force the same 720p mode exposed by the interface setting.
  layout_tela_definir(2400, 1080);
  gpun_iniciar(2400, 1080);
  gpun_forcar_720();
  layout_tela_definir(telaW, telaH);
  gpun_quadro_inicio();
  assert(targetW == 1600 && targetH == 720);
  gpun_quadro_fim();
  assert(viewportW == 2400 && viewportH == 1080);
  GLuint oldFbo = intFbo;
  prefFixa = 1; decidido = 1;
  layout_tela_definir(2340, 1040);
  gpun_redimensionar(2340, 1040);
  assert(telaW == 2340 && telaH == 1040);
  assert(nivel == 3 && forca720 == 1 && prefFixa == 1 && decidido == 1);
  assert(!intFbo && !intTex && !intFalhou && !intLigado);
  assert(freedFbo == 1 && freedTexture == 1);
  gpun_quadro_inicio();
  assert(intFbo && intFbo != oldFbo && targetW == 1560 && targetH == 693);
  gpun_quadro_fim();
  assert(viewportW == 2340 && viewportH == 1040);
  assert(fabsf(copied.w - 2430.0f) < 0.01f && copied.h == 1080.0f);
  gpun_redimensionar(2340, 1040);
  gpun_redimensionar(0, 1080);
  assert(freedFbo == 1 && freedTexture == 1);
  assert(telaW == 2340 && telaH == 1040);
  // The 1080p fallback also follows the new logical aspect.
  alvo1080 = 1;
  layout_tela_definir(3120, 1440);
  gpun_redimensionar(3120, 1440);
  gpun_quadro_inicio();
  assert(targetW == 2340 && targetH == 1080);
  gpun_quadro_fim();
  assert(viewportW == 3120 && viewportH == 1440);
  layout_tela_definir(1440, 3120);
  gpun_redimensionar(1440, 3120);
  gpun_quadro_inicio();
  assert(targetW == 1080 && targetH == 2340);
  gpun_quadro_fim();
  assert(viewportW == 1440 && viewportH == 3120);
  assert(fabsf(copied.w - 1080.0f) < 0.01f && copied.h == 2340.0f);
  // Changing interface mode recreates the target even at the same physical size.
  GLuint portraitFbo = intFbo;
  int freedBefore = freedFbo;
  layout_modo_definir(0);
  gpun_redimensionar(1440, 3120);
  assert(!intFbo && !intTex && freedFbo == freedBefore + 1);
  gpun_quadro_inicio();
  assert(intFbo != portraitFbo && targetW == 1920 && targetH == 1080);
  gpun_quadro_fim();
  assert(copied.w == 1920 && copied.h == 1080);
  GLuint tvFbo = intFbo;
  freedBefore = freedFbo;
  layout_modo_definir(1);
  gpun_redimensionar(1440, 3120);
  assert(!intFbo && !intTex && freedFbo == freedBefore + 1);
  gpun_quadro_inicio();
  assert(intFbo != tvFbo && targetW == 1080 && targetH == 2340);
  gpun_quadro_fim();
  assert(copied.w == 1080 && copied.h == 2340);
  puts("gpun_touch: resize recreates both internal modes and preserves GPU selection PASS");
  return 0;
}
