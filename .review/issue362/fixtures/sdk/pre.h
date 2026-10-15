#include <SDL2/SDL.h>
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A))<=0)
#endif

#define RW_SEEK_SET SEEK_SET
#include <GLES2/gl2.h>
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
void glDeleteTextures(GLsizei,const GLuint*);
