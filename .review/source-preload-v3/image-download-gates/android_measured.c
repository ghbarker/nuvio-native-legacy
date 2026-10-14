/* Actual rede.c; only the Android HTTP/JNI boundary and curl transport are
 * doubled. No network, credentials, phone logs, or replacement length helper. */
#define NV_ANDROID 1
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef ACTUAL_REDE_SOURCE
#error Define ACTUAL_REDE_SOURCE as the src/rede.c path
#endif
#include ACTUAL_REDE_SOURCE

static const char json[] = "{\"status\":\"success\",\"subtitles\":[{\"lang\":\"en\"}]}";
static unsigned char binary[2048], smallBinary[768];
static int androidCalls, curlCalls, curlCode, failures;

static void responseFor(const char *url, const void **data, long *n, int *status) {
  *status = 200;
  if (strstr(url, "/binary")) { *data = binary; *n = sizeof binary; }
  else if (strstr(url, "/small-binary")) { *data = smallBinary; *n = sizeof smallBinary; }
  else if (strstr(url, "/empty")) { *data = ""; *n = 0; }
  else if (strstr(url, "/one")) { *data = "x"; *n = 1; }
  else if (strstr(url, "/missing")) { *data = json; *n = sizeof json - 1; *status = 404; }
  else if (strstr(url, "/forbidden")) { *data = json; *n = sizeof json - 1; *status = 403; }
  else { *data = json; *n = sizeof json - 1; }
}

char *android_http(const char *method, const char *url, const char *headers,
                   const char *body, int timeout, int *status, long *n,
                   char *error, size_t errorSize) {
  const void *data;
  char *r;
  (void)method; (void)headers; (void)body; (void)timeout;
  __atomic_add_fetch(&androidCalls, 1, __ATOMIC_RELAXED);
  if (strstr(url, "/transport-failure")) {
    *status = 0; *n = 0;
    snprintf(error, errorSize, "synthetic transport failure");
    return NULL;
  }
  responseFor(url, &data, n, status);
  r = malloc((size_t)*n + 1);
  if (!r) exit(70);
  memcpy(r, data, (size_t)*n);
  r[*n] = 0;
  return r;
}
void android_etapa(const char *name) { (void)name; }
void android_quadro(void) {}

typedef struct {
  char url[256];
  size_t (*write)(void *, size_t, size_t, void *);
  void *writeData;
} FakeCurl;
static void *fakeInit(void) { return calloc(1, sizeof(FakeCurl)); }
static void fakeCleanup(void *p) { free(p); }
static void fakeReset(void *p) { memset(p, 0, sizeof(FakeCurl)); }
static int fakeOption(void *p, int option, ...) {
  FakeCurl *f = p;
  va_list ap;
  va_start(ap, option);
  if (option == OPT_URL) snprintf(f->url, sizeof f->url, "%s", va_arg(ap, const char *));
  else if (option == OPT_WRITEFUNCTION) f->write = va_arg(ap, size_t (*)(void *, size_t, size_t, void *));
  else if (option == OPT_WRITEDATA) f->writeData = va_arg(ap, void *);
  va_end(ap);
  return 0;
}
static int fakePerform(void *p) {
  FakeCurl *f = p;
  const void *data;
  long n;
  int status;
  __atomic_add_fetch(&curlCalls, 1, __ATOMIC_RELAXED);
  if (curlCode) return curlCode;
  responseFor(f->url, &data, &n, &status);
  if (f->write && n) f->write((void *)data, 1, (size_t)n, f->writeData);
  return 0;
}
static int fakeInfo(void *p, int option, ...) {
  FakeCurl *f = p;
  const void *data;
  long n;
  int status;
  va_list ap;
  responseFor(f->url, &data, &n, &status);
  va_start(ap, option);
  if (option == INFO_RESPONSE_CODE) *va_arg(ap, long *) = curlCode ? 0 : status;
  else if (option == INFO_NUM_CONNECTS) *va_arg(ap, long *) = 1;
  else if (option == INFO_SIZE_DOWNLOAD) *va_arg(ap, double *) = curlCode ? 0 : (double)n;
  else if (option == INFO_URL_FINAL) *va_arg(ap, char **) = f->url;
  va_end(ap);
  return 0;
}
static void *fakeAppend(void *p, const char *header) { (void)header; return p ? p : malloc(1); }

