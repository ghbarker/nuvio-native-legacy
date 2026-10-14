// Run the actual upload entrypoints with local-only mode enabled.
#include <assert.h>
#include <stdio.h>
static int saves;
#include "avisos.c"
void android_salvar_registro_local(void) { saves++; }
int main(void) {
  assert(!NV_REC_URL[0]);
  envPendenteRede = 1; envioEstado = 0;
  avisos_envio_auto_passo(1000);
  assert(saves == 0 && envPendenteRede == 1 && envioEstado == 0);
  assert(enviarRegistro((void *)&AUTO_ANTERIOR) == NULL);
  assert(enviarRegistro((void *)&ATUAL) == NULL);
  char receipt[32] = "old";
  assert(!avisos_enviar_diagnostico("test", "test report", receipt, sizeof receipt));
  assert(!receipt[0]);
  avisos_enviar_registro_atual(); assert(saves == 1);
  enviarAgora(); assert(saves == 2);
  puts("PASS actual C log worker/automatic retry blocked; diagnostics disabled; manual/current/crash saves locally");
  return 0;
}
