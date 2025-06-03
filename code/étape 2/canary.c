#include <stdint.h>
#include <unistd.h>
#include <evl/evl.h>
#include <evl/clock.h>
#include <evl/timer.h>

#include "canary.h"

// Canary task: increments canary_counter periodically using EVL timer
void *canary_task(void *cookie) {
    priv_canary_args_t *priv = (priv_canary_args_t *)cookie;
    struct sched_param param;
    uint64_t ticks;

    // Création du timer EVL
    struct itimerspec value;
    int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
    value.it_value.tv_sec += 1; // démarre dans 1 seconde
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = CANARY_PERIOD_NS;
    evl_set_timer(tmfd, &value, NULL);

    // Définir la priorité temps réel
    param.sched_priority = CANARY_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    // Attacher à EVL
    if (evl_attach_self("EVL canary thread") < 0)
        return NULL;

    // Boucle principale
    while (*(priv->running)) {
        oob_read(tmfd, &ticks, sizeof(ticks));
        atomic_fetch_add(priv->compteur, 1);  // Incrémentation atomique
    }

    return NULL;
}