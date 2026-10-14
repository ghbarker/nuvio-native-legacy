#ifndef LIVE_TV_FIXTURE_SUPPORT_H
#define LIVE_TV_FIXTURE_SUPPORT_H
#include "perfis.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
void fixture_init(void);
void fixture_cleanup(void);
const char *fixture_dir(void);
#endif
