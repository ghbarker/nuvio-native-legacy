/* Accepted offline account/provider companions; real Settings storage stays live. */
#include "sessao.h"
#include "debrid.h"
#include <assert.h>
#include <string.h>
const char *sessao_usuario(void){return "offline-fixture-account";}
void debrid_conta_iniciar(DebridConta *c){memset(c,0,sizeof *c);}
void debrid_conta_definir(DebridConta *c,const char *provider,const char *key){(void)c;assert(provider);assert(!key || !key[0]);}
void debrid_aplicar_chaves_locais(const DebridConta *c){for(int i=0;i<4;i++)assert(!c->chave[i][0]);}
