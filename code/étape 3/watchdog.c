#include <stdint.h>
#include <unistd.h>
#include <evl/evl.h>
#include <evl/clock.h>
#include <evl/timer.h>

#include "de1soc_utils/de1soc_io.h"
#include "watchdog.h"

#define RECOVERY_CYCLES 5
#define RECOVERY_CYCLES_DEG2_TO_DEG1 15

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
    const double alpha = 0.1; // facteur de lissage pour la moyenne mobile
    int recovery_counter = 0; // compteur pour la récupération de l'état
    int state = atomic_load(priv->video_mode); // état courant du mode vidéo

    // Boucle principale du watchdog
    while (*(priv->running)) {
        oob_read(tmfd, &ticks, sizeof(ticks));
        int current = atomic_load(priv->compteur);
        int delta = (last_counter + 10) - current;
        avg_delta = (1.0 - alpha) * avg_delta + alpha * delta;

        // Arrêt d'urgence si le système est trop en retard
        if (avg_delta > VIDEO_MODE_DEGRADED_2_TRESHOLD) {
            evl_printf("[WATCHDOG] Canary missed too many times! Terminating application.\n");
            *(priv->running) = false;
            break;
        }

        // Gestion des transitions d'états du mode vidéo
        switch (state) {
            case VIDEO_MODE_NORMAL:
                // Passage immédiat à DEGRADED 1 si surcharge détectée
                if (avg_delta > VIDEO_MODE_NORMAL_TRESHOLD) {
                    state = VIDEO_MODE_DEGRADED_1;
                    atomic_store(priv->video_mode, state);
                    evl_printf("[WATCHDOG] video mode = DEGRADED 1.\n");
                    recovery_counter = 0;
                }
                break;
            case VIDEO_MODE_DEGRADED_1:
                // Passage immédiat à DEGRADED 2 si surcharge plus forte
                if (avg_delta > VIDEO_MODE_DEGRADED_1_TRESHOLD) {
                    state = VIDEO_MODE_DEGRADED_2;
                    atomic_store(priv->video_mode, state);
                    evl_printf("[WATCHDOG] video mode = DEGRADED 2.\n");
                    recovery_counter = 0;
                // Retour à NORMAL après plusieurs cycles sous le seuil
                } else if (avg_delta <= VIDEO_MODE_NORMAL_TRESHOLD) {
                    recovery_counter++;
                    if (recovery_counter >= RECOVERY_CYCLES) {
                        state = VIDEO_MODE_NORMAL;
                        atomic_store(priv->video_mode, state);
                        evl_printf("[WATCHDOG] video mode = NORMAL.\n");
                        recovery_counter = 0;
                    }
                } else {
                    recovery_counter = 0;
                }
                break;
            case VIDEO_MODE_DEGRADED_2:
                // Retour à DEGRADED 1 après plusieurs cycles sous le seuil
                if (avg_delta <= VIDEO_MODE_DEGRADED_1_TRESHOLD) {
                    recovery_counter++;
                    if (recovery_counter >= RECOVERY_CYCLES_DEG2_TO_DEG1) {
                        state = VIDEO_MODE_DEGRADED_1;
                        atomic_store(priv->video_mode, state);
                        evl_printf("[WATCHDOG] video mode = DEGRADED 1.\n");
                        recovery_counter = 0;
                    }
                } else {
                    recovery_counter = 0;
                }
                break;
        }
        last_counter = current; // Mise à jour du compteur précédent
    }
    return NULL;
}