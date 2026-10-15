/* The generated include contains the exact production JNI decoder and track
 * callback. Only the JNI string transport is replaced by UTF-16 memory. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "video.h"
#include "linguas.h"
#include "audioinfo.h"
typedef uint16_t jchar;
typedef int jsize;
typedef int jint;
typedef void *jclass;
typedef struct TestJString { const jchar *chars; jsize length; } *jstring;
typedef const struct TestJNIFunctions *JNIEnv;
struct TestJNIFunctions {
  const jchar *(*GetStringChars)(JNIEnv *, jstring, void *);
  jsize (*GetStringLength)(JNIEnv *, jstring);
  void (*ReleaseStringChars)(JNIEnv *, jstring, const jchar *);
};
#define JNIEXPORT
#define JNICALL
#define MAX_FAIXAS NV_FAIXA_MAX
static VideoFaixa novasA[MAX_FAIXAS], novasL[MAX_FAIXAS];
static int nNovasA, nNovasL;
const char *i18n(const char *s) { return s; }
static const jchar *chars(JNIEnv *env, jstring s, void *copy) { (void)env; (void)copy; return s->chars; }
static jsize length(JNIEnv *env, jstring s) { (void)env; return s->length; }
static void release(JNIEnv *env, jstring s, const jchar *p) { (void)env; (void)s; (void)p; }
#include SOURCE_BRIDGE_FUNCTIONS

static int valid_utf8(const unsigned char *s) {
  while (*s) {
    unsigned c = *s++, n = c < 0x80 ? 0 : c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : 99;
    if (n == 99) return 0;
    while (n--) { if ((*s & 0xc0) != 0x80) return 0; s++; }
  }
  return 1;
}
int main(void) {
  static const struct TestJNIFunctions funcs = {chars, length, release};
  JNIEnv env = &funcs;
  jchar en[] = {'e','n'};
  jchar full[] = {'F','u','l','l',' ','h','o','n','o','r','i','f','i','c','s'};
  struct TestJString language = {en, 2}, title = {full, 15};
  Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(&env, NULL, 1, 7, &language, 0, &title, NULL, 0);
  assert(nNovasL == 1 && nNovasA == 0);
  assert(!strcmp(novasL[0].nome, "Full honorifics"));
  assert(!strcmp(novasL[0].idioma, "en"));
  assert(novasL[0].numero == 7 && novasL[0].ordinalMkv == 7);
  assert(novasL[0].tipoLeg == LING_LEG_COMPLETA && !novasL[0].letreiro);
  assert(strstr(novasL[0].rotulo, i18n(ling_nome("en"))));
  Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(&env, NULL, 1, 8, &language, 1, NULL, NULL, 0);
  assert(!novasL[1].nome[0] && novasL[1].letreiro);
  assert(novasL[1].numero == 8 && !strcmp(novasL[1].idioma, "en"));

  /* Name preservation is independent of the old 48-byte classifier buffer. */
  jchar long_title[180];
  for (int i=0; i<120; i++) long_title[i] = 'x';
  long_title[120] = 0xd83c; long_title[121] = 0xdfb5;  /* musical note */
  for (int i=122; i<180; i++) long_title[i] = 0x65e5; /* Japanese character */
  struct TestJString long_name = {long_title, 180};
  Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(&env, NULL, 1, 9, &language, 0, &long_name, NULL, 0);
  assert(strlen(novasL[2].nome) > 48 && strlen(novasL[2].nome) < sizeof novasL[2].nome);
  assert(valid_utf8((const unsigned char *)novasL[2].nome));
  assert(novasL[2].nome[0] == 'x' && novasL[2].numero == 9);

  jchar unicode[] = {'F','u','l','l',' ',0x65e5,0x672c,0x8a9e,' ',0xd83c,0xdfb5};
  struct TestJString utf = {unicode, 11};
  Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(&env, NULL, 1, 10, &language, 0, &utf, NULL, 0);
  assert(!strcmp(novasL[3].nome, "Full 日本語 🎵"));
  assert(valid_utf8((const unsigned char *)novasL[3].nome));

  /* Audio continues to use its established language/codec display. */
  Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(&env, NULL, 0, 2, &language, 0, &title, NULL, 0);
  assert(nNovasA == 1 && !novasA[0].nome[0]);
  assert(!strcmp(novasA[0].idioma, "en") && novasA[0].numero == 2);
  puts("android names: exact production JNI callback and UTF-16 decoder checks passed");
  return 0;
}
