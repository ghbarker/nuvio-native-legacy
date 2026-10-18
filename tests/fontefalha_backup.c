#include "fontefalha.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void recusar(const char *s,size_t n) {
  FonteFalhaConfig c,antes;memset(&c,0xa5,sizeof c);antes=c;
  assert(!fontefalha_backup_ler(s,n,&c) && !memcmp(&c,&antes,sizeof c));
}
static char *trocar(const char *s,const char *de,const char *para) {
  const char *p=strstr(s,de);assert(p);
  char *out=calloc(strlen(s)+strlen(para)+1,1);assert(out);
  memcpy(out,s,(size_t)(p-s));strcat(out,para);strcat(out,p+strlen(de));return out;
}
int main(int argc,char **argv) {
  assert(argc==2);
  FonteFalhaConfig c={0},d={0},ativa,apos;
  fontefalha_config(&ativa);
  c.provedores=c.addons=1;c.nAddons=FONTEFALHA_MAX_ADDONS;
  for(int i=0;i<4;i++){c.ordem[i]=3-i;c.limites[i]=i+1;}
  for(int i=0;i<c.nAddons;i++){snprintf(c.addon[i].chave,sizeof c.addon[i].chave,"%032x",i);c.addon[i].limite=i%10+1;}
  for(int i=0;i<FONTEFILTRO_N;i++)c.filtro.excluir[i]=(UINT64_C(1)<<fontefiltro_opcoes_n(i))-1;
  c.filtro.cache=FONTEFILTRO_CACHE_SIM;c.addon[0].aoVivo=1;
  c.filtro.maxGB=FONTEFILTRO_MAX_GB;strcpy(c.filtro.grupos,"GroupA,GroupB");
  char *s=NULL;assert(fontefalha_backup_exportar(&c,&s) && s);
  assert(fontefalha_backup_ler(s,strlen(s),&d));
  char *outra=NULL;assert(fontefalha_backup_exportar(&d,&outra));assert(!strcmp(s,outra));free(outra);
  assert(d.filtro.cache==FONTEFILTRO_CACHE_SIM && d.addon[0].aoVivo==1);
  assert(d.nAddons==128 && d.limites[0]==1 && d.ordem[0]==3 && d.filtro.maxGB==1000);
  const char *de[]={"SETTINGS 2","provedores 1","limites 1 2 3 4","ordem 3 2 1 0","contagem 128","filtro_maxgb 1000","filtro_grupos 47726f7570412c47726f757042"};
  const char *para[]={"SETTINGS 9","provedores 2","limites 0 2 3 4","ordem 3 2 1 1","contagem 999999999999999999999","filtro_maxgb 1001","filtro_grupos 00"};
  for(size_t i=0;i<sizeof de/sizeof *de;i++){char *m=trocar(s,de[i],para[i]);recusar(m,strlen(m));free(m);}
  char *m=trocar(s,"filtro_resolucao 00000000000000ff","filtro_resolucao 8000000000000000");recusar(m,strlen(m));free(m);
  m=trocar(s,"addon 00000000000000000000000000000001 2","addon 00000000000000000000000000000000 2");recusar(m,strlen(m));free(m);
  // Every truncation, trailing credentials/garbage and embedded NUL fails.
  for(size_t n=0;n<strlen(s);n++)recusar(s,n);
  m=malloc(strlen(s)+100);assert(m);strcpy(m,s);strcat(m,"token secret\n");recusar(m,strlen(m));
  strcpy(m,s);m[50]=0;recusar(m,strlen(s));free(m);
  recusar(s,16385);recusar(NULL,0);
  c.nAddons=129;outra=(char*)1;assert(!fontefalha_backup_exportar(&c,&outra) && !outra);c.nAddons=128;
  memset(c.addon[0].chave,'x',sizeof c.addon[0].chave);assert(!fontefalha_backup_exportar(&c,&outra));
  fontefalha_config(&apos);assert(!memcmp(&ativa,&apos,sizeof ativa));
  // The validated snapshot is also understood by the existing native store:
  // restoring it must change the real runtime settings, not scalar mirrors.
  char path[1100];snprintf(path,sizeof path,"%s/fontefalha.txt",argv[1]);
  FILE *f=fopen(path,"w");assert(f);assert(fwrite(s,1,strlen(s),f)==strlen(s));assert(!fclose(f));
  fontefalha_dir(argv[1]);fontefalha_config(&d);
  assert(fontefalha_backup_exportar(&d,&outra) && !strcmp(outra,s));free(outra);
  free(s);puts("PASS: source-policy snapshots preserve provider/add-on order, limits and filters; strict bounded decoding, atomic output and no active-state mutation");
}
