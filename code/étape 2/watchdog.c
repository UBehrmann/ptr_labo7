#include <stdint.h>
#include <unistd.h>
#include <evl/evl.h>
#include <evl/clock.h>
#include <evl/timer.h>

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
    int missed_count = 0;
    // Boucle principale
    while (*(priv->running)) {
        oob_read(tmfd, &ticks, sizeof(ticks));

        // Lecture atomique du compteur
        int current = atomic_load(priv->compteur);

        // Check si le compteur a changé
        if (current < (last_counter + 10)) {
            missed_count++;
            evl_printf("[WATCHDOG] Canary not alive! Missed %d/%d\n", missed_count, WATCHDOG_MISSED_THRESHOLD);
            evl_printf("[WATCHDOG] current_counter=%d last_counter=/%d\n", current, last_counter);

            if (missed_count >= WATCHDOG_MISSED_THRESHOLD) {
                evl_printf("[WATCHDOG] Canary missed too many times! Terminating application.\n");
                *(priv->running) = false;
                break;
            }
        } else {
            missed_count = 0;
        }

        last_counter = current;
    }
    return NULL;
}