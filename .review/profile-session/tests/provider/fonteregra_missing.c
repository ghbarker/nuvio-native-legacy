#include "fonteregra.h"
#include "dados.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(void) {
 char dir[]="/tmp/nuvio-profile-source-rules-XXXXXX", regex[FR_REGEX_MAX+1];
 const char *order[]={"Old plugin","Old addon"};
 assert(mkdtemp(dir));setenv("NUVIO_DADOS",dir,1);dados_iniciar(dir);
 fonteregra_carregar();fonteregra_definir_regex("(?<=release)1080p");
 fonteregra_alternar(0,"Old addon");fonteregra_alternar(1,"Old plugin");fonteregra_ordem_definir(order,2);fonteregra_perfil_guardar(1);
 unsigned before=fonteregra_versao();
 assert(fonteregra_perfil_restaurar(2)==0);
 assert(fonteregra_versao()!=before);assert(fonteregra_n(0)==0&&fonteregra_n(1)==0&&fonteregra_ordem_n()==0);
 fonteregra_regex(regex,sizeof regex);assert(!regex[0]&&fonteregra_regex_estado()==0);
 FonteRegraCfg cfg={FR_ESCOPO_TODAS,0,FR_REGEX_DESLIGADA};
 assert(fonteregra_grupo(&cfg,"Different addon",0,"release1080p")==0);
 // The working file is empty too, so cold reload cannot re-import old rules.
 fonteregra_carregar();assert(fonteregra_n(0)==0&&fonteregra_n(1)==0&&fonteregra_ordem_n()==0);
 fonteregra_regex(regex,sizeof regex);assert(!regex[0]);
 // The source profile's private copy remains intact when it is reopened.
 assert(fonteregra_perfil_restaurar(1)==1);
 assert(fonteregra_n(0)==1&&fonteregra_n(1)==1&&fonteregra_ordem_n()==2);
 fonteregra_regex(regex,sizeof regex);assert(!strcmp(regex,"(?<=release)1080p"));
 assert(fonteregra_regex_estado()==1);
 puts("source rules: first-profile reset, version, restrictions, cold reload, return to prior profile PASS");return 0;
}
