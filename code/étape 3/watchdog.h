#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "commun.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct
{
    atomic_int* video_mode;
    atomic_int* compteur;
    bool *running;
    pthread_t rt_task;
    atomic_int *compensation_mode;
} priv_watchdog_args_t;

void *watchdog_task(void* cookie);

#endif