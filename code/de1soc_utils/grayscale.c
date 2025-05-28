#include "grayscale.h"

#define R_OFF 0
#define G_OFF 1
#define B_OFF 2

void rgba_to_grayscale32(const struct img_1D_t *img, struct img_1D_t *result)
{
    uint8_t *current_data = img->data;
    uint8_t *result_data = result->data;
    unsigned pixel_val;
    int img_size = img->width * img->height * img->components;

    for (int i = 0; i < img_size; i += 4)
    {
        pixel_val = (((int)*(current_data) + *(current_data + 1) + *(current_data + 2)) / 3);
        result_data[i + 0] = pixel_val;
        result_data[i + 1] = pixel_val;
        result_data[i + 2] = pixel_val;

        // Ignore alpha value
        current_data += 4;
    }
}

void rgba_to_grayscale8(const struct img_1D_t *img, uint8_t *result)
{
    uint8_t *current_data = img->data;
    int img_size = img->width * img->height;

    for (int i = 0; i < img_size; i++)
    {

        *result = (((int)*(current_data) + *(current_data + 1) + *(current_data + 2)) / 3);

        current_data += img->components;

        result++;
    }
}

void grayscale_to_rgba(const uint8_t *grayscale_buffer, struct img_1D_t *result)
{
    for (int i = 0; i < result->width * result->height; i++)
    {
        result->data[i * result->components + R_OFF] = grayscale_buffer[i];
        result->data[i * result->components + G_OFF] = grayscale_buffer[i];
        result->data[i * result->components + B_OFF] = grayscale_buffer[i];
    }
}
