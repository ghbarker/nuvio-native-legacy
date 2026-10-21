// Reuse the private in-memory disk fixture, with no real account or network.
#define main simkl_legacy_fixture_main
#define rede_baixar_st legacy_get
#define rede_postar_st legacy_post
#define simkl_credencial_renovada legacy_renewed
#include "simklauth_perfil.c"
#undef main
#undef rede_baixar_st
#undef rede_postar_st
#undef simkl_credencial_renovada
#include <time.h>

static int pinCalls, pollCalls, refreshCalls, renewed, refreshStatus=200, switchProfile;
char *rede_baixar_st(const char *u,int seg,const char *const *cab,int *st) {
  (void)seg;(void)cab;assert(strstr(u,"/oauth/pin?"));pinCalls++;*st=400;
  return strdup("{\"error\":\"unauthorized_client\"}");
}
char *rede_postar_st(const char *u,int seg,const char *const *cab,const char *body,int *st) {
  (void)seg;assert(!strcmp(cab[0],"Content-Type: application/x-www-form-urlencoded"));
  assert(strstr(body,"client_id=cliente-teste"));
  if(strstr(u,"/oauth2/device")) {
    assert(strstr(body,"scope=media%3Aread%20media%3Awrite"));*st=200;
    return strdup("{\"device_code\":\"device-test-only\",\"user_code\":\"ABCD-EFGH\",\"verification_uri\":\"https://simkl.com/pin\",\"verification_uri_complete\":\"https://simkl.com/pin?user_code=ABCD-EFGH\",\"expires_in\":900,\"interval\":5}");
  }
  assert(strstr(u,"/oauth2/token"));
  if(strstr(body,"device_code=")) {
    assert(strstr(body,"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code"));
    pollCalls++;if(pollCalls==1){*st=400;return strdup("{\"error\":\"authorization_pending\"}");}
    *st=200;return strdup("{\"access_token\":\"fake-v2-access\",\"refresh_token\":\"fake-v2-refresh\",\"token_type\":\"bearer\",\"scope\":\"media:read media:write\",\"expires_in\":604800}");
  }
  assert(strstr(body,"grant_type=refresh_token&refresh_token=fake-v2-refresh"));refreshCalls++;usleep(30000);
  if(switchProfile){switchProfile=0;simklauth_carregar_perfil(3);}
  *st=refreshStatus;
  if(refreshStatus==503)return strdup("{}");
  if(refreshStatus==400)return strdup("{\"error\":\"invalid_grant\"}");
  return strdup("{\"access_token\":\"fake-v2-renewed\",\"refresh_token\":\"fake-v2-refresh\",\"token_type\":\"Bearer\",\"scope\":\"media:read media:write\",\"expires_in\":604800}");
}
void simkl_credencial_renovada(const char *a,const char *b) {
  assert(a[0] && !strcmp(b,"fake-v2-renewed"));renewed++;
}
static int v2Saved(void) { return contem("simkl-p2.txt","fake-v2-access") && contem("simkl-p2.txt","expires_at"); }
static void expired(void) {
  dados_gravar("simkl-p2.txt","{\"version\":2,\"access_token\":\"fake-v2-access\",\"refresh_token\":\"fake-v2-refresh\",\"client_id\":\"cliente-teste\",\"expires_at\":1}");
  assert(simklauth_carregar_perfil(2));
}
static void *refresh(void *unused) { (void)unused;assert(simklauth_atualizar());return NULL; }
int main(void) {
  simklauth_esquecer();simklauth_carregar_perfil(2);simklauth_comecar();
  assert(rodaAte(v2Saved,3000));assert(pinCalls==1 && pollCalls==2);
  assert(!strcmp(simklauth_codigo(),"ABCD-EFGH"));
  assert(!strcmp(simklauth_url(),"https://simkl.com/pin?user_code=ABCD-EFGH"));
  assert(contem("simkl-p2.txt","fake-v2-refresh") && !existe("simkl-p1.txt"));
  assert(!simklauth_carregar_perfil(1) && !simklauth_token()[0]);assert(simklauth_carregar_perfil(2));
  expired();pthread_t threads[6];for(int i=0;i<6;i++)assert(!pthread_create(&threads[i],NULL,refresh,NULL));
  for(int i=0;i<6;i++)pthread_join(threads[i],NULL);
  assert(refreshCalls==1 && renewed==1 && !strcmp(simklauth_token(),"fake-v2-renewed"));
  assert(contem("simkl-p2.txt","fake-v2-renewed"));
  expired();refreshStatus=503;assert(!simklauth_atualizar());int before=refreshCalls;
  assert(!simklauth_atualizar() && refreshCalls==before && simklauth_token()[0]);
  assert(contem("simkl-p2.txt","fake-v2-refresh"));
  expired();refreshStatus=200;switchProfile=1;assert(!simklauth_atualizar());
  assert(!simklauth_token()[0] && !existe("simkl-p3.txt"));
  assert(contem("simkl-p2.txt","fake-v2-access"));
  expired();refreshStatus=400;assert(!simklauth_atualizar());
  assert(!simklauth_token()[0] && !existe("simkl-p2.txt"));
  char t[300];assert(simklauth_classificar(400,"{\"error\":\"slow_down\"}",t,sizeof t)==SMK_POLL_LENTO);
  assert(simklauth_classificar(400,"{\"error\":\"expired_token\"}",t,sizeof t)==SMK_POLL_NEGADO);
  puts("PASS: AUTH V2 fallback, code flow, pending/backoff, private persistence, single-owner refresh, transient outage, revoked grant and profile-switch isolation");
}
