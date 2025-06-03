#ifndef VIDEO_UTILS_H
#define VIDEO_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#define VIDEO_FILE      "/dev/de1_video"
#define VIDEO_BUF_SIZE  0x4B000

/**
 * @brief map the buffer address and open the char device
 * @return 0 on success
 */
int init_video();

/**
 * @brief free everything that was setup during the init
 */
void clear_video();

/**
 * @brief get the buffer used to display a frame on the VGA
 * @return address of the mapped buffer
 */
void* get_video_buffer();

/**
 * @brief write a frame with copy (safe but slow)
 * @param frame_data buffer containing RGB data
 * @param size The size of the frame
 * @return nb bytes wrote
 */
int write_frame(uint8_t *frame_data, unsigned size);

#endif // VIDEO_UTILS_H
