#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <time.h>
#include <pthread.h>

#include <evl/evl.h>

#include "de1soc_utils/de1soc_video.h"
#include "de1soc_utils/de1soc_io.h"
#include "de1soc_utils/grayscale.h"
#include "video_setup.h"
#include "load_setup.h"

// Add canary counter and mutex
volatile int canary_counter = 0;
pthread_mutex_t canary_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations for canary and watchdog tasks
void *canary_task(void *cookie);
void *watchdog_task(void *cookie);

#define CANARY_PRIO 60
#define WATCHDOG_PRIO 90

// Timer periods in nanoseconds
#define CANARY_PERIOD_NS 10000000UL   // 10ms
#define WATCHDOG_PERIOD_NS 10000000UL // 100ms

#define WATCHDOG_MISSED_THRESHOLD 10 // Number of missed canary increments before termination

bool running;

// Signal handler used to end the infinite loop
void sigint_handler(int signum)
{
    if (signum != SIGINT) return;
    printf("\nCtrl+C received. Exiting...\n");
    running = false;
}

int main()
{
    int ret;

    signal(SIGINT, sigint_handler);

    mlockall(MCL_CURRENT | MCL_FUTURE);

    running = true;

    //Init EVL
    if(evl_init() < 0)
    {
        perror("Unable to init EVL\n");
        return -1;
    }

    // Ioctl setup
    ret = init_de1soc_io();
    if (ret < 0)
    {
        perror("Could not init the io...\n");
        return ret;
    }

    // Video setup
    ret = init_video();
    if (ret < 0)
    {
        perror("Could not init the video...\n");
        return ret;
    }

    Priv_video_args_t priv_video;
    priv_video.running = &running;
    // Create the video acquisition task
    if (pthread_create(&priv_video.rt_task, NULL, video_task, &priv_video) != 0)
    {
        perror("Error while starting video_function");
        exit(EXIT_FAILURE);
    }
    printf("Launched video acquisition task\n");

    Priv_load_args_t priv_load;
    priv_load.running = &running;
    // Create the load task
    if (pthread_create(&priv_load.rt_task, NULL, load_task, &priv_load) != 0)
    {
        perror("Error while starting load function");
        exit(EXIT_FAILURE);
    }
    printf("Launched load task\n");

    // Canary and Watchdog threads
    pthread_t canary_thread, watchdog_thread;

    // Create canary task
    if (pthread_create(&canary_thread, NULL, canary_task, &running) != 0)
    {
        perror("Error while starting canary task");
        exit(EXIT_FAILURE);
    }
    printf("Launched canary task\n");

    // Create watchdog task
    if (pthread_create(&watchdog_thread, NULL, watchdog_task, &running) != 0)
    {
        perror("Error while starting watchdog task");
        exit(EXIT_FAILURE);
    }
    printf("Launched watchdog task\n");

    pthread_join(priv_video.rt_task, NULL);

    // Wait for canary and watchdog to finish
    pthread_join(canary_thread, NULL);
    pthread_join(watchdog_thread, NULL);

    clear_de1soc_io();
    clear_video();

    munlockall();

    printf("Application has correctly been terminated.\n");

    return EXIT_SUCCESS;
}

// Canary task: increments canary_counter periodically using EVL timer
void *canary_task(void *cookie) {

    bool *running_ptr = (bool *)cookie;
    struct sched_param param;
    uint64_t ticks;

    struct itimerspec value;
    int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
    value.it_value.tv_sec += 1; // start in 1s
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = CANARY_PERIOD_NS;
    evl_set_timer(tmfd, &value, NULL);

    param.sched_priority = CANARY_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    if (evl_attach_self("EVL audio acq thread") < 0) return NULL;

    while (*running_ptr) {

        oob_read(tmfd, &ticks, sizeof(ticks));

        // Critical section to increment canary_counter
        pthread_mutex_lock(&canary_mutex);
        canary_counter++;
        pthread_mutex_unlock(&canary_mutex);

    }

    return NULL;
}

// Watchdog task: checks if canary_counter is incremented using EVL timer
void *watchdog_task(void *cookie) {

    bool *running_ptr = (bool *)cookie;
    struct sched_param param;

    struct itimerspec value;
    int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
    value.it_value.tv_sec += 1; // start in 1s
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = WATCHDOG_PERIOD_NS;
    evl_set_timer(tmfd, &value, NULL);

    param.sched_priority = WATCHDOG_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (evl_attach_self("EVL audio acq thread") < 0) return NULL;

    uint64_t ticks;
    int last_counter = 0;
    int missed_count = 0;
    while (*running_ptr) {

        oob_read(tmfd, &ticks, sizeof(ticks));

        // Critical section to read canary_counter
        pthread_mutex_lock(&canary_mutex);
        int current = canary_counter;
        pthread_mutex_unlock(&canary_mutex);

        // Check if canary_counter has changed
        if (current < (last_counter + 10)) {

            missed_count++;
            evl_printf("[WATCHDOG] Canary not alive! Missed %d/%d\n", missed_count, 10);

            // If canary missed WATCHDOG_MISSED_THRESHOLD times, terminate the application
            if (missed_count >= WATCHDOG_MISSED_THRESHOLD) {

                evl_printf("[WATCHDOG] Canary missed 10 times! Terminating application.\n");
                *running_ptr = false;
                break;
            }
        } else {
            // Canary is alive, reset missed count
            missed_count = 0;
        }

        last_counter = current;
    }
    return NULL;
}
