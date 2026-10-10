/* Actual Saved social controls and dispatcher. All account/request writes
 * terminate at inert spies; this does not contact a server or alter a profile.
 * The separate pointer fixture checks DOWN/redraw/UP delivery. */
#define NV_SOCIAL_V2_UI 1
#define NV_SAVED_SOCIAL_DOUBLES_ONLY 1
#include "salvospainel_phone_content.c"

static int perfilTeste=1, privacidadeTeste, tecladoTeste;
static int identidadeTeste[3], operacaoTeste[3], aceitouTeste, recusouTeste;
static int alcanceChamadas, contaChamadas[3], tecladoChamadas, ultimoAlcance;
static char ultimoPedido[16];
int perfis_ativo(void) { return perfilTeste; }
int teclado_aberto(void) { return tecladoTeste; }
int recomenda_alcance(void) { return privacidadeTeste; }
void recomenda_responder_alcance(int n) { alcanceChamadas++;ultimoAlcance=n;privacidadeTeste=n; }
int recomenda_aceitar(const char *p) { aceitouTeste++;snprintf(ultimoPedido,sizeof ultimoPedido,"%s",p);return 1; }
int recomenda_recusar(const char *p) { recusouTeste++;snprintf(ultimoPedido,sizeof ultimoPedido,"%s",p);return 1; }
int recomenda_identidade_situacao(void) { return identidadeTeste[0]; }
int recomenda_identidade_op(void) { return operacaoTeste[0]; }
int recomenda_identidade_estado(int i) { return identidadeTeste[i]; }
int recomenda_identidade_op_de(int i) { return operacaoTeste[i]; }
const char *recomenda_identidade_trakt(void) { return "fixture-account"; }
const char *recomenda_identidade_usuario_letterboxd(void) { return "fixture-account"; }
void recomenda_identidade_op_limpar(void) { operacaoTeste[0]=0; }
void recomenda_identidade_op_limpar_de(int i) { operacaoTeste[i]=0; }
int recomenda_identidade_unir(void) { contaChamadas[0]++;return 1; }
int recomenda_identidade_separar(void) { contaChamadas[0]++;return 1; }
int recomenda_identidade_simkl_unir(void) { contaChamadas[1]++;return 1; }
int recomenda_identidade_simkl_separar(void) { contaChamadas[1]++;return 1; }
int recomenda_identidade_letterboxd_separar(void) { contaChamadas[2]++;return 1; }
void teclado_abrir_com(const char *t,const char *d,int max,const char *permitidos,const char *inicio) {
  assert(t&&d&&permitidos&&inicio&&max==30);tecladoChamadas++;
}
int recomenda_item(int i,RecItem *r) { (void)i;(void)r;return 0; }
int recomenda_sugestao(int i,RecSugestao *r) { (void)i;(void)r;return 0; }
int recomenda_pedido(int i,RecPessoa *r) { (void)i;(void)r;return 0; }
int recomenda_contatos(RecContato *r,int max) { (void)r;(void)max;return 0; }
unsigned recresp_revisao(void) { return 1; }
int recresp_assistida(long long i) { (void)i;return 0; }
int recresp_ler(long long i,RecResp *r) { (void)i;(void)r;return 0; }
unsigned socialvis_revisao(void) { return 1; }
int socialvis_amigo_indice(const char *s) { (void)s;return -1; }
int socialvis_ultima_enviada(const char *s,SvEnviada *r) { (void)s;(void)r;return 0; }
void rec_avatar(GfxRect r,const char *u,const char *n,const char *id,float a) {
  (void)u;(void)n;(void)id;gfx_cor(r,.5f,1,1,1,a);
}
const char *extras_caminho_marca_nome(const char *s) { (void)s;return NULL; }
void gfx_anel_fora(GfxRect r,float raio,float folga,float esp,float cr,float cg,float cb,float a) {
  (void)folga;(void)esp;gfx_cor(r,raio,cr,cg,cb,a);
}

