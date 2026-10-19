// #621: exercise the Android animation path with logical screens wider/taller
// than the partial frames. GL calls are recorded, not sent to a GPU.
#include "gif.h"
#include "nvtmp.h"
#include <assert.h>
#include <math.h>
#include <unistd.h>
#undef glTexImage2D
#undef glTexSubImage2D

int perftv_enabled,perftv_uploads;
static int uploadW,uploadH;
void gfx_vidro_sujar(void) {}
void glGenTextures(GLsizei n,GLuint *t) { assert(n==1);*t=7; }
void glBindTexture(GLenum target,GLuint t) {(void)target;(void)t;}
void glTexParameteri(GLenum t,GLenum p,GLint v) {(void)t;(void)p;(void)v;}
void glTexImage2D(GLenum t,GLint l,GLint i,GLsizei w,GLsizei h,GLint b,GLenum f,GLenum k,const void *px) {
  (void)t;(void)l;(void)i;(void)b;(void)f;(void)k;(void)px;uploadW=w;uploadH=h;
}
void glTexSubImage2D(GLenum t,GLint l,GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum k,const void *px) {
  (void)t;(void)l;(void)x;(void)y;(void)f;(void)k;assert(w==uploadW && h<=uploadH && px);
}
int main(void) {
  const int telas[][2]={{500,207},{280,498},{128,128}};
  assert(gif_aspecto()==0);
  for(unsigned i=0;i<sizeof telas/sizeof telas[0];i++) {
    unsigned char gif[]={
      'G','I','F','8','9','a',0,0,0,0,0x80,0,0,0,0,0,255,255,255,
      0x21,0xf9,4,0,4,0,0,0,0x2c,0,0,0,0,1,0,1,0,0,2,2,0x44,0x01,0,
      0x21,0xf9,4,0,4,0,0,0,0x2c,0,0,0,0,1,0,1,0,0,2,2,0x44,0x01,0,0x3b
    };
    int w=telas[i][0],h=telas[i][1];gif[6]=w;gif[7]=w>>8;gif[8]=h;gif[9]=h>>8;
    char caminho[1200];nvt_set(caminho,sizeof caminho,"/nuvio-gif-avatar.XXXXXX");
    int fd=mkstemp(caminho);assert(fd>=0);
    assert(write(fd,gif,sizeof gif)==sizeof gif);close(fd);
    GLuint t=0;
    for(int k=0;k<2000 && !t;k++) {t=gif_textura(caminho,120);if(!t)usleep(1000);}
    assert(t && uploadW>0 && uploadH>0);
    assert(fabsf(gif_aspecto()-(float)w/h)<0.00001f);
    // Reopening at a larger focus size retains the source aspect while the
    // replacement frame is still being decoded. Stopping clears it.
    (void)gif_textura(caminho,200);
    assert(fabsf(gif_aspecto()-(float)w/h)<0.00001f);
    gif_parar();assert(gif_aspecto()==0);unlink(caminho);
  }
  puts("gif avatar #621: Android decode/upload, logical-screen aspect, resize and stop pass");
}
