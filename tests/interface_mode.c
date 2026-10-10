// A mode is persisted per device, never imported, exported or copied by profile.
#include "../src/ajustes.c"
#include <assert.h>

int main(int argc, char **argv) {
  assert(argc == 2);
  const char *dir = argv[1];
  setenv("NUVIO_DADOS", dir, 1);
  dados_iniciar(dir);
  ajustes_dir(dir);
  assert(AJ_INTERFACE_MODO == AJ_LEG_SYNC_AUTO + 1);
  assert(valorPadrao[AJ_INTERFACE_MODO] == 0);
  assert(!ajustes_interface_mobile() && !layout_modo_mobile());
  assert(!strcmp(CHAVE[AJ_INTERFACE_MODO], "interfaceModoLocal"));
  assert(somenteDesteAparelho(AJ_INTERFACE_MODO) && !dePerfil(AJ_INTERFACE_MODO));
  assert(OPCOES[AJ_INTERFACE_MODO].n == 2);
  assert(!strcmp(V_INTERFACE_MODO[0], "TV"));
  assert(!strcmp(V_INTERFACE_MODO[1], "Mobile"));

  // Switching on the current drawable requires no Android resize or restart.
  layout_tela_definir(1080, 2340);
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  assert(definirValorDireto(AJ_INTERFACE_MODO, 1));
  assert(ajustes_interface_mobile() && layout_modo_mobile());
  assert(NV_TELA_W == 1080.0f && NV_TELA_H == 2340.0f);
  char *saved = dados_ler("ajustes.txt");
  assert(saved && strstr(saved, "interfaceModoLocal 1\n"));
  free(saved);

  // Loading the saved choice restores the canvas before rendering starts.
  valor[AJ_INTERFACE_MODO] = 0;
  layout_modo_definir(0);
  ajustes_dir(dir);
  assert(ajustes_interface_mobile() && layout_modo_mobile());
  assert(NV_TELA_W == 1080.0f && NV_TELA_H == 2340.0f);

  // Account pull/push and profile switching must preserve the local choice.
  const char *account = "{\"interfaceModoLocal\":0}";
  ajustes_aplicar_blob(account);
  assert(ajustes_interface_mobile());
  char *merged = NULL;
  assert(ajustes_mesclar_blob(account, &merged) == 0);
  assert(!merged || !strcmp(merged, account));
  free(merged);
  ajustes_perfil_guardar(1);
  char *profile = dados_ler("ajustes-p1.txt");
  assert(profile && !strstr(profile, "interfaceModoLocal"));
  free(profile);

  assert(definirValorDireto(AJ_INTERFACE_MODO, 0));
  assert(!ajustes_interface_mobile() && !layout_modo_mobile());
  assert(NV_TELA_W == 1920.0f && NV_TELA_H == 1080.0f);
  assert(ajustes_perfil_restaurar(1));
  assert(!ajustes_interface_mobile() && !layout_modo_mobile());
  assert(valor[AJ_FONTE_ORDEM_ADDON] == valorPadrao[AJ_FONTE_ORDEM_ADDON]);
  assert(valor[AJ_LEG_SYNC_AUTO] == valorPadrao[AJ_LEG_SYNC_AUTO]);
  puts("interface_mode: TV default, persistence, switching and account/profile isolation PASS");
  return 0;
}
