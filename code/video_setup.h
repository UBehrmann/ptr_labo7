#ifndef VIDEO_SETUP_H
#define VIDEO_SETUP_H

#include <pthread.h>
#include <stdbool.h>

#define VIDEO_ACK_TASK_PRIORITY 50

// Video defines
#define WIDTH 320
#define HEIGHT 240
#define NUM_FRAMES 150
#define FRAMERATE 15
#define NB_FRAMES 300
#define BYTES_PER_PIXEL 4
#define VIDEO_FILENAME "output_video.raw"

typedef struct Priv_video_args
{
    bool *running;
    pthread_t rt_task;
} Priv_video_args_t;

void *video_task(void *cookie);

#endif // VIDEO_SETUP_H
