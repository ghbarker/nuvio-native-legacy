// Validacao estrita para pulos automaticos; leitura manual permanece tolerante.
#ifndef NV_INTROAUTO_H
#define NV_INTROAUTO_H
#include "intro.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static inline const char *ia_espaco(const char *p,const char *f) {
  while(p<f&&(*p==' '||*p=='\t'||*p=='\r'||*p=='\n'))p++;
  return p;
}
static inline const char *ia_string(const char *p,const char *f) {
  if(p>=f||*p++!='"')return NULL;
  while(p<f){
    unsigned char c=(unsigned char)*p++;
    if(c=='"')return p;
    if(c<32)return NULL;
    if(c=='\\'){
      if(p>=f)return NULL;
      c=(unsigned char)*p++;
      if(c=='u'){
        for(int i=0;i<4;i++){
          if(p>=f||!strchr("0123456789abcdefABCDEF",*p))return NULL;
          p++;
        }
      }else if(!strchr("\"\\/bfnrt",c))return NULL;
    }
  }
  return NULL;
}
static inline const char *ia_numero(const char *p,const char *f) {
  if(p<f&&*p=='-')p++;
  if(p>=f)return NULL;
  if(*p=='0')p++;
  else {if(*p<'1'||*p>'9')return NULL;while(p<f&&*p>='0'&&*p<='9')p++;}
  if(p<f&&*p=='.'){
    p++;if(p>=f||*p<'0'||*p>'9')return NULL;
    while(p<f&&*p>='0'&&*p<='9')p++;
  }
  if(p<f&&(*p=='e'||*p=='E')){
    p++;if(p<f&&(*p=='+'||*p=='-'))p++;
    if(p>=f||*p<'0'||*p>'9')return NULL;
    while(p<f&&*p>='0'&&*p<='9')p++;
  }
  return p;
}
static inline const char *ia_valor(const char *p,const char *f,int nivel) {
  const char *q;
  p=ia_espaco(p,f);if(p>=f||nivel>16)return NULL;
  if(*p=='"')return ia_string(p,f);
  if(*p=='{'||*p=='['){
    int objeto=*p=='{';char fecha=objeto?'}':']';p=ia_espaco(p+1,f);
    if(p<f&&*p==fecha)return p+1;
    for(;;){
      if(objeto){p=ia_string(p,f);if(!p)return NULL;p=ia_espaco(p,f);if(p>=f||*p++!=':')return NULL;}
      p=ia_valor(p,f,nivel+1);if(!p)return NULL;p=ia_espaco(p,f);
      if(p>=f)return NULL;
      if(*p==fecha)return p+1;
      if(*p++!=',')return NULL;
      p=ia_espaco(p,f);
    }
  }
  if(f-p>=4&&!memcmp(p,"null",4))return p+4;
  if(f-p>=4&&!memcmp(p,"true",4))return p+4;
  if(f-p>=5&&!memcmp(p,"false",5))return p+5;
  q=ia_numero(p,f);return q;
}
// Campo unico da raiz de um objeto ja validado.
static inline int ia_campo(const char *p,const char *f,const char *chave,
                           const char **vi,const char **vf){
  size_t n=strlen(chave);int achou=0;
  p=ia_espaco(p,f);if(p>=f||*p!='{')return 0;p=ia_espaco(p+1,f);
  while(p<f&&*p!='}'){
    const char *nome=p,*nf=ia_string(p,f),*fim;
    if(!nf)return 0;
    if(memchr(nome,'\\',(size_t)(nf-nome)))return 0; // chave escapada pode duplicar ASCII
    p=ia_espaco(nf,f);if(p>=f||*p++!=':')return 0;p=ia_espaco(p,f);
    fim=ia_valor(p,f,1);if(!fim)return 0;
    if((size_t)(nf-nome)==n+2&&!memcmp(nome+1,chave,n)){
      if(achou)return 0;
      achou=1;*vi=p;*vf=fim;
    }
    p=ia_espaco(fim,f);if(p<f&&*p==',')p=ia_espaco(p+1,f);else break;
  }
  return achou;
}