static void socialPreparar(int tipo) {
  prepararPainel();aba=SP_ABA_SOCIAL;spPont=1;foco=-1;
  tecladoTeste=0;perfilTeste=1;nPedFeito=0;pedCol=0;alcCol=-1;identConfirma=0;
  aceitouTeste=recusouTeste=alcanceChamadas=tecladoChamadas=0;
  memset(contaChamadas,0,sizeof contaChamadas);memset(toqueSocial,0,sizeof toqueSocial);
  nSocial=1;social[0]=(SPSocial){(unsigned char)tipo,0};nPeds=1;
  memset(peds,0,sizeof peds);snprintf(peds[0].pub,sizeof peds[0].pub,"fixture-person");
  snprintf(peds[0].apelido,sizeof peds[0].apelido,"Ana");
  privacidadeTeste=0;
  identidadeTeste[0]=REC_IDENT_PODE_UNIR;identidadeTeste[1]=identidadeTeste[2]=REC_IDENT_E_PODE;
  memset(operacaoTeste,0,sizeof operacaoTeste);memset(animFoco,0,sizeof animFoco);
}
static void socialDesenhar(int tipo,int recortar) {
  limparDesenho();spPont=1;
  float y=listaTopo()+20;
  if(recortar)y=SP_LISTA_BASE-20-(tipo==SPS_PEDIDO?(SPS_H_SUG-44)*.5f:0);
  gfx_recorte(SP_X,listaTopo(),SP_W,SP_LISTA_BASE-listaTopo());
  if(tipo==SPS_PEDIDO)desenhaPedidoLinha(0,0,0,y,1,5000);
  else if(tipo==SPS_ALCANCE)desenhaAlcanceSeg(0,0,y,SPS_H_ALCANCE,1);
  else for(int i=0;i<nSocial;i++)desenhaConta(i,0,y+(i?SPS_H_CONTAS:0),SPS_H_CONTAS,1,5000);
  gfx_sem_recorte();assert(!foraTeste);
  for(int i=0;i<nBotoesTeste;i++) {
    assert(alvoAtivar[i]==toqueSocialAtivar&&alvoFocar[i]==toqueSocialFocar);
    assert(botoesTeste[i].y>=listaTopo()&&botoesTeste[i].y+botoesTeste[i].h<=SP_LISTA_BASE+.01f);
  }
}
static void socialAtivar(int target) {
  assert(target>=0&&target<nBotoesTeste);alvoAtivar[target](alvoA[target],alvoB[target]);
}
static int pedidoCaso(int coluna) {
  socialPreparar(SPS_PEDIDO);socialDesenhar(SPS_PEDIDO,0);assert(nBotoesTeste==2);
  int b=alvoB[coluna];socialDesenhar(SPS_PEDIDO,0);socialDesenhar(SPS_PEDIDO,0);assert(alvoB[coluna]==b);
  socialAtivar(coluna);assert(aceitouTeste==(coluna==0)&&recusouTeste==(coluna==1));
  assert(!strcmp(ultimoPedido,"fixture-person"));
  toqueSocialAtivar(0,b);assert(aceitouTeste+recusouTeste==1);
  return 1;
}
static void pedidoObsoleto(void) {
  socialPreparar(SPS_PEDIDO);socialDesenhar(SPS_PEDIDO,0);int b=alvoB[1];
  snprintf(peds[0].pub,sizeof peds[0].pub,"other-person");toqueSocialAtivar(0,b);assert(!aceitouTeste&&!recusouTeste);
  socialDesenhar(SPS_PEDIDO,0);assert(alvoB[1]!=b);
  b=alvoB[1];perfilTeste=2;toqueSocialAtivar(0,b);assert(!aceitouTeste&&!recusouTeste);
  perfilTeste=1;pop=POP_SOCIAL;toqueSocialAtivar(0,b);assert(!aceitouTeste&&!recusouTeste);
  pop=0;tecladoTeste=1;toqueSocialAtivar(0,b);assert(!aceitouTeste&&!recusouTeste);
  tecladoTeste=0;aba=SP_ABA_SALVOS;toqueSocialAtivar(0,b);assert(!aceitouTeste&&!recusouTeste);
  aba=SP_ABA_SOCIAL;aberto=0;toqueSocialAtivar(0,b);assert(!aceitouTeste&&!recusouTeste);
}
static void alcanceCaso(int coluna) {
  socialPreparar(SPS_ALCANCE);socialDesenhar(SPS_ALCANCE,0);assert(nBotoesTeste==3);
  int b=alvoB[coluna];socialDesenhar(SPS_ALCANCE,0);assert(alvoB[coluna]==b);
  socialAtivar(coluna);assert(alcCol==coluna&&alcanceChamadas==(coluna!=0));
  if(coluna)assert(ultimoAlcance==coluna);
  toqueSocialAtivar(0,b);assert(alcanceChamadas==(coluna!=0));
}
static void estadoObsoleto(void) {
  socialPreparar(SPS_ALCANCE);socialDesenhar(SPS_ALCANCE,0);int b=alvoB[2];
  privacidadeTeste=1;toqueSocialAtivar(0,b);assert(!alcanceChamadas);
  socialDesenhar(SPS_ALCANCE,0);assert(alvoB[2]!=b);
  socialPreparar(SPS_LETTERBOXD);socialDesenhar(SPS_LETTERBOXD,0);b=alvoB[0];
  operacaoTeste[2]=REC_IDENT_OP_INDO;toqueSocialAtivar(0,b);assert(!tecladoChamadas&&!contaChamadas[2]);
  socialDesenhar(SPS_LETTERBOXD,0);assert(alvoB[0]!=b);
  operacaoTeste[2]=0;identidadeTeste[2]=REC_IDENT_E_LIGADO;
  toqueSocialAtivar(0,b);assert(!identConfirma&&!tecladoChamadas&&!contaChamadas[2]);
}
static void contaCaso(int servico,int ligado) {
  socialPreparar(SPS_IDENT);nSocial=3;
  social[0]=(SPSocial){SPS_IDENT,0};social[1]=(SPSocial){SPS_SIMKL,0};social[2]=(SPSocial){SPS_LETTERBOXD,0};
  if(ligado)identidadeTeste[servico]=servico?REC_IDENT_E_LIGADO:REC_IDENT_UNIDA;
  socialDesenhar(SPS_IDENT,0);assert(nBotoesTeste==3);
  int b=alvoB[servico];socialDesenhar(SPS_IDENT,0);assert(alvoB[servico]==b);
  socialAtivar(servico);
  if(ligado) {
    assert(!contaChamadas[servico]&&identConfirma==social[servico].tipo+1);
    toqueSocialAtivar(servico,b);assert(!contaChamadas[servico]);
    socialDesenhar(SPS_IDENT,0);assert(alvoB[servico]!=b);socialAtivar(servico);assert(contaChamadas[servico]==1);
  } else if(servico<2)assert(contaChamadas[servico]==1);
  else assert(tecladoChamadas==1&&tecladoPara==TK_LETTERBOXD);
}
#ifndef NV_SAVED_SOCIAL_ACTIONS_DOUBLES_ONLY
int main(void) {
  const float telas[][2]={{1080,1920},{1080,2340},{2340,1080},{2520,1080}},scales[]={1,1.2f,1.3f,1.5f};int casos=0;
  for(int t=0;t<4;t++)for(int z=0;z<4;z++) {
    nv_layout_w=telas[t][0];nv_layout_h=telas[t][1];zoom=escala=scales[z];
    for(int c=0;c<2;c++)casos+=pedidoCaso(c);pedidoObsoleto();
    for(int c=0;c<3;c++){alcanceCaso(c);casos++;}estadoObsoleto();
    for(int s=0;s<3;s++)for(int on=0;on<2;on++){contaCaso(s,on);casos++;}
    socialPreparar(SPS_PEDIDO);socialDesenhar(SPS_PEDIDO,1);assert(nBotoesTeste==2);
    socialPreparar(SPS_ALCANCE);socialDesenhar(SPS_ALCANCE,1);assert(nBotoesTeste==0);
    socialPreparar(SPS_IDENT);socialDesenhar(SPS_IDENT,1);assert(nBotoesTeste==1&&botoesTeste[0].h<96);
  }
  nv_layout_w=1920;nv_layout_h=1080;zoom=escala=1;socialPreparar(SPS_PEDIDO);socialDesenhar(SPS_PEDIDO,0);assert(!nBotoesTeste);
  printf("Saved social actual controls: %d phone/scaling/action cases, stale identities, clipped targets, TV path PASS\n",casos);
  return 0;
}
#endif
