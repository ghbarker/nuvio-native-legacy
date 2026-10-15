#include NV_ADDONS_SOURCE
#include <assert.h>
static int cases;
static void selected(const char *a, const char *b, const char *first, const char *second, int expected) {
  const char *groups[3];
  ling_local_legenda(a); ling_local_legenda2(b);
  assert(gruposIdioma(groups) == expected);
  assert(!strcmp(groups[0], first));
  if (expected>1) assert(!strcmp(groups[1], second));
  if (expected>2) assert(!strcmp(groups[2], "en"));
  cases++;
}
int main(void) {
  ling_conta_legenda(""); ling_conta_legenda2("");
  selected("", "", "", NULL, 1);
  selected("fr", "", "fr", "en", 2);
  selected("ja", "", "ja", "en", 2);
  selected("ro", "es", "ro", "es", 3);
  selected("en", "", "en", NULL, 1);
  selected("pt", "pob", "pt", "en", 2);
  selected("*", "", "", NULL, 1);
  ling_conta_legenda("de"); ling_local_legenda(""); ling_local_legenda2("");
  { const char *groups[3]; assert(gruposIdioma(groups)==2 && !strcmp(groups[0],"de") && !strcmp(groups[1],"en")); cases++; }
  printf("unchanged actual subtitle language-group cases=%d\n", cases);
}
