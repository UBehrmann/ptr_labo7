#ifndef LOAD_SETUP_H
#define LOAD_SETUP_H

#include <pthread.h>
#include <stdbool.h>

typedef struct Priv_load_args
{
    bool *running;
    pthread_t rt_task;
} Priv_load_args_t;

void *load_task(void* cookie);

#endif