#include "stub_perfisessao.h"
#include "../src/epg.c"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *pasta;

char *dados_caminho(char *out, unsigned cap, const char *nome) {
  int n = snprintf(out, cap, "%s/%s", pasta, nome);
  return n >= 0 && (unsigned)n < cap ? out : NULL;
}
char *dados_ler(const char *nome) {
  char cam[1024]; assert(dados_caminho(cam, sizeof cam, nome));
  FILE *f = fopen(cam, "rb"); if (!f) return NULL;
  assert(!fseek(f, 0, SEEK_END)); long n = ftell(f); assert(n >= 0); rewind(f);
  char *s = malloc((size_t)n + 1); assert(s);
  assert(fread(s, 1, (size_t)n, f) == (size_t)n); s[n] = 0; fclose(f); return s;
}
int dados_gravar_leve(const char *nome, const char *s) {
  char cam[1024]; assert(dados_caminho(cam, sizeof cam, nome));
  FILE *f = fopen(cam, "wb"); assert(f);
  assert(fwrite(s, 1, strlen(s), f) == strlen(s)); return fclose(f) == 0;
}
int dados_apagar(const char *nome) {
  char cam[1024]; assert(dados_caminho(cam, sizeof cam, nome)); return unlink(cam) == 0;
}
void dados_fs_travar(void) {}
void dados_fs_liberar(void) {}
void dados_marcar_sujo(int leve) { (void)leve; }
char *rede_baixar_bin(const char *u, int s, long *n) { (void)u; (void)s; (void)n; return NULL; }
static atomic_int lentas, liberar, chamadas;
char *rede_baixar_bin_medido_controle(const char *u, int s, const char *const *h,
                                    const RedeControle *c, long *n, RedeMedida *m) {
  (void)s;(void)h;atomic_fetch_add(&chamadas,1);memset(m,0,sizeof *m);m->status=200;
  if(strstr(u,"slow")) {
    atomic_store(&lentas,1);
    while(!atomic_load(&liberar) && !*c->cancelado) { struct timespec t={0,1000000};nanosleep(&t,NULL); }
  }
  if(*c->cancelado){m->status=0;return NULL;}
  if(strstr(u,"missing")){m->status=503;return NULL;}
  const char *titulo=strstr(u,"/one")?"Account one":"Account two";
  char xml[1024];
  int k=snprintf(xml,sizeof xml,"<tv><channel id=\"news\"><display-name>News HD</display-name></channel>"
      "<programme channel=\"news\" start=\"20990101000000 +0000\" stop=\"20990101010000 +0000\"><title>%s</title></programme></tv>",titulo);
  *n=k;return strdup(xml);
}
static void esperar(int tipo,unsigned id) {
  for(int i=0;i<5000;i++) {
    epg_passo();EpgContexto *c=contaContexto(tipo,id);
    if(c && c->estado_==EPG_PRONTO && !c->fioVivo_)return;
    struct timespec t={0,1000000};nanosleep(&t,NULL);
  }
  assert(!"schedule worker timed out");
}
static void programa(const char *canal,const char *titulo) {
  int idx=epg_match_canal(canal,"news","News HD");EpgProg p;
  assert(idx>=0 && epg_total(idx)==1);
  assert(epg_agora(idx,4070908801LL,&p));assert(!strcmp(p.titulo,titulo));
  EpgProg ps[2];assert(epg_faixa(idx,4070908800LL,4070912400LL,ps,2)==1);
  assert(!strcmp(ps[0].titulo,titulo));
}
int main(int argc,char **argv) {
  assert(argc==2);pasta=argv[1];assert(!mkdir(pasta,0700));
  epg_abrir_sessao(1);epg_so_provedores(1);
  EpgContaFonte fs[]={ {1,1,"https://epg.invalid/slow/one"}, {1,2,"https://epg.invalid/two"},
                      {2,3,"https://epg.invalid/one"} };
  epg_contas_configurar(fs,3);epg_iniciar();
  esperar(1,2);esperar(2,3);
  assert(atomic_load(&lentas) && epg_canal_estado("xtream:@00000001:7")==EPG_BAIXANDO);
  programa("xtream:@00000002:7","Account two");
  programa("nvm3u:@00000003:1234","Account one");
  atomic_store(&liberar,1);esperar(1,1);
  programa("xtream:@00000001:7","Account one");
  assert(epg_match("News HD")==-1);
  assert(epg_match_canal("xtream:@00000004:7","news","News HD")==-1);
  puts("PASS: duplicate IDs/names and mixed provider types stay isolated; fast schedules publish before a slow account");
  int antigo=epg_match_canal("xtream:@00000001:7","news","News HD");
  epg_contas_configurar(fs+1,2);EpgProg p;assert(!epg_agora(antigo,4070908801LL,&p));
  programa("xtream:@00000002:7","Account two");
  epg_contas_configurar(fs,3);esperar(1,1);
  programa("xtream:@00000001:7","Account one");
  puts("PASS: disabled account handles stop resolving, without disturbing other accounts; re-enable uses its own cache");
  int antes=atomic_load(&chamadas);
  epg_abrir_sessao(1);epg_so_provedores(1);epg_contas_configurar(fs,3);epg_iniciar();
  esperar(1,1);esperar(1,2);esperar(2,3);assert(atomic_load(&chamadas)==antes);
  programa("xtream:@00000001:7","Account one");programa("xtream:@00000002:7","Account two");
  puts("PASS: cold restart loads each account's disk cache independently");
  epg_abrir_sessao(2);epg_so_provedores(1);
  strcpy(fs[0].url,"https://epg.invalid/missing");epg_contas_configurar(fs,1);epg_iniciar();
  for(int i=0;i<5000 && epg_canal_estado("xtream:@00000001:7")!=EPG_FALHOU;i++) {
    epg_passo();struct timespec t={0,1000000};nanosleep(&t,NULL);
  }
  assert(epg_canal_estado("xtream:@00000001:7")==EPG_FALHOU);
  assert(epg_match_canal("xtream:@00000001:7","news","News HD")==-1);
  puts("PASS: another profile or changed URL cannot borrow an existing private schedule");
  epg_abrir_sessao(1);epg_so_provedores(1);atomic_store(&liberar,0);atomic_store(&lentas,0);
  strcpy(fs[0].url,"https://epg.invalid/slow/changed");epg_contas_configurar(fs,1);epg_iniciar();
  for(int i=0;i<1000 && !atomic_load(&lentas);i++){struct timespec t={0,1000000};nanosleep(&t,NULL);}
  assert(atomic_load(&lentas));epg_abrir_sessao(3);
  for(int i=0;i<5000 && atomic_load(&epgFios);i++) {epg_passo();struct timespec t={0,1000000};nanosleep(&t,NULL);}
  assert(!atomic_load(&epgFios));assert(epg_match_canal("xtream:@00000001:7","news","News HD")==-1);
  puts("PASS: switching profiles cancels an in-flight account and discards its unpublished grade");
  return 0;
}
