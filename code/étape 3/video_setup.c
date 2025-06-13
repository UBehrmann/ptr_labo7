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
#include "de1soc_utils/de1soc_io.h"
#include "commun.h"

void downscale_image(const uint8_t *src, uint8_t *dst, int width, int height, int factor, int components) {
    int new_width = width / factor;
    int new_height = height / factor;

    for (int by = 0; by < new_height; by++) {
        for (int bx = 0; bx < new_width; bx++) {
            int sum[4] = {0}; // Max 4 composants
            int count = 0;

            for (int fy = 0; fy < factor; fy++) {
                for (int fx = 0; fx < factor; fx++) {
                    int x = bx * factor + fx;
                    int y = by * factor + fy;
                    if (x < width && y < height) {
                        int idx = (y * width + x) * components;
                        for (int c = 0; c < components; c++) {
                            sum[c] += src[idx + c];
                        }
                        count++;
                    }
                }
            }

            int dst_idx = (by * new_width + bx) * components;
            for (int c = 0; c < components; c++) {
                dst[dst_idx + c] = sum[c] / count;
            }
        }
    }
}

void upscale_image(const uint8_t *src, uint8_t *dst, int src_width, int src_height, int factor, int components) {
    int dst_width = src_width * factor;

    for (int sy = 0; sy < src_height; sy++) {
        for (int sx = 0; sx < src_width; sx++) {
            int src_index = (sy * src_width + sx) * components;

            for (int fy = 0; fy < factor; fy++) {
                for (int fx = 0; fx < factor; fx++) {
                    int dx = sx * factor + fx;
                    int dy = sy * factor + fy;
                    int dst_index = (dy * dst_width + dx) * components;

                    for (int c = 0; c < components; c++) {
                        dst[dst_index + c] = src[src_index + c];
                    }
                }
            }
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

    // Allocation des buffers pour les différents modes
    int small_w_2 = WIDTH / 2, small_h_2 = HEIGHT / 2, small_px_2 = small_w_2 * small_h_2;
    int small_w_4 = WIDTH / 4, small_h_4 = HEIGHT / 4, small_px_4 = small_w_4 * small_h_4;
    uint8_t *buf_small_2 = malloc(small_px_2 * BYTES_PER_PIXEL);
    uint8_t *buf_gray_2 = malloc(small_px_2);
    uint8_t *buf_conv_2 = malloc(small_px_2);
    uint8_t *buf_rgba_2 = malloc(small_px_2 * 4);
    uint8_t *buf_small_4 = malloc(small_px_4 * BYTES_PER_PIXEL);
    uint8_t *buf_gray_4 = malloc(small_px_4);
    uint8_t *buf_conv_4 = malloc(small_px_4);
    uint8_t *buf_rgba_4 = malloc(small_px_4 * 4);

    dst_image.width = src_image.width = WIDTH;
    dst_image.height = src_image.height = HEIGHT;
    dst_image.components = src_image.components = BYTES_PER_PIXEL;
    src_image.data = (uint8_t *)malloc(WIDTH * HEIGHT * BYTES_PER_PIXEL);
    dst_image.data = (uint8_t *)malloc(WIDTH * HEIGHT * BYTES_PER_PIXEL);

    // Création du timer EVL pour la synchro vidéo
    struct itimerspec value;
    int tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    evl_read_clock(EVL_CLOCK_MONOTONIC, &value.it_value);
    value.it_value.tv_sec += 1;
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = VIDEO_PERIOD_NS;
    evl_set_timer(tmfd, &value, NULL);

    // Passage du thread en temps réel
    param.sched_priority = VIDEO_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (evl_attach_self("EVL video thread") < 0) return NULL;

    FILE *file = fopen(VIDEO_FILENAME, "rb");

    if (!file) {
        printf("Error: Couldn't open raw video file.\n");

    } else {
        // Boucle principale vidéo
        while (*priv->running) {
            fseek(file, 0, SEEK_SET);
            for (int i = 0; i < NB_FRAMES; i++) {

                if (!*priv->running) break;

                fread(src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL, 1, file);

                video_mode_t mode = atomic_load(priv->video_mode);

                uint32_t keys = read_key() & 0x7;

                // Gestion du mode de compensation selon la touche
                if (keys == 0x1 && compensation_mode != MODE_NONE) {
                    compensation_mode = MODE_NONE;
                    evl_printf("[VIDEO_TASK] Compensation mode set to NONE.\n");
                } else if (keys == 0x2 && compensation_mode != MODE_REDUCTION_SCALE) {
                    compensation_mode = MODE_REDUCTION_SCALE;
                    evl_printf("[VIDEO_TASK] Compensation mode set to REDUCTION SCALE.\n");
                } else if (keys == 0x4 && compensation_mode != MODE_REDUCTION_COMPLEXITY) {
                    compensation_mode = MODE_REDUCTION_COMPLEXITY;
                    evl_printf("[VIDEO_TASK] Compensation mode set to REDUCTION COMPLEXITY.\n");
                }
                // Traitement vidéo selon le mode de compensation et le mode vidéo
                switch (compensation_mode) {
                    case MODE_REDUCTION_COMPLEXITY:
                        switch (mode) {
                            case VIDEO_MODE_DEGRADED_2:
                                memcpy(get_video_buffer(), src_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            case VIDEO_MODE_DEGRADED_1:
                                rgba_to_grayscale32(&src_image, &dst_image);
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
                    case MODE_REDUCTION_SCALE:
                        switch (mode) {
                            case VIDEO_MODE_DEGRADED_2: {
                                downscale_image(src_image.data, buf_small_4, WIDTH, HEIGHT, 4, BYTES_PER_PIXEL);
                                struct img_1D_t tmp_img = { .data = buf_small_4, .width = small_w_4, .height = small_h_4, .components = BYTES_PER_PIXEL };
                                rgba_to_grayscale8(&tmp_img, buf_gray_4);
                                convolution_grayscale(buf_gray_4, buf_conv_4, small_w_4, small_h_4);
                                struct img_1D_t tmp_rgba = { .data = buf_rgba_4, .width = small_w_4, .height = small_h_4, .components = 4 };
                                grayscale_to_rgba(buf_conv_4, &tmp_rgba);
                                upscale_image(buf_rgba_4, dst_image.data, small_w_4, small_h_4, 4, BYTES_PER_PIXEL);
                                memcpy(get_video_buffer(), dst_image.data, WIDTH * HEIGHT * BYTES_PER_PIXEL);
                                break;
                            }
                            case VIDEO_MODE_DEGRADED_1: {
                                downscale_image(src_image.data, buf_small_2, WIDTH, HEIGHT, 2, BYTES_PER_PIXEL);
                                struct img_1D_t tmp_img = { .data = buf_small_2, .width = small_w_2, .height = small_h_2, .components = BYTES_PER_PIXEL };
                                rgba_to_grayscale8(&tmp_img, buf_gray_2);
                                convolution_grayscale(buf_gray_2, buf_conv_2, small_w_2, small_h_2);
                                struct img_1D_t tmp_rgba = { .data = buf_rgba_2, .width = small_w_2, .height = small_h_2, .components = 4 };
                                grayscale_to_rgba(buf_conv_2, &tmp_rgba);
                                upscale_image(buf_rgba_2, dst_image.data, small_w_2, small_h_2, 2, BYTES_PER_PIXEL);
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

                // Détection d'overrun du timer
                if (ticks > 1) {
                    evl_printf("[VIDEO_TASK] Timer overrun detected! Overruns: %llu\n", ticks - 1);
                }
            }
        }

        fclose(file);
    }

    // Libération des buffers alloués
    free(buf_small_2); free(buf_gray_2); free(buf_conv_2); free(buf_rgba_2);
    free(buf_small_4); free(buf_gray_4); free(buf_conv_4); free(buf_rgba_4);
    free(src_image.data); free(dst_image.data);
    free(gs_src); free(gs_dst); free(gs_tmp);

    evl_printf("Terminating video task.\n");

    return NULL;
}
