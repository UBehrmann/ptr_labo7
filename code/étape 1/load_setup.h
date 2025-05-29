#ifndef LOAD_SETUP_H
#define LOAD_SETUP_H

#include <pthread.h>
#include <stdbool.h>

#define LOAD_PRIO 70
#define LOAD_PERIOD_MS 100

typedef struct Priv_load_args
{
    bool *running;
    pthread_t rt_task;
} Priv_load_args_t;

void *load_task(void* cookie);

#endif