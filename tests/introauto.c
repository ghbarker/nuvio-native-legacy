#include "introauto.h"
#include <assert.h>
#include <stdio.h>

static unsigned todos(void){return (1u<<INTRO_ABERTURA)|(1u<<INTRO_RESUMO)|(1u<<INTRO_CREDITOS);}
static void json(const char *s,int ok){
  double a=-1,b=-1;int r=introauto_json(s,s+strlen(s),&a,&b);
  assert(r==ok);if(r)assert(a>=0.0&&b>a);
}
int main(void){
  json("{\"start_ms\":0,\"end_ms\":30000}",1);
  json("{\"start_ms\":1e3,\"end_ms\":2000.5,\"note\":{\"x\":[null,true,false,\"ok\"]}}",1);
  const char *invalidos[]={
    "{\"end_ms\":30000}","{\"start_ms\":null,\"end_ms\":30000}",
    "{\"start_ms\":0,\"end_ms\":null}","{\"start_ms\":-1,\"end_ms\":30000}",
    "{\"start_ms\":\"0\",\"end_ms\":30000}","{\"start_ms\":0,\"end_ms\":\"30000\"}",
    "{\"start_ms\":0,\"end_ms\":true}","{\"start_ms\":0,\"end_ms\":1e999}",
    "{\"start_ms\":NaN,\"end_ms\":30000}","{\"start_ms\":0,\"end_ms\":Infinity}",
    "{\"start_ms\":0,\"end_ms\":30garbage}","{\"start_ms\":0,\"end_ms\":01}",
    "{\"start_ms\":0,\"end_ms\":+30}","{\"start_ms\":0,\"end_ms\":.30}",
    "{\"start_ms\":0,\"end_ms\":30.}","{\"start_ms\":0,\"end_ms\":30,}",
    "{\"start_ms\":0,\"start_ms\":1000,\"end_ms\":30000}",
    "{\"start_ms\":0,\"\\u0073tart_ms\":1000,\"end_ms\":30000}",
    "{\"start_ms\":0,\"end_ms\":30000,\"\\u0065nd_ms\":40000}",
    "{\"nested\":{\"start_ms\":0},\"end_ms\":30000}",
    "{\"start_ms\":1000,\"end_ms\":1000}","{\"start_ms\":1000,\"end_ms\":0}",
    "{\"start_ms\":0,\"end_ms\":30000", "{\"start_ms\":0,\"end_ms\":30000}junk",
    "{\"start_ms\":0,\"end_ms\":30000,\"bad\":undefined}"
  };
  for(size_t i=0;i<sizeof invalidos/sizeof *invalidos;i++)json(invalidos[i],0);
  IntroTrecho v[]={ {0,30,INTRO_RESUMO,1},{30,60,INTRO_ABERTURA,1},
    {90,100,INTRO_CREDITOS,1},{105,120,INTRO_CREDITOS,1} };
  IntroAuto s;double fim=-1;introauto_zerar(&s);
  assert(!introauto_decidir(&s,v,4,0,120,0,todos(),0,&fim)); // pausa/buffer/live/seek
  assert(!introauto_decidir(&s,v,4,0,0,0,todos(),1,&fim)); // duracao desconhecida
  assert(!introauto_decidir(&s,v,4,0,120,0,0,1,&fim));
  assert(introauto_decidir(&s,v,4,0,120,0,1u<<INTRO_RESUMO,1,&fim)==INTRO_RESUMO&&fim==30);
  assert(!introauto_decidir(&s,v,4,0,120,0,todos(),1,&fim)); // falhou/atrasou: nao repete
  assert(introauto_decidir(&s,v,4,30,120,0,todos(),1,&fim)==INTRO_ABERTURA&&fim==60);
  assert(introauto_decidir(&s,v,4,91,120,0,1u<<INTRO_CREDITOS,1,&fim)==INTRO_CREDITOS&&fim==100);
  assert(!introauto_decidir(&s,v,4,100,120,0,todos(),1,&fim)); // preserva cena pos-creditos
  assert(introauto_decidir(&s,v,4,119,120,0,todos(),1,&fim)==INTRO_CREDITOS&&fim==120);
  assert(!introauto_decidir(&s,v,4,120,120,0,todos(),1,&fim));
  introauto_zerar(&s);introauto_voltar(&s,80,35);
  assert(!introauto_decidir(&s,v,4,40,120,0,todos(),1,&fim)); // recuo antes de marcadores tardios
  introauto_voltar(&s,70,0);introauto_voltar(&s,60,5);
  assert(!introauto_decidir(&s,v,4,0,120,0,todos(),1,&fim)); // zero mantido
  introauto_zerar(&s);
  assert(introauto_decidir(&s,v,4,40,120,0,todos(),1,&fim)); // sessao nova
  assert(!introauto_decidir(&s,v,4,91,130,0,todos(),1,&fim)); // fonte/duracao diferente
  introauto_zerar(&s);s.fonteIncompativel=1;
  assert(!introauto_decidir(&s,v,4,40,120,0,todos(),1,&fim)); // URL substituida
  IntroTrecho t={0,0,INTRO_CREDITOS,1};assert(!introauto_limites(&t,120,0));
  t=(IntroTrecho){-1,30,INTRO_ABERTURA,1};assert(!introauto_limites(&t,120,0));
  t=(IntroTrecho){0,121,INTRO_ABERTURA,1};assert(!introauto_limites(&t,120,0));
  t=(IntroTrecho){0,30,INTRO_ABERTURA,0};assert(!introauto_limites(&t,120,0));
  t=(IntroTrecho){0,NAN,INTRO_ABERTURA,1};assert(!introauto_limites(&t,120,0));
  t=(IntroTrecho){0,181,INTRO_ABERTURA,1};assert(!introauto_limites(&t,300,0));
  t=(IntroTrecho){1000,1901,INTRO_CREDITOS,1};assert(!introauto_limites(&t,2000,0));
  t=(IntroTrecho){10,30,INTRO_CREDITOS,1};assert(!introauto_limites(&t,120,1));
  puts("introauto: strict JSON, boundaries, controls, one attempt, rewind, source guard passed");
  return 0;
}
