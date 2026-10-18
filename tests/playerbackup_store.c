#include "../src/player.c"
#include <assert.h>
static int aplicado,sujo;
void video_legenda_estilo(const VideoLegendaEstilo *e){assert(e);aplicado++;}
void dados_marcar_sujo(int leve){assert(!leve);sujo++;}
int main(int argc,char **argv) {
  assert(argc==2);char path[1100];snprintf(path,sizeof path,"%s/player.txt",argv[1]);
  PlayerBackup b={.aspecto=PLR_ASP_CROP,.estilo={.tamanho=150,.cor=2,.fundo=3,.posicao=6,.borda=1,.atrasoMs=-1250,.opacidade=2,.familia=TXT_FAMILIA_INTER,.negrito=1,.corBorda=-1},.tocado=PLR_LEG_COR|PLR_LEG_FONTE},d;
  char *s=NULL,*out=NULL;assert(playerbackup_exportar(&b,&s));
  FILE *f=fopen(path,"w");assert(f);assert(fwrite(s,1,strlen(s),f)==strlen(s));assert(!fclose(f));
  player_dir(argv[1]);player_backup_capturar(&d);
  assert(playerbackup_exportar(&d,&out) && !strcmp(s,out));free(out);
  assert(!sujo && !aplicado); // No double inversion of the saved delay.
  d.estilo.tamanho=155;assert(!player_leg_estilo_definir(&d.estilo,PLR_LEG_COR));assert(!sujo && !aplicado);
  d.estilo.tamanho=180;d.estilo.atrasoMs=250;d.estilo.corBorda=5;
  assert(player_leg_estilo_definir(&d.estilo,PLR_LEG_COR_BORDA));assert(sujo==1 && aplicado==1);
  player_dir(argv[1]);player_backup_capturar(&b);assert(b.estilo.tamanho==180 && b.estilo.atrasoMs==250 && b.estilo.corBorda==5);
  assert(player_leg_estilo_tocado(PLR_LEG_COR_BORDA));
  free(s);puts("PASS: backup payload reloads through the real player store; normal setter validation, persistence, delay sign and border flags preserved");
}
