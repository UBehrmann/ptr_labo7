#include "convolution.h"

void convolution_grayscale(uint8_t *input, uint8_t *output, int width, int height)
{
    // Sobel operator kernels
    int kernel_x[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
    int kernel_y[] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
    int kernel_size = 3;

    int k_center = kernel_size / 2;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Initialize gradients
            int gradient_x = 0;
            int gradient_y = 0;

            // Apply convolution
            for (int j = 0; j < kernel_size; j++)
            {
                for (int i = 0; i < kernel_size; i++)
                {
                    int x_index = x - k_center + i;
                    int y_index = y - k_center + j;

                    if (x_index >= 0 && x_index < width &&
                        y_index >= 0 && y_index < height)
                    {
                        int index = y_index * width + x_index;
                        gradient_x += input[index] * kernel_x[j * kernel_size + i];
                        gradient_y += input[index] * kernel_y[j * kernel_size + i];
                    }
                }
            }

            // Calculate gradient magnitude
            int gradient_mag = abs(gradient_x) + abs(gradient_y);

            // Threshold the gradient magnitude
            output[y * width + x] = gradient_mag > 125 ? 220 : 50;
        }
    }
}
