#ifndef CANARY_H
#define CANARY_H

#include "commun.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct
{
    atomic_int* compteur;
    bool *running;
    pthread_t rt_task;
} priv_canary_args_t;

void *canary_task(void* cookie);

#endif