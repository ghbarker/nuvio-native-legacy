#ifndef NV_BACKUPLOCAL_H
#define NV_BACKUPLOCAL_H
#include <stddef.h>
// Local preferences only. These identifiers are not filesystem paths.
enum {
  BACKUP_AJUSTES_APARELHO, BACKUP_AJUSTES_PERFIL, BACKUP_PLAYER,
  BACKUP_FONTES_COMPARTILHADAS, BACKUP_FONTES_PERFIL, BACKUP_FILEIRAS,
  BACKUP_PARTES_N
};
#define BACKUP_LOCAL_MAX (512u * 1024u)
#define BACKUP_PARTE_MAX 65536u
typedef struct BackupLocal BackupLocal;
BackupLocal *backuplocal_novo(const char *dono,int perfil,long long criado);
void backuplocal_liberar(BackupLocal *b);
// Copies bounded text; binary assets, credentials and arbitrary paths are not parts.
int backuplocal_adicionar(BackupLocal *b,int tipo,const char *texto,size_t n);
const char *backuplocal_parte(const BackupLocal *b,int tipo,size_t *n);
int backuplocal_quantidade(const BackupLocal *b);
long long backuplocal_criado(const BackupLocal *b);
// Allocates output. The caller frees it. CRC32 detects accidental corruption;
// it is not authentication. Module payloads still need their owners' validation.
int backuplocal_exportar(const BackupLocal *b,char **saida,size_t *n);
// Requires the current account and stable server profile ID, never a slot mapping.
// No file writes or runtime changes; failure leaves output NULL.
int backuplocal_ler(const char *texto,size_t n,const char *dono,int perfil,BackupLocal **saida);
#endif
