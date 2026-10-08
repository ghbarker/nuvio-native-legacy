// O JNI protege este estado com travaRetomada; a posicao vem do player.
#ifndef NV_VIDEO_AUTO_H
#define NV_VIDEO_AUTO_H
#include <string.h>
#include <stdint.h>
enum { VA_PRONTO=1,VA_BUSCAVEL=2,VA_LIVE=4,VA_FIM=8 };
typedef struct {
  unsigned geracao,serial,busca;
  uint32_t relatadoEm,pedidoEm;
  int recebeu,pendente,bloqueado,pausado,flags,posMs;
} VideoAuto;
static inline void videoauto_abrir(VideoAuto *s,unsigned geracao){
  memset(s,0,sizeof *s);s->geracao=geracao;
}
static inline unsigned videoauto_pedir(VideoAuto *s,uint32_t agora,int busca){
  if(++s->serial==0)s->serial++;
  s->recebeu=0;
  if(busca){s->busca=s->serial;s->pendente=1;s->bloqueado=0;s->pedidoEm=agora;}
  return s->serial;
}
static inline void videoauto_relatar(VideoAuto *s,unsigned geracao,unsigned serial,
                                     unsigned busca,int estado,int posMs,int flags,uint32_t agora){
  if(geracao!=s->geracao||serial!=s->serial||posMs<0)return;
  // Um tick anterior ao seek nao tem o serial do pedido; ACK antigo nao vale.
  s->posMs=posMs;s->flags=flags;s->relatadoEm=agora;s->recebeu=1;
  if(busca==s->busca&&estado>0){s->pendente=0;s->bloqueado=0;}
  if(busca==s->busca&&estado<0){s->pendente=0;s->bloqueado=1;}
  if(flags&VA_FIM){s->pendente=0;s->bloqueado=0;}
}
// 1 = tocando VOD buscavel, 2 = fim confirmado, 0 = passivo.
static inline int videoauto_estado(VideoAuto *s,uint32_t agora,double *pos){
  if(s->pendente&&(uint32_t)(agora-s->pedidoEm)>10000u)s->bloqueado=1;
  if(!s->recebeu||(uint32_t)(agora-s->relatadoEm)>1000u||s->pausado||(s->flags&VA_LIVE))return 0;
  if(pos)*pos=s->posMs/1000.0;
  if(s->flags&VA_FIM)return 2;
  if(s->pendente||s->bloqueado)return 0;
  return (s->flags&(VA_PRONTO|VA_BUSCAVEL))==(VA_PRONTO|VA_BUSCAVEL)?1:0;
}
#endif
