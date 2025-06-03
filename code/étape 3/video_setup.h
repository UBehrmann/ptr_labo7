#ifndef VIDEO_SETUP_H
#define VIDEO_SETUP_H

#include <pthread.h>
#include <stdbool.h>

// Video defines
#define WIDTH 320
#define HEIGHT 240
#define NUM_FRAMES 150
#define NB_FRAMES 300
#define BYTES_PER_PIXEL 4
#define VIDEO_FILENAME "output_video.raw"

typedef struct Priv_video_args
{
    atomic_int* video_mode;
    bool *running;
    pthread_t rt_task;
} Priv_video_args_t;

void *video_task(void *cookie);

#endif // VIDEO_SETUP_H
