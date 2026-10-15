#include "perfis.h"
int dados_gravar(const char *n, const char *c) { (void)n; (void)c; return 1; }
void fil_definir_perfil(int p) { (void)p; }
void fontepref_definir_perfil(int p) { (void)p; }
void legmem_definir_perfil(int p) { (void)p; }
void arteesc_definir_perfil(int p) { (void)p; }
void plugins_perfil_mudou(void) {}
void servidores_perfil_trocou(void) {}
const char *sessao_usuario(void) { return "fixture-household"; }
