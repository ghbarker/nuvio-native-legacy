#include SETTINGS_SOURCE
int main(void) {
  printf("styles|%d\n", TXT_NFONTES);
  for (int i=0;i<AJ_N;i++) printf("%d|%s|%d|%d|%s\n",i,CHAVE[i],valor[i],OPCOES[i].tipo,OPCOES[i].rotulo);
  return 0;
}
