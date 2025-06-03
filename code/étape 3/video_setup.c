#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <evl/evl.h>
#include <evl/timer.h>

#include "video_setup.h"
#include "de1soc_utils/de1soc_video.h"
#include "de1soc_utils/image.h"
#include "de1soc_utils/grayscale.h"
#include "de1soc_utils/convolution.h"
#include "commun.h"


void *video_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;
    struct img_1D_t src_image;
    struct img_1D_t dst_image;
    uint64_t ticks;
    struct sched_param param;

    uint8_t *gs_src = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    uint8_t *gs_dst = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t));

    dst_image.width = src_image.width = WIDTH;
    dst_image.height = src_image.height = HEIGHT;
    dst_image.components = src_image.components = BYTES_PER_PIXEL;
    src_image.data = (uint8_t *)malloc(WIDTH * HEIGHT * BYTES_PER_PIXEL);
    dst_image.data = (uint8_t *)malloc(WIDTH * HEIGHT * BYTES_PER_PIXEL);

    /* Create timer */
    struct itimerspec value;
	int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
	evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
	//Make timer start 1 sec from now (for some headroom)
    value.it_value.tv_sec += 1;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = VIDEO_PERIOD_NS;

	evl_set_timer(tmfd, &value, NULL);

    // Make thread RT
    param.sched_priority = VIDEO_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if(evl_attach_self("EVL video thread") < 0) return NULL;

    FILE *file = fopen(VIDEO_FILENAME, "rb");

    if (!file) {
        printf("Error: Couldn't open raw video file.\n");
    } else {
        // Loop that reads a file with raw data
        while (*priv->running){

            // Read each frame from the file
            for (int i = 0; i < NB_FRAMES; i++) {

                if (!*priv->running)
                    break;

                /* CONVOLUTION DISPLAY */
                fread(src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL, 1, file);

                video_mode_t mode = atomic_load(priv->video_mode);
                switch (mode) {
                    case VIDEO_MODE_DEGRADED_2:
                        // fall through
                        memcpy(get_video_buffer(), src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                        break;
                    case VIDEO_MODE_DEGRADED_1:
                        // Skip convolution
                        rgba_to_grayscale32(&src_image, &dst_image);
                        memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                        break;
                    case VIDEO_MODE_NORMAL:
                        // Full processing
                        rgba_to_grayscale8(&src_image, gs_src);
                        convolution_grayscale(gs_src, gs_dst, WIDTH, HEIGHT);
                        grayscale_to_rgba(gs_dst, &dst_image);
                        memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                        break;
                }
                oob_read(tmfd, &ticks, sizeof(ticks));

                // Ajout : détection d'overrun
                if (ticks > 1) {
                    printf("[VIDEO_TASK] Timer overrun detected! Overruns: %llu\n", ticks-1);
                }
            }
        }

        fclose(file);
    }

    free(src_image.data);
    free(dst_image.data);
    
    free(gs_src);
    free(gs_dst);

    evl_printf("Terminating video task.\n");

    return NULL;
}