// Exigir numeros JSON finitos em chaves diretas e unicas de um objeto completo.
static inline int introauto_json(const char *p,const char *f,double *inicio,double *fim) {
  double v[2]={0,0};unsigned vistos=0;
  const char *q;
  if(!p||!f||p>=f||*p!='{')return 0;
  q=ia_valor(p,f,0);if(!q||ia_espaco(q,f)!=f)return 0;
  p=ia_espaco(p+1,f);
  while(p<f&&*p!='}'){
    const char *nome=p,*nf=ia_string(p,f),*vf;int k=-1;
    if(!nf)return 0;
    if(memchr(nome,'\\',(size_t)(nf-nome)))return 0;
    if(nf-nome==10&&!memcmp(nome,"\"start_ms\"",10))k=0;
    if(nf-nome==8&&!memcmp(nome,"\"end_ms\"",8))k=1;
    p=ia_espaco(nf,f);if(p>=f||*p++!=':')return 0;
    p=ia_espaco(p,f);vf=ia_valor(p,f,1);if(!vf)return 0;
    if(k>=0){
      char bruto[64];size_t n=(size_t)(vf-p);const char *finNum=ia_numero(p,vf);
      if((vistos&(1u<<k))||finNum!=vf||n>=sizeof bruto)return 0;
      memcpy(bruto,p,n);bruto[n]=0;v[k]=strtod(bruto,NULL);
      if(!isfinite(v[k])||v[k]<0.0)return 0;
      vistos|=1u<<k;
    }
    p=ia_espaco(vf,f);if(p<f&&*p==',')p=ia_espaco(p+1,f);else break;
  }
  if(vistos!=3||v[1]<=v[0])return 0;
  *inicio=v[0]/1000.0;*fim=v[1]/1000.0;return 1;
}

typedef struct {
  IntroTrecho usados[8];int n;
  double voltouDe,voltouAte,durFonte;
  int fonteIncompativel,voltou;
} IntroAuto;
static inline void introauto_zerar(IntroAuto *s){memset(s,0,sizeof *s);}
static inline void introauto_voltar(IntroAuto *s,double de,double para){
  if(para>=de)return;
  if(!s->voltou||de>s->voltouDe)s->voltouDe=de;
  if(!s->voltou||para<s->voltouAte)s->voltouAte=para;
  s->voltou=1;
}
static inline int introauto_limites(const IntroTrecho *t,double dur,int filme){
  if(!t->automatico||!isfinite(dur)||dur<=1.0||!isfinite(t->inicio)||!isfinite(t->fim)||
     t->inicio<0.0||t->fim<=t->inicio||t->fim>dur)return 0;
  if(t->tipo<INTRO_ABERTURA||t->tipo>INTRO_CREDITOS)return 0;
  if(t->fim-t->inicio>(t->tipo==INTRO_CREDITOS?900.0:180.0))return 0;
  return !(filme&&t->tipo==INTRO_CREDITOS&&t->inicio<dur*0.5);
}
static inline int introauto_decidir(IntroAuto *s,const IntroTrecho *v,int n,double pos,
                                   double dur,int filme,unsigned tipos,int elegivel,double *fim){
  if(!elegivel||!isfinite(pos)||s->fonteIncompativel)return 0;
  if(s->durFonte<=1.0)s->durFonte=dur;
  else if(fabs(s->durFonte-dur)>2.0){s->fonteIncompativel=1;return 0;}
  for(int i=0;i<n;i++){
    int usado=0;
    if(!introauto_limites(v+i,dur,filme)||!(tipos&(1u<<v[i].tipo))||pos<v[i].inicio||pos>=v[i].fim)continue;
    // Recuo manual tambem vale quando os marcadores chegam depois do gesto.
    if(s->voltou&&s->voltouDe>v[i].inicio&&s->voltouAte<v[i].fim)continue;
    for(int j=0;j<s->n;j++)if(s->usados[j].tipo==v[i].tipo&&
        s->usados[j].inicio==v[i].inicio&&s->usados[j].fim==v[i].fim)usado=1;
    if(usado||s->n>=8)continue;
    s->usados[s->n++]=v[i]; // Marcar antes do pedido evita repetir uma falha ou atraso.
    *fim=v[i].fim;return v[i].tipo;
  }
  return 0;
}
#endif
