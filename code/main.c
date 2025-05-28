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

#define S_IN_NS 1000000000UL

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

    pthread_join(priv_video.rt_task, NULL);

    clear_de1soc_io();
    clear_video();

    munlockall();

    printf("Application has correctly been terminated.\n");

    return EXIT_SUCCESS;
}
