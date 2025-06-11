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

// Helper: downscale RGBA image by factor (nearest neighbor)
static void downscale_rgba(const uint8_t *src, uint8_t *dst, int width, int height, int factor) {
    int dst_w = width / factor;
    int dst_h = height / factor;
    for (int dy = 0; dy < dst_h; ++dy) {
        for (int dx = 0; dx < dst_w; ++dx) {
            // Pick the top-left pixel in the factor x factor block
            int sx = dx * factor;
            int sy = dy * factor;
            int src_idx = (sy * width + sx) * BYTES_PER_PIXEL;
            int dst_idx = (dy * dst_w + dx) * BYTES_PER_PIXEL;
            memcpy(&dst[dst_idx], &src[src_idx], BYTES_PER_PIXEL);
        }
    }
}

// Helper: upscale RGBA image by factor (nearest neighbor)
static void upscale_rgba(const uint8_t *src, uint8_t *dst, int width, int height, int factor) {
    int src_w = width / factor;
    int src_h = height / factor;
    for (int dy = 0; dy < height; ++dy) {
        int sy = dy / factor;
        for (int dx = 0; dx < width; ++dx) {
            int sx = dx / factor;
            int src_idx = (sy * src_w + sx) * BYTES_PER_PIXEL;
            int dst_idx = (dy * width + dx) * BYTES_PER_PIXEL;
            memcpy(&dst[dst_idx], &src[src_idx], BYTES_PER_PIXEL);
        }
    }
}

static void upscale_grayscale(const uint8_t *src, uint8_t *dst, int width, int height, int factor) {
    int src_w = width / factor;
    int src_h = height / factor;
    for (int dy = 0; dy < height; ++dy) {
        int sy = dy / factor;
        for (int dx = 0; dx < width; ++dx) {
            int sx = dx / factor;
            dst[dy * width + dx] = src[sy * src_w + sx];
        }
    }
}

void *video_task(void *cookie) {
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;
    struct img_1D_t src_image;
    struct img_1D_t dst_image;
    uint64_t ticks;
    struct sched_param param;
    int compensation_mode = MODE_NONE;

    uint8_t *gs_src = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    uint8_t *gs_dst = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    uint8_t *gs_tmp = (uint8_t *)malloc(WIDTH * HEIGHT * sizeof(uint8_t)); // temp for upscaling

    // Precompute downscaled video buffers
    uint8_t *video_buffer_2x = (uint8_t *)malloc(NB_FRAMES * (WIDTH/2) * (HEIGHT/2) * BYTES_PER_PIXEL);

    uint8_t *video_buffer_4x = (uint8_t *)malloc(NB_FRAMES * (WIDTH/4) * (HEIGHT/4) * BYTES_PER_PIXEL);

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
            // Downscale by 2 (RGBA)
            downscale_rgba(src_image.data, video_buffer_2x + i * (WIDTH/2) * (HEIGHT/2) * BYTES_PER_PIXEL, WIDTH, HEIGHT, 2);
            // Downscale by 4 (RGBA)
            downscale_rgba(src_image.data, video_buffer_4x + i * (WIDTH/4) * (HEIGHT/4) * BYTES_PER_PIXEL, WIDTH, HEIGHT, 4);
        }

        // Main loop
        while (*priv->running) {
            fseek(file, 0, SEEK_SET);
            

            for (int i = 0; i < NB_FRAMES; i++) {
                if (!*priv->running) break;

                fread(src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL, 1, file);

                video_mode_t mode = atomic_load(priv->video_mode);
                uint32_t keys = read_key() & 0x7;

                // Change compensation mode depending on the key pressed
                if (keys == 0x1 && compensation_mode != MODE_NONE) {
                    compensation_mode = MODE_NONE;
                    evl_printf("[VIDEO_TASK] Compensation mode set to NONE.\n");

                } else if (keys == 0x2 && compensation_mode != MODE_REDUCTION_SCALE) {
                    compensation_mode = MODE_REDUCTION_SCALE;
                    evl_printf("[VIDEO_TASK] Compensation mode set to REDUCTION FRAMERATE.\n");
                } else if (keys == 0x4 && compensation_mode != MODE_REDUCTION_COMPLEXITY) {
                    compensation_mode = MODE_REDUCTION_COMPLEXITY;
                    evl_printf("[VIDEO_TASK] Compensation mode set to REDUCTION COMPLEXITY.\n");
                }

                switch (compensation_mode) {
                    case MODE_REDUCTION_COMPLEXITY:
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
                    case MODE_REDUCTION_SCALE:
                        //--Méthode 2 : Diminuer le framerate (now: keep image size reduction)
                        switch (mode) {
                            case VIDEO_MODE_DEGRADED_2: {
                                struct img_1D_t this_frame;
                                this_frame.data = video_buffer_4x + i * (WIDTH/4) * (HEIGHT/4) * BYTES_PER_PIXEL;
                                this_frame.width = WIDTH / 4;
                                this_frame.height = HEIGHT / 4;
                                this_frame.components = BYTES_PER_PIXEL;

                                rgba_to_grayscale8(&this_frame, gs_tmp);
                                convolution_grayscale(gs_tmp, gs_tmp, WIDTH/4, HEIGHT/4);
                                upscale_grayscale(gs_tmp, gs_dst, WIDTH, HEIGHT, 4);
                                grayscale_to_rgba(gs_dst, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            }
                            case VIDEO_MODE_DEGRADED_1: {
                                struct img_1D_t this_frame;
                                this_frame.data = video_buffer_2x + i * (WIDTH/2) * (HEIGHT/2) * BYTES_PER_PIXEL;
                                this_frame.width = WIDTH / 2;
                                this_frame.height = HEIGHT / 2;
                                this_frame.components = BYTES_PER_PIXEL;

                                rgba_to_grayscale8(&this_frame, gs_tmp);
                                convolution_grayscale(gs_tmp, gs_tmp, WIDTH/2, HEIGHT/2);
                                upscale_grayscale(gs_tmp, gs_dst, WIDTH, HEIGHT, 2);
                                grayscale_to_rgba(gs_dst, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            }
                            case VIDEO_MODE_NORMAL:
                                rgba_to_grayscale8(&src_image, gs_src);
                                convolution_grayscale(gs_src, gs_dst, WIDTH, HEIGHT);
                                grayscale_to_rgba(gs_dst, &dst_image);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                        }
                        break;
                    case MODE_NONE:
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
    free(video_buffer_2x);
    free(video_buffer_4x);

    evl_printf("Terminating video task.\n");

    return NULL;
}
