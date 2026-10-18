#define _POSIX_C_SOURCE 200809L
#include "backupdisco.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
struct BackupDisco {int raiz;unsigned seq;BackupSessaoAtual atual;void *ctx;};
static int nome(const char *dono,int perfil,int tipo,char out[160]) {
  if(tipo==BACKUP_IO_DIARIO) {
    if(dono || perfil)return 0;
    strcpy(out,"backup-restore-journal.txt");return 1;
  }
  // Fixed file names only, bound to the original Nuvio account/profile.
  if(!dono || perfil<1)return 0;
  size_t n=strlen(dono);if(!n || n>80)return 0;
  for(size_t i=0;i<n;i++) {
    unsigned char c=(unsigned char)dono[i];
    if(!((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-'))return 0;
  }
  switch(tipo) {
    case BACKUP_AJUSTES_APARELHO:strcpy(out,"ajustes.txt");break;
    case BACKUP_AJUSTES_PERFIL:snprintf(out,160,"ajustes-p%d.txt",perfil);break;
    case BACKUP_PLAYER:strcpy(out,"player.txt");break;
    case BACKUP_FONTES_COMPARTILHADAS:strcpy(out,"fontefalha.txt");break;
    case BACKUP_FONTES_PERFIL:snprintf(out,160,"fontefalha-%s-p%d.txt",dono,perfil);break;
    // Raw home row keys may contain configured URLs. No file access until
    // their identity adapter is implemented and included in restore planning.
    default:return 0;
  }
  return 1;
}
static int regular(BackupDisco *d,const char *arq,int *existe) {
  struct stat st;
  if(fstatat(d->raiz,arq,&st,AT_SYMLINK_NOFOLLOW)==0){*existe=1;return S_ISREG(st.st_mode);}
  *existe=0;return errno==ENOENT;
}
static int ler(void *ctx,const char *dono,int perfil,int tipo,char **out,size_t *n) {
  if(out)*out=NULL;
  if(n)*n=0;
  BackupDisco *d=ctx;char arq[160];if(!d || !out || !n || !nome(dono,perfil,tipo,arq))return -1;
  int fd=openat(d->raiz,arq,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
  if(fd<0)return errno==ENOENT?0:-1;
  size_t limite=tipo==BACKUP_IO_DIARIO?BACKUP_LOCAL_MAX+128u:BACKUP_PARTE_MAX;
  struct stat st;
  if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<0 || (uintmax_t)st.st_size>limite){close(fd);return -1;}
  size_t tamanho=(size_t)st.st_size,lido=0;char *s=malloc(tamanho+1);
  if(!s){close(fd);return -1;}
  int ok=1;
  while(lido<tamanho) {
    ssize_t k=read(fd,s+lido,tamanho-lido);
    if(k<0 && errno==EINTR)continue;
    if(k<=0){ok=0;break;}lido+=(size_t)k;
  }
  char extra;ssize_t k;
  do{k=read(fd,&extra,1);}while(k<0 && errno==EINTR);
  if(k!=0)ok=0;
  if(close(fd)!=0)ok=0;
  if(!ok){free(s);return -1;}
  s[tamanho]=0;*out=s;*n=tamanho;return 1;
}
static int sincronizar(int fd) {
  int r;do{r=fsync(fd);}while(r<0 && errno==EINTR);return r==0;
}
static int gravar(void *ctx,const char *dono,int perfil,int tipo,const char *s,size_t n) {
  BackupDisco *d=ctx;char arq[160],tmp[80];int existe;
  size_t limite=tipo==BACKUP_IO_DIARIO?BACKUP_LOCAL_MAX+128u:BACKUP_PARTE_MAX;
  if(!d || !s || n>limite || !nome(dono,perfil,tipo,arq) || !regular(d,arq,&existe))return 0;
  int fd=-1;
  for(int i=0;i<100;i++) {
    snprintf(tmp,sizeof tmp,".nuvio-restore-%ld-%u.tmp",(long)getpid(),++d->seq);
    fd=openat(d->raiz,tmp,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd>=0 || errno!=EEXIST)break;
  }
  if(fd<0)return 0;
  int ok=fchmod(fd,0600)==0;size_t feito=0;
  while(ok && feito<n) {
    ssize_t k=write(fd,s+feito,n-feito);
    if(k<0 && errno==EINTR)continue;
    if(k<=0)ok=0;else feito+=(size_t)k;
  }
  if(ok)ok=sincronizar(fd);
  if(close(fd)!=0)ok=0;
  if(ok)ok=renameat(d->raiz,tmp,d->raiz,arq)==0;
  if(ok)ok=sincronizar(d->raiz);
  // A failed directory fsync may follow a successful rename. The transaction
  // engine treats false as uncertain and retains/recovers its private journal.
  if(!ok)unlinkat(d->raiz,tmp,0);
  return ok;
}
static int apagar(void *ctx,const char *dono,int perfil,int tipo) {
  BackupDisco *d=ctx;char arq[160];int existe;
  if(!d || !nome(dono,perfil,tipo,arq) || !regular(d,arq,&existe))return 0;
  if(existe && unlinkat(d->raiz,arq,0)!=0)return 0;
  // ENOENT still needs a barrier: a previous removal may have succeeded just
  // before its directory fsync failed, so absence alone is not durable proof.
  return sincronizar(d->raiz);
}
static int atual(void *ctx,const char *dono,int perfil,uint64_t sessao) {
  BackupDisco *d=ctx;return d && d->atual && d->atual(d->ctx,dono,perfil,sessao);
}
BackupDisco *backupdisco_abrir(const char *dir,BackupSessaoAtual verificar,void *ctx) {
#ifdef __EMSCRIPTEN__
  // IDBFS requires an asynchronous persistent-storage adapter, not fsync().
  (void)dir;(void)verificar;(void)ctx;return NULL;
#else
  if(!dir || !*dir)return NULL;
  int fd=open(dir,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return NULL;
  BackupDisco *d=calloc(1,sizeof *d);if(!d){close(fd);return NULL;}
  d->raiz=fd;d->atual=verificar;d->ctx=ctx;return d;
#endif
}
void backupdisco_fechar(BackupDisco *d){if(d){close(d->raiz);free(d);}}
BackupIO backupdisco_io(BackupDisco *d){return (BackupIO){d,ler,gravar,apagar,atual};}
