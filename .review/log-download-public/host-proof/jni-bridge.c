#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include ANDROID_SOURCE

static int mode, exception, new_strings, calls, deletes, cleared;
static char act_tag, cls_tag;
static const struct JNINativeInterface_ functions;
static JNIEnv env = &functions;
void *SDL_AndroidGetJNIEnv(void) { return mode == 1 ? NULL : &env; }
void *SDL_AndroidGetActivity(void) { return mode == 2 ? NULL : &act_tag; }
static jclass JNICALL object_class(JNIEnv *e, jobject a) {
  assert(e == &env && a == (jobject)&act_tag);
  return mode == 3 ? NULL : (jclass)&cls_tag;
}
static jmethodID JNICALL method(JNIEnv *e, jclass cls, const char *name, const char *signature) {
  assert(e == &env && cls == (jclass)&cls_tag);
  assert(!strcmp(name, "baixarRegistro"));
  assert(!strcmp(signature, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"));
  if (mode == 4) { exception = 1; return NULL; }
  return (jmethodID)1;
}
static jstring JNICALL string(JNIEnv *e, const char *value) {
  assert(e == &env && value);
  new_strings++;
  if (mode == 4 + new_strings) { exception = 1; return NULL; }
  return (jstring)strdup(value);
}
static void JNICALL call(JNIEnv *e, jobject a, jmethodID id, ...) {
  assert(e == &env && a == (jobject)&act_tag && id == (jmethodID)1);
  va_list args; va_start(args, id);
  assert(!strcmp((char *)va_arg(args, jstring), "/private/current.log"));
  assert(!strcmp((char *)va_arg(args, jstring), "localized success"));
  assert(!strcmp((char *)va_arg(args, jstring), "localized failure"));
  va_end(args);
  calls++;
  if (mode == 8) exception = 1;
}
static jboolean JNICALL has_exception(JNIEnv *e) { assert(e == &env); return exception; }
static void JNICALL clear_exception(JNIEnv *e) { assert(e == &env && exception); exception = 0; cleared++; }
static void JNICALL delete(JNIEnv *e, jobject object) {
  assert(e == &env && object);
  deletes++;
  if (object != (jobject)&act_tag && object != (jobject)&cls_tag) free(object);
}
static const struct JNINativeInterface_ functions = {
  .GetObjectClass = object_class, .GetMethodID = method, .NewStringUTF = string,
  .CallVoidMethod = call, .ExceptionCheck = has_exception,
  .ExceptionClear = clear_exception, .DeleteLocalRef = delete
};
int main(void) {
  for (mode = 0; mode <= 8; mode++) {
    exception = new_strings = calls = deletes = cleared = 0;
    android_baixar_registro("/private/current.log", "localized success", "localized failure");
    assert(!exception);
    assert(calls == (mode == 0 || mode == 8));
    assert(deletes == (mode == 1 || mode == 2 ? 0 : mode == 3 ? 1 :
                       mode == 4 || mode == 5 ? 2 : mode == 6 ? 3 : mode == 7 ? 4 : 5));
    assert(cleared == (mode >= 4));
    printf("PASS JNI mode %d\n", mode);
  }
}
