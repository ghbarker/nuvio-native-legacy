#define main original_controller_main
#include "controller_no_native.inc"
#undef main
int main(void) {
  ui=pthread_self();reset();agendar();prontoFonte();
  assert(!iniciaBuffer&&!tokenAtual&&reservas==4&&http==4);
  atomic_fetch_add(&relogio,FA_PREPARADA_VALE_MS-1);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2")&&!renova);
  atomic_fetch_add(&relogio,1);
  assert(proximopreparo_renovar("tt42:1:1","tt42:1:2"));prontoFonte();
  assert(renova==1&&resolucoes==1&&reservas==4&&http==4&&!iniciaBuffer);
  assert(!proximopreparo_renovar("tt42:1:1","tt42:1:2"));
  ProximoPreparoResultado result;
  assert(proximopreparo_consumir("tt42:1:2",&result)&&!result.token&&result.indiceFonte>=0);
  assert(fontefalha_tentativas(result.politica)==4);
  proximopreparo_liberar_resultado(&result);proximopreparo_encerrar();
  puts("PASS source-only: fresh URL reused, exact45s source expiry renews same winner once, no budget reset");
  return 0;
}
