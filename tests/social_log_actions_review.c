/* Independent action review using the builder's inert drawing boundaries. */
#define main social_log_builder_fixture_main
#include "social_log_phone.c"
#undef main

int main(void) {
  nv_layout_w=1080;nv_layout_h=1920;scale=ui=1.5f;
#if defined(TESTE_REACAO)
  for(int i=0;i<3;i++) {
    char id[24];snprintf(id,sizeof id,"tt%d",100+i);
    abrir(id,"movie","Filme com uma recomendacao","",1,5000);c.anim=1;
    reacaoTelefoneDesenhar("O que achou?","","",ROTULO,5000,screenH()-48,1);
    int old=writes,request=requests,key=targets[i+1].b;
    assert(targets[i+1].a==i&&targets[i+1].action==reacaoTelefoneEscolher);
    targets[i+1].action(targets[i+1].a,key);
    assert(!c.aberto&&reacao_estado(id)==VALOR[i]&&writes==old+1);
    int answered=requests;assert(answered==request+1);
    reacaoTelefoneEscolher(i,key);assert(writes==old+1&&requests==answered);
    abrir("tt999","movie","Novo filme","",1,5000);c.anim=1;
    reacaoTelefoneEscolher(i,key);assert(c.aberto&&writes==old+1&&requests==answered);
  }
  /* All message actions use their current step; old reaction targets do not. */
  for(int i=0;i<3;i++) {
    abrir("tt999","movie","Filme","",1,5000);c.anim=1;c.rec=21;strcpy(c.nome,"Ana");
    int oldkey=reacaoTelefoneGeracao*2;
    c.passo=1;reacaoTelefoneDesenhar("Mandar uma mensagem?","","",ROTULO_MSG,5000,screenH()-48,1);
    int before=requests;reacaoTelefoneEscolher(i,oldkey);assert(c.aberto&&!c.escrevendo&&requests==before);
    assert(targets[i+1].a==i&&targets[i+1].b==oldkey+1);
    targets[i+1].action(targets[i+1].a,targets[i+1].b);
    assert(c.escrevendo==(i==1));assert(c.aberto==(i==1));
    assert(requests==before+(i==0));
  }
  puts("social_log_actions_review: all reaction/message decisions, step/reopen/closed guards PASS");
#elif defined(TESTE_REGISTRO)
  aberto=1;area=RG_REDE;pausado=1;segEd=1;foco=3;regTesteTexto="[rede] exemplo\n";nLin=0;semFonte=0;
  toquerol_limpar(&toqueAreas);toqueAreasOffset=0;nTargets=0;rgSeg(1,RP_X+30,RP_Y+54);
  int before=writes,req=requests;
  if(toqueAreas.maximo>0)drag(&toqueAreas,toqueRegistroRolar,0);
  assert(pausado==1&&area==RG_REDE&&foco==3&&writes==before&&requests==req);
  toqueRegArea(RG_SISTEMA,0);assert(area==RG_SISTEMA&&!pausado&&foco==-1&&!toqueRegistro.livre);
  /* A status transition cannot reuse a stored failed-upload Retry target. */
  envio=(AvisosEnvio){.estado=3,.motivo=AVISOS_ENVIO_SERVIDOR,.http=503};envTesteFixo=1;
  registro_envio_abrir();envTelefoneDesenhar(&envio);assert(nTargets==3);
  PonteiroFn retry=targets[1].action;int a=targets[1].a,k=targets[1].b;
  envio.estado=2;retry(a,k);assert(envAberto&&requests==req);
  envio.estado=3;envio.motivo=AVISOS_ENVIO_OFFLINE;retry(a,k);assert(envAberto&&requests==req);
  registro_envio_abrir();retry(a,k);assert(envAberto&&requests==req);
  envTelefoneDesenhar(&envio);assert(nTargets==2);targets[1].action(targets[1].a,targets[1].b);assert(!envAberto&&requests==req);
  /* Closing an in-progress receipt preserves the background upload. */
  envio.estado=1;registro_envio_abrir();envTelefoneDesenhar(&envio);
  int key=targets[1].b;targets[1].action(targets[1].a,key);assert(!envAberto&&requests==req);
  envTelefoneEscolher(0,key);assert(requests==req&&writes==before);
  puts("social_log_actions_review: free filter/pause/resume, receipt transition/reopen/close/retry guards PASS");
#else
#error choose TESTE_REACAO or TESTE_REGISTRO
#endif
}
