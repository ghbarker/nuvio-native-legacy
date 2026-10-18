#ifndef NV_PLAYERBACKUP_H
#define NV_PLAYERBACKUP_H
#include "video.h"
#include "text.h"
#include "legenda.h"
#include <stddef.h>
typedef struct { int aspecto;VideoLegendaEstilo estilo;int tocado; } PlayerBackup;
// Shared by the normal setter and backup validation.
static inline int playerbackup_estilo_valido(const VideoLegendaEstilo *e) {
  return e && e->tamanho>=TXT_LEG_PCT_MIN && e->tamanho<=TXT_LEG_PCT_MAX && !(e->tamanho%10) &&
    e->cor>=0 && e->cor<VIDEO_LEG_NCORES && e->corBorda>=-1 && e->corBorda<VIDEO_LEG_NCORES &&
    e->familia>=0 && e->familia<TXT_FAMILIA_N && e->fundo>=0 && e->fundo<=4 &&
    e->posicao>=0 && e->posicao<=7 && e->borda>=0 && e->borda<=2 &&
    e->opacidade>=0 && e->opacidade<=3 && e->negrito>=0 && e->negrito<=1 &&
    e->atrasoMs>=-LEG_ATRASO_MANUAL_MAX && e->atrasoMs<=LEG_ATRASO_MANUAL_MAX;
}
// Captures preferences only, without playback/session/source information.
void player_backup_capturar(PlayerBackup *out);
int playerbackup_exportar(const PlayerBackup *b,char **saida);
int playerbackup_ler(const char *texto,size_t n,PlayerBackup *saida);
#endif
