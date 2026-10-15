#include SETTINGS_SOURCE
int main(void) {
  printf("styles|%d\n", TXT_NFONTES);
  for (int i=0;i<AJ_N;i++)
    printf("%d|%s|%d|%d|%s|%d|%d|%d|%d\n", i, CHAVE[i], valor[i], OPCOES[i].tipo,
           OPCOES[i].rotulo, OPCOES[i].n, OPCOES[i].min, OPCOES[i].max, OPCOES[i].passo);
  return 0;
}
