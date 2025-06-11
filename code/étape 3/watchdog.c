#include <stdint.h>
#include <unistd.h>
#include <evl/evl.h>
#include <evl/clock.h>
#include <evl/timer.h>

#include "de1soc_utils/de1soc_io.h"
#include "watchdog.h"

// Watchdog task: checks if canary_counter is incremented using EVL timer
void *watchdog_task(void *cookie) {
    priv_watchdog_args_t *priv = (priv_watchdog_args_t *)cookie;
    struct sched_param param;
    uint64_t ticks;

    // Création du timer EVL
    struct itimerspec value;
    int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
    value.it_value.tv_sec += 1; // démarre dans 1 seconde
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = WATCHDOG_PERIOD_NS;
    evl_set_timer(tmfd, &value, NULL);

    // Définir la priorité temps réel
    param.sched_priority = WATCHDOG_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    // Attacher à EVL
    if (evl_attach_self("EVL watchdog thread") < 0)
        return NULL;

    int last_counter = 0;
    double avg_delta = 0.0;
    const double alpha = 0.05; // smoothing factor for moving average

    // Boucle principale
    while (*(priv->running)) {
        oob_read(tmfd, &ticks, sizeof(ticks));

        // Lecture atomique du compteur
        int current = atomic_load(priv->compteur);

        // Calcul du delta et moyenne flottante
        int delta = (last_counter + 10) - current;
        avg_delta = (1.0 - alpha) * avg_delta + alpha * delta;

        if(avg_delta > VIDEO_MODE_DEGRADED_2_TRESHOLD){
                evl_printf("[WATCHDOG] Canary missed too many times! Terminating application.\n");
                *(priv->running) = false;
                break;
        } else if(avg_delta > VIDEO_MODE_DEGRADED_1_TRESHOLD
            && atomic_load(priv->video_mode) != VIDEO_MODE_DEGRADED_2){
            atomic_store(priv->video_mode, VIDEO_MODE_DEGRADED_2);
            evl_printf("[WATCHDOG] video mode = DEGRADED 2.\n");
        } else if(avg_delta >= VIDEO_MODE_NORMAL_TRESHOLD &&
                atomic_load(priv->video_mode) != VIDEO_MODE_DEGRADED_1){
                atomic_store(priv->video_mode, VIDEO_MODE_DEGRADED_1);
                evl_printf("[WATCHDOG] video mode = DEGRADED 1.\n");
        } else if(atomic_load(priv->video_mode) != VIDEO_MODE_NORMAL){
            atomic_store(priv->video_mode, VIDEO_MODE_NORMAL);
            evl_printf("[WATCHDOG] video mode = NORMAL.\n");
        }

        last_counter = current;
    }
    return NULL;
}