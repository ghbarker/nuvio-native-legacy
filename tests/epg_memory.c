#include "stub_perfisessao.h"
#include "../src/epg.c"
#include <assert.h>

static void tempo(char *out,time_t t) { strftime(out,32,"%Y%m%d%H%M%S +0000",gmtime(&t)); }
static void programa(char *xml,size_t cap,size_t *n,time_t ini,time_t fim,const char *nome) {
  char a[32],b[32];tempo(a,ini);tempo(b,fim);
  int k=snprintf(xml+*n,cap-*n,"<programme channel=\"news\" start=\"%s\" stop=\"%s\"><title>%s</title></programme>",a,b,nome);
  assert(k>0 && (size_t)k<cap-*n);*n+=(size_t)k;
}
int main(void) {
  epg_memoria_definir(EPG_MEMORIA_AUTO,1024);assert(epgMemoriaBaixa && epgMemoriaFios==1);
  epg_memoria_definir(EPG_MEMORIA_AUTO,0);assert(epgMemoriaBaixa);
  epg_memoria_definir(EPG_MEMORIA_AUTO,8192);assert(!epgMemoriaBaixa && epgMemoriaFios==EPG_MAX_FIOS);
  epg_memoria_definir(EPG_MEMORIA_COMPLETA,512);assert(!epgMemoriaBaixa);
  epg_memoria_definir(EPG_MEMORIA_BAIXA,8192);assert(epgMemoriaBaixa);
  epg_memoria_definir(EPG_MEMORIA_COMPLETA,8192);
  // A mode change cancels the old worker; its snapshot must stay immutable.
  carga.memoriaBaixa=0;fioVivo=1;extraMudou=0;reiniciarPend=0;
  epg_memoria_definir(EPG_MEMORIA_BAIXA,8192);
  assert(carga.cancelada && reiniciarPend && extraMudou && !carga.memoriaBaixa);
  fioVivo=0;carga.cancelada=0;reiniciarPend=0;
  char xml[4096];size_t n=(size_t)snprintf(xml,sizeof xml,"<tv><channel id=\"news\"><display-name>News</display-name></channel>");
  time_t now=time(NULL);carga.agora=now;
  programa(xml,sizeof xml,&n,now-7200,now,"Expired");
  programa(xml,sizeof xml,&n,now-7200,now+900,"Current");
  programa(xml,sizeof xml,&n,now+900,now+3600,"Next");
  programa(xml,sizeof xml,&n,now+86399,now+90000,"Boundary");
  programa(xml,sizeof xml,&n,now+86400,now+90000,"Tomorrow");
  programa(xml,sizeof xml,&n,now+172800,now+180000,"Later");
  snprintf(xml+n,sizeof xml-n,"</tv>");
  carga.memoriaBaixa=0;assert(wAbrir(&W));assert(wProcessar(&W,xml)==6);wFechar(&W);
  assert(W.nCanais==1 && W.nEvs==6);gradeLiberar(&W);
  carga.memoriaBaixa=1;assert(wAbrir(&W));assert(wProcessar(&W,xml)==3);wFechar(&W);
  assert(W.nCanais==1 && W.nEvs==3);
  assert(!strcmp(W.arena+W.evs[0].tit,"Current"));assert(!strcmp(W.arena+W.evs[2].tit,"Boundary"));
  gradeLiberar(&W);
  assert(wAbrir(&W));int programas=0;
  assert(xmltvExtra(xml,(long)strlen(xml),NULL,NULL,0,NULL,NULL));
  assert(xmltvExtra(xml,(long)strlen(xml),&W,NULL,0,&programas,NULL) && programas==3);
  assert(W.nCanais==1 && W.nEvs==3);gradeLiberar(&W);
  puts("PASS: device policy, explicit overrides, immutable in-flight snapshot, current programme and 24-hour boundaries; full mode restores provider schedule and channel metadata stays intact");
  return 0;
}
