/* Public GL scalar type needed by the CPU fixture's rendering mocks. */
#ifndef PHONE_CARDS_LOCAL_GLES_H
#define PHONE_CARDS_LOCAL_GLES_H
typedef unsigned int GLuint;typedef unsigned int GLenum;typedef int GLint;typedef int GLsizei;
enum{GL_TEXTURE_2D=0x0DE1,GL_UNPACK_ALIGNMENT=0x0CF5,GL_RGB=0x1907,GL_UNSIGNED_BYTE=0x1401,GL_TEXTURE_MIN_FILTER=0x2801,GL_TEXTURE_MAG_FILTER=0x2800,GL_NEAREST=0x2600,GL_TEXTURE_WRAP_S=0x2802,GL_TEXTURE_WRAP_T=0x2803,GL_CLAMP_TO_EDGE=0x812F};
void glGenTextures(GLsizei,GLuint*);void glBindTexture(GLenum,GLuint);void glPixelStorei(GLenum,GLint);void glTexImage2D(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);void glTexParameteri(GLenum,GLenum,GLint);
typedef unsigned int GLbitfield; typedef float GLfloat;enum{GL_COLOR_BUFFER_BIT=0x4000};void glClearColor(GLfloat,GLfloat,GLfloat,GLfloat);void glClear(GLbitfield);
#endif
