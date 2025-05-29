#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

struct img_1D_t
{
    int width;
    int height;
    int components;

    uint8_t *data;
};

#endif