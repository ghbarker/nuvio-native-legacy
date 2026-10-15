#include <pthread.h>
typedef struct em_proxying_queue em_proxying_queue;
em_proxying_queue *em_proxying_queue_create(void);
int emscripten_proxy_sync(em_proxying_queue *, pthread_t, void (*)(void *), void *);
