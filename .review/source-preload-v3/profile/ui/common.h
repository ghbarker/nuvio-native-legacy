#ifndef PROFILE_TEST_COMMON_H
#define PROFILE_TEST_COMMON_H
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "perfis.h"
#include "dados.h"
static atomic_uint_fast64_t ep = 1;
static atomic_int pf = 1;
static pthread_mutex_t epLock;
static char dataPath[300];
static int deleted;
static void setup(void) {
  pthread_mutexattr_t a; pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&epLock,&a); pthread_mutexattr_destroy(&a);
  snprintf(dataPath,sizeof dataPath,"%s",getenv("PROFILE_TEST_DATA"));
  assert(dataPath[0] && !strncmp(dataPath,"/tmp/",5));
}
uint64_t perfis_sessao_geracao(void) { return atomic_load(&ep); }
int perfis_sessao_atual(uint64_t x) { return x == atomic_load(&ep); }
int perfis_sessao_travar(uint64_t x) { pthread_mutex_lock(&epLock); if(perfis_sessao_atual(x))return 1;pthread_mutex_unlock(&epLock);return 0; }
void perfis_sessao_destravar(void) { pthread_mutex_unlock(&epLock); }
int perfis_ativo(void) { return atomic_load(&pf); }
static void selectProfile(int p) { pthread_mutex_lock(&epLock);atomic_store(&pf,p);atomic_fetch_add(&ep,1);pthread_mutex_unlock(&epLock); }
Uint32 SDL_GetTicks(void) { return 1000; }
struct SDL_mutex { pthread_mutex_t m; };
SDL_mutex *SDL_CreateMutex(void) { SDL_mutex *m=malloc(sizeof*m);pthread_mutex_init(&m->m,NULL);return m; }
int SDL_LockMutex(SDL_mutex *m) { return pthread_mutex_lock(&m->m); }
int SDL_UnlockMutex(SDL_mutex *m) { return pthread_mutex_unlock(&m->m); }
const char *dados_dir(void) { return dataPath; }
char *dados_caminho(char *out,unsigned n,const char *name) { snprintf(out,n,"%s/%s",dataPath,name);return out; }
char *dados_ler(const char *name) { char path[400];dados_caminho(path,sizeof path,name);FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);char*s=malloc((size_t)n+1);assert(fread(s,1,(size_t)n,f)==(size_t)n);s[n]=0;fclose(f);return s; }
int dados_gravar(const char *name,const char *s) {char path[400];dados_caminho(path,sizeof path,name);FILE*f=fopen(path,"wb");assert(f);fputs(s,f);fclose(f);return 1;}
int dados_gravar_leve(const char *name,const char*s){return dados_gravar(name,s);}
int dados_apagar(const char *name){char path[400];deleted++;dados_caminho(path,sizeof path,name);return unlink(path)==0;}

#endif
