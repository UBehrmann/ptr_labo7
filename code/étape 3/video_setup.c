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

// Helper: downscale grayscale image by factor (nearest neighbor)
static void downscale_grayscale(const uint8_t *src, uint8_t *dst, int width, int height, int factor) {
    int new_w = width / factor;
    int new_h = height / factor;
    for (int y = 0; y < new_h; ++y) {
        for (int x = 0; x < new_w; ++x) {
            dst[y * new_w + x] = src[(y * factor) * width + (x * factor)];
        }
    }
}

// Helper: upscale grayscale image by factor (nearest neighbor)
static void upscale_grayscale(const uint8_t *src, uint8_t *dst, int width, int height, int factor) {
    int src_w = width / factor;
    int src_h = height / factor;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            dst[y * width + x] = src[(y / factor) * src_w + (x / factor)];
        }
    }
}

void *video_task(void *cookie) {
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;
    struct img_1D_t src_image;
    struct img_1D_t dst_image;
    uint64_t ticks;
    struct sched_param param;

    uint8_t *gs_src = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    uint8_t *gs_dst = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    uint8_t *gs_tmp = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t)); // temp for upscaling

    // Precompute downscaled video buffers
    uint8_t *video_buffer_8x = (uint8_t *)malloc(NB_FRAMES * (WIDTH/8) * (HEIGHT/8) * sizeof(uint8_t));
    uint8_t *video_buffer_16x = (uint8_t *)malloc(NB_FRAMES * (WIDTH/16) * (HEIGHT/16) * sizeof(uint8_t));

    dst_image.width = src_image.width = WIDTH;
    dst_image.height = src_image.height = HEIGHT;
    dst_image.components = src_image.components = BYTES_PER_PIXEL;
    src_image.data = (uint8_t *)malloc(WIDTH * HEIGHT * BYTES_PER_PIXEL);
    dst_image.data = (uint8_t *)malloc(WIDTH * HEIGHT * BYTES_PER_PIXEL);

    /* Create timer */
    struct itimerspec value;
    int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
    // Make timer start 1 sec from now (for some headroom)
    value.it_value.tv_sec += 1;
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = VIDEO_PERIOD_NS;

    evl_set_timer(tmfd, &value, NULL);

    // Make thread RT
    param.sched_priority = VIDEO_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (evl_attach_self("EVL video thread") < 0) return NULL;

    FILE *file = fopen(VIDEO_FILENAME, "rb");

    if (!file) {
        printf("Error: Couldn't open raw video file.\n");
    } else {
        // Precompute downscaled buffers
        fseek(file, 0, SEEK_SET);
        for (int i = 0; i < NB_FRAMES; i++) {
            fread(src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL, 1, file);
            rgba_to_grayscale8(&src_image, gs_src);
            // Downscale by 8
            downscale_grayscale(gs_src, video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8), WIDTH, HEIGHT, 8);
            // Downscale by 16
            downscale_grayscale(gs_src, video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16), WIDTH, HEIGHT, 16);
        }

        // Main loop
        while (*priv->running) {
            fseek(file, 0, SEEK_SET);

            for (int i = 0; i < NB_FRAMES; i++) {
                if (!*priv->running) break;

                fread(src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL, 1, file);

                video_mode_t mode = atomic_load(priv->video_mode);
                uint32_t keys = read_key() & 0x3;

                switch (keys) {
                    case 3:
                        //--Méthode 1 : Diminuer les opérations
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
                        break;
                    case 0:
                        //--Méthode 2 : Diminuer le framerate (now: keep image size reduction)
                        switch (mode) {
                            case VIDEO_MODE_DEGRADED_2:
                                convolution_grayscale(
                                    video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16),
                                    video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16),
                                    WIDTH/16, HEIGHT/16
                                );
                                upscale_grayscale(
                                    video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16),
                                    gs_tmp, WIDTH, HEIGHT, 16
                                );
                                grayscale_to_rgba(gs_tmp, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            case VIDEO_MODE_DEGRADED_1:
                                convolution_grayscale(
                                    video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8),
                                    video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8),
                                    WIDTH/8, HEIGHT/8
                                );
                                upscale_grayscale(
                                    video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8),
                                    gs_tmp, WIDTH, HEIGHT, 8
                                );
                                grayscale_to_rgba(gs_tmp, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            case VIDEO_MODE_NORMAL:
                                rgba_to_grayscale8(&src_image, gs_src);
                                convolution_grayscale(gs_src, gs_dst, WIDTH, HEIGHT);
                                grayscale_to_rgba(gs_dst, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                        }
                        break;
                    case 1:
                        //--Méthode 3 : Augmenter la priorité
                        switch (mode) {
                            case VIDEO_MODE_DEGRADED_2:
                                param.sched_priority = VIDEO_PRIO + 30;
                                pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
                                convolution_grayscale(
                                    video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16),
                                    video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16),
                                    WIDTH/16, HEIGHT/16
                                );
                                upscale_grayscale(
                                    video_buffer_16x + i * (WIDTH/16) * (HEIGHT/16),
                                    gs_tmp, WIDTH, HEIGHT, 16
                                );
                                grayscale_to_rgba(gs_tmp, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            case VIDEO_MODE_DEGRADED_1:
                                param.sched_priority = VIDEO_PRIO + 15;
                                pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
                                convolution_grayscale(
                                    video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8),
                                    video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8),
                                    WIDTH/8, HEIGHT/8
                                );
                                upscale_grayscale(
                                    video_buffer_8x + i * (WIDTH/8) * (HEIGHT/8),
                                    gs_tmp, WIDTH, HEIGHT, 8
                                );
                                grayscale_to_rgba(gs_tmp, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            case VIDEO_MODE_NORMAL:
                                param.sched_priority = VIDEO_PRIO;
                                pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
                                rgba_to_grayscale8(&src_image, gs_src);
                                convolution_grayscale(gs_src, gs_dst, WIDTH, HEIGHT);
                                grayscale_to_rgba(gs_dst, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                        }
                        break;
                    default:
                        rgba_to_grayscale8(&src_image, gs_src);
                        convolution_grayscale(gs_src, gs_dst, WIDTH, HEIGHT);
                        grayscale_to_rgba(gs_dst, &dst_image);
                        memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                        break;
                }

                oob_read(tmfd, &ticks, sizeof(ticks));

                // Ajout : détection d'overrun
                if (ticks > 1) {
                    evl_printf("[VIDEO_TASK] Timer overrun detected! Overruns: %llu\n", ticks - 1);
                }
            }
        }

        fclose(file);
    }

    free(src_image.data);
    free(dst_image.data);

    free(gs_src);
    free(gs_dst);
    free(gs_tmp);
    free(video_buffer_8x);
    free(video_buffer_16x);

    evl_printf("Terminating video task.\n");

    return NULL;
}