static void check(int ok, const char *name) {
  printf("CHECK %s %s\n", ok ? "PASS" : "FAIL", name);
  if (!ok) failures++;
}
static int measuredBinary(const char *path, long expected, const void *data, int expectedStatus) {
  char url[256];
  RedeMedida m = {0};
  long n = -1;
  char *r;
  int ok;
  snprintf(url, sizeof url, "https://image-test.example%s", path);
  r = rede_baixar_bin_medido_controle(url, 2, NULL, NULL, &n, &m);
  ok = r && n == expected && m.bytes == expected && m.status == expectedStatus &&
       !m.limitado && !m.cancelado && !memcmp(r, data, (size_t)expected);
  free(r);
  return ok;
}
static int measuredText(const char *path, int expectedStatus) {
  char url[256];
  RedeMedida m = {0};
  char *r;
  int ok;
  snprintf(url, sizeof url, "https://image-test.example%s", path);
  r = rede_baixar_medido_controle(url, 2, NULL, NULL, &m);
  ok = r && m.bytes == sizeof json - 1 && m.status == expectedStatus &&
       !m.limitado && !m.cancelado && !strcmp(r, json);
  free(r);
  return ok;
}
typedef struct { const char *path; long n; const void *data; int ok; } ThreadCase;
static void *threadRequests(void *p) {
  ThreadCase *c = p;
  int i;
  c->ok = 1;
  for (i = 0; i < 40; i++) {
    if (!measuredBinary(c->path, c->n, c->data, 200) ||
        !measuredBinary("/empty", 0, "", 200)) c->ok = 0;
  }
  return NULL;
}

int main(void) {
  unsigned i;
  int before;
  char *r;
  RedeMedida m;
  long n;
  pthread_t a, b;
  ThreadCase ta = {"/binary", sizeof binary, binary, 0};
  ThreadCase tb = {"/small-binary", sizeof smallBinary, smallBinary, 0};
  for (i = 0; i < sizeof binary; i++) binary[i] = (unsigned char)(i * 37u);
  for (i = 0; i < sizeof smallBinary; i++) smallBinary[i] = (unsigned char)(i * 61u);
  /* Image-like binary, with embedded NULs. strlen cannot recover this length. */
  memcpy(binary, "\211PNG\r\n\032\n", 8); binary[8] = 0;
  memcpy(smallBinary, "\xff\xd8\xff\xe0", 4); smallBinary[5] = 0;
  pronto = 1; curl_init = fakeInit; curl_cleanup = fakeCleanup;
  curl_reset = fakeReset; curl_setopt_f = fakeOption; curl_perform = fakePerform;
  curl_getinfo = fakeInfo; slist_append = fakeAppend; slist_free = free;
  __atomic_store_n(&curlRuim, 1, __ATOMIC_RELEASE);

  check(measuredBinary("/binary", sizeof binary, binary, 200), "android-first-binary-length-and-data");
  check(measuredText("/json", 200), "android-first-text-measurement");
  check(measuredBinary("/small-binary", sizeof smallBinary, smallBinary, 200), "success-after-different-length");
  check(measuredBinary("/one", 1, "x", 200), "single-byte-response");
  check(measuredBinary("/empty", 0, "", 200), "true-empty-body");
  check(measuredBinary("/missing", sizeof json - 1, json, 404), "404-body-and-status-retained");
  check(measuredText("/forbidden", 403), "403-text-body-and-status-retained");
  redeBytesLocal = 991;
  memset(&m, 0, sizeof m); n = -1;
  r = rede_baixar_bin_medido_controle("https://image-test.example/transport-failure", 2, NULL, NULL, &n, &m);
  check(!r && n == 0 && m.bytes == 0 && m.status == 0 && !m.limitado && !m.cancelado,
        "transport-failure-clears-count");
  free(r);
  redeBytesLocal = 993;
  check(measuredBinary("/empty", 0, "", 200), "empty-response-clears-previous-count");

  __atomic_store_n(&curlRuim, 0, __ATOMIC_RELEASE);
  curlCode = 28;
  before = androidCalls;
  check(measuredBinary("/binary", sizeof binary, binary, 200) && androidCalls == before + 1 &&
        curlCalls > 0 && __atomic_load_n(&curlRuim, __ATOMIC_ACQUIRE),
        "curl-failure-then-android-binary");
  before = curlCalls;
  check(measuredBinary("/small-binary", sizeof smallBinary, smallBinary, 200) && curlCalls == before,
        "sticky-android-after-curl-failure");

  if (pthread_create(&a, NULL, threadRequests, &ta) || pthread_create(&b, NULL, threadRequests, &tb)) return 71;
  pthread_join(a, NULL); pthread_join(b, NULL);
  check(ta.ok && tb.ok, "parallel-thread-local-lengths-and-empty-counts");

  __atomic_store_n(&curlRuim, 0, __ATOMIC_RELEASE); curlCode = 0;
  before = androidCalls;
  check(measuredBinary("/binary", sizeof binary, binary, 200) && androidCalls == before,
        "unchanged-curl-binary-path");
  /* Drop the main thread's retained fake curl handle for leak checking. */
  if (pthread_getspecific(handleChave)) {
    fakeCleanup(pthread_getspecific(handleChave));
    pthread_setspecific(handleChave, NULL);
  }
  printf("RESULT checks=13 failures=%d android_calls=%d curl_calls=%d\n", failures, androidCalls, curlCalls);
  return failures ? 42 : 0;
}
