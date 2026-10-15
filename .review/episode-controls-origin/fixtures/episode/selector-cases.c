#include "proximo.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifndef HAVE_PREVIOUS
#define HAVE_PREVIOUS 1
#endif
static int checks;
static void expect(int actual, int wanted, const char *what) {
  if (actual != wanted) fprintf(stderr, "%s: wanted %d, got %d\n", what, wanted, actual);
  assert(actual == wanted); checks++;
}
int main(void) {
  CatEp rows[] = {
    {.temporada=2,.episodio=3}, {.temporada=1,.episodio=4},
    {.temporada=1,.episodio=1}, {.temporada=1,.episodio=4},
    {.temporada=0,.episodio=1}, {.temporada=2,.episodio=1},
    {.temporada=-1,.episodio=2}, {.temporada=1,.episodio=0},
    {.temporada=3,.episodio=2}
  };
  CatEp before[sizeof rows / sizeof rows[0]];
  const int n=(int)(sizeof rows/sizeof rows[0]);
  memcpy(before,rows,sizeof rows);
  expect(prox_indice_seguinte(rows,n,1,3),1,"Next chooses smallest greater pair in unsorted metadata");
  expect(prox_indice_seguinte(rows,n,1,4),5,"Next crosses season and skips duplicate current episode");
  expect(prox_indice_seguinte(rows,n,2,2),0,"Next handles missing current episode and numbering gap");
  expect(prox_indice_seguinte(rows,n,3,2),-1,"Next stops at last known episode");
  expect(prox_indice_seguinte(NULL,n,1,1),-1,"Next with missing metadata is unavailable");
  expect(prox_indice_seguinte(rows,0,1,1),-1,"Next with empty metadata is unavailable");
  expect(prox_indice_seguinte(rows,-1,1,1),-1,"Next rejects negative metadata count");
  CatEp single={.temporada=1,.episodio=1};
  expect(prox_indice_seguinte(&single,1,1,1),-1,"Single-episode series has no Next");
  CatEp invalid[]={{.temporada=-1,.episodio=4},{.temporada=1,.episodio=-2},{.temporada=1,.episodio=0},{.temporada=0,.episodio=8}};
  expect(prox_indice_seguinte(invalid,4,1,1),-1,"Next ignores invalid rows and regular-series specials");
  CatEp extreme[]={{.temporada=INT_MAX,.episodio=INT_MAX},{.temporada=INT_MAX,.episodio=INT_MAX-1}};
  expect(prox_indice_seguinte(extreme,2,INT_MAX,INT_MAX-1),0,"Next ordering does not overflow integers");
#if HAVE_PREVIOUS
  expect(prox_indice_anterior(rows,n,1,3),2,"Previous chooses greatest lower pair in unsorted metadata");
  expect(prox_indice_anterior(rows,n,1,4),2,"Previous skips duplicate current episode");
  expect(prox_indice_anterior(rows,n,2,1),1,"Previous crosses season and chooses first duplicate of prior pair");
  expect(prox_indice_anterior(rows,n,2,2),5,"Previous handles missing current episode and numbering gap");
  expect(prox_indice_anterior(rows,n,1,1),-1,"Previous stops at first regular episode and skips specials");
  expect(prox_indice_anterior(rows,n,3,2),0,"Previous from last known episode remains available");
  expect(prox_indice_anterior(NULL,n,1,1),-1,"Previous with missing metadata is unavailable");
  expect(prox_indice_anterior(rows,0,1,1),-1,"Previous with empty metadata is unavailable");
  expect(prox_indice_anterior(rows,-1,1,1),-1,"Previous rejects negative metadata count");
  expect(prox_indice_anterior(&single,1,1,1),-1,"Single-episode series has no Previous");
  expect(prox_indice_anterior(invalid,4,1,9),-1,"Previous ignores invalid rows and regular-series specials");
  expect(prox_indice_anterior(extreme,2,INT_MAX,INT_MAX),1,"Previous ordering does not overflow integers");
#endif
  assert(!memcmp(before,rows,sizeof rows)); checks++;
  printf("PASS %d actual episode-selector checks; catalog bytes unchanged\n",checks);
  return 0;
}
