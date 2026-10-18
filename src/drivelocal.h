#ifndef NV_DRIVELOCAL_H
#define NV_DRIVELOCAL_H
#include "rede.h"
#include "backuplocal.h"
#define DRIVE_BACKUPS_PAGINA 50
typedef struct {char id[200],nome[128];long long criadoMs;size_t bytes;} DriveBackup;
typedef struct {DriveBackup arquivo[DRIVE_BACKUPS_PAGINA];int n;char proxima[4096];} DrivePagina;
typedef struct {
  const char *token,*dono;int perfil;RedeJob *job;
  // Optional deterministic transport for tests. Default uses rede_pedir.
  int (*pedir)(void *ctx,const RedePedido *,RedeResposta *);
  void *ctx;
} DriveLocal;
enum {
  DRIVE_LOCAL_OK=1,DRIVE_LOCAL_INVALIDO=-1,DRIVE_LOCAL_REDE=-2,
  DRIVE_LOCAL_AUTORIZAR=-3,DRIVE_LOCAL_NEGADO=-4,DRIVE_LOCAL_REPETIR=-5,
  DRIVE_LOCAL_RESPOSTA=-6,DRIVE_LOCAL_CANCELADO=-7,DRIVE_LOCAL_AUSENTE=-8
};
// Blocking worker-only operations. Tokens stay in the Authorization header;
// no credentials or raw provider URLs belong in Drive metadata. No auto POST
// retry: a lost response may already have created a backup.
int drivelocal_listar(const DriveLocal *,const char *pagina,DrivePagina *saida);
int drivelocal_enviar(const DriveLocal *,const char *snapshot,size_t n,char id[200]);
// Download checks the envelope against the CURRENT authenticated Nuvio owner
// and profile. The restore planner must still validate every module payload.
int drivelocal_baixar(const DriveLocal *,const char *id,BackupLocal **saida);
#endif
