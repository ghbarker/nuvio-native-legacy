/* Independent review of the drawn phone decisions and changing result states. */
#define main diagnostic_builder_fixture_main
#include "livetvdiag_phone.c"
#undef main

int main(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}};
  const float escalas[]={1,1.2f,1.3f,1.5f};
  int paginas=0,acoes=0;
  for(int t=0;t<4;t++)for(int z=0;z<4;z++)for(int caso=0;caso<5;caso++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];zoom=escalas[z];escala=1;
    for(int st=E_PARADO;st<=E_PRONTO;st++) {
      dados();L.estado=st;L.pfVivo=0;L.pfDesde=4000;
      if(caso==0) {L.n=0;L.xtConfig=L.contaLida=L.redeMedida=0;}
      if(caso==1) {L.conta.valido=0;L.conta.http=503;L.rec.confianca=0;L.redeMedida=0;L.recModo=-1;}
      if(caso==2) {L.conta.expira=1760000000;L.conta.temM3u8=L.conta.temTs=1;L.rec.formato=2;L.rec.espera=2;L.rec.dezBits=0;L.rec.semDecoder=0;}
      if(caso==3) {for(int i=0;i<L.n;i++){L.it[i].f[F_HLS].tocou=1;L.it[i].f[F_HLS].testou=1;}L.tocouModo[M_A]=1;L.rec.formato=0;L.recModo=-1;}
      if(caso==4) {L.sair=1;}
      int escolha[B_N],n=botoes(escolha);
      livetvdiag_desenhar(5000);paginas++;
      assert(escala==1 && ltdPhone.corpo.h>0 && ltdPhone.corpo.w>0);
      if(L.sair) {assert(nBotoesTeste==0);continue;}
      assert(nBotoesTeste==1+(n?n:1));
      for(int i=0;i<n;i++) {
        assert(alvoA[i+1]==i && alvoB[i+1]==escolha[i]+1);
        int antesAplica=applies,antesEnvia=sends,antesQuery=queries;
        if(escolha[i]==B_DENOVO) continue; /* rerun separately after saved decisions */
        ltdPhoneAcao(alvoA[i+1],alvoB[i+1]);acoes++;
        assert(applies==antesAplica+(escolha[i]==B_APLICAR));
        assert(sends==antesEnvia+(escolha[i]==B_ENVIAR));
        assert(queries==antesQuery);
      }
      /* Every non-result page has the drawn Cancel decision. It only exits. */
      if(!n) {
        assert(alvoB[1]==0);
        int antesAplica=applies,antesEnvia=sends,antesQuery=queries;
        ltdPhoneAcao(alvoA[1],alvoB[1]);acoes++;
        assert(L.sair && applies==antesAplica && sends==antesEnvia && queries==antesQuery);
      }
    }
  }
  /* A stored finished-page action is rejected after the state changes. */
  dados();livetvdiag_desenhar(5000);
  int salvoA=alvoA[1],salvoB=alvoB[1],antes=applies;
  L.estado=E_REDE;ltdPhoneAcao(salvoA,salvoB);assert(applies==antes);
  L.estado=E_PRONTO;L.sair=1;ltdPhoneAcao(salvoA,salvoB);assert(applies==antes);
  /* Retry uses the existing no-channel branch and resets the gesture state. */
  dados();ltdPhone.offset=500;int q=queries;ltdPhoneAcao(0,B_DENOVO+1);
  assert(queries==q+1 && L.n==0 && L.estado==E_PRONTO && ltdPhone.offset==0);
  printf("livetvdiag_actions_review: %d actual conditional pages, %d drawn decisions, cancel/stale/retry guards PASS\n",paginas,acoes);
}
