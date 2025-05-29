#ifndef GRAYSCALE_H
#define GRAYSCALE_H

#include "image.h"

void rgba_to_grayscale32(const struct img_1D_t *img, struct img_1D_t *result);
void rgba_to_grayscale8(const struct img_1D_t *img, uint8_t *result);
void grayscale_to_rgba(const uint8_t *grayscale_buffer, struct img_1D_t *result);

#endif // GRAYSCALE_H
