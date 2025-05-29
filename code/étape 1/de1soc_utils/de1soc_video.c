#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <evl/evl.h>

#include "de1soc_video.h"
#include "common.h"

static int video_fd;
static void *video_buf;

int init_video()
{
    video_fd = open(VIDEO_FILE, O_RDWR);
    if (video_fd < 0) {
        perror("Failed to open the IOCTL device file\n");
        return -1;
    }

    // Map the device memory to user space
    video_buf = mmap(NULL, VIDEO_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, 0);

    if (video_buf == MAP_FAILED) {
        perror("Failed to map device memory");
        close(video_fd);
        return -1;
    }

    return 0;
}

void clear_video()
{
    close(video_fd);
}

void *get_video_buffer()
{
    return video_buf;
}

int write_frame(uint8_t *frame_data, unsigned size)
{
    ssize_t bytes_written = oob_write(video_fd, frame_data, size);
    if (bytes_written < 0) {
        perror("Failed to write frame data");
        return -1;
    } else if ((size_t)bytes_written != size) {
        fprintf(stderr, "Incomplete write: expected %u bytes, wrote %zd bytes\n", size, bytes_written);
        return -1;
    }
    return 0;
}

