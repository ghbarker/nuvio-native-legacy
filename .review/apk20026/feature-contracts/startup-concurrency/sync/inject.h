/* Transport/thread-creation control only; actual sync.c remains unchanged. */
#include <pthread.h>
int nv_sync_thread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
#define pthread_create nv_sync_thread_create
