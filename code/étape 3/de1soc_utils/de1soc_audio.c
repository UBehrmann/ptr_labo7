#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <evl/evl.h>

#include "de1soc_audio.h"

static int snd_fd;

int init_audio()
{
    snd_fd = open(AUDIO_FILE, O_RDWR);

    if (snd_fd < 0) {
        perror("Failed to open the snd device file\n");
    }

    return 0;
}

void clear_audio()
{
    close(snd_fd);
}

ssize_t write_samples(const void *buf, size_t count)
{
    return oob_write(snd_fd, buf, count);
}

ssize_t read_samples(data_t *buf, size_t count)
{
    return oob_read(snd_fd, buf, count);
}

int16_t map_volume(int16_t sample, uint8_t volume)
{
    uint8_t shift = VOL_MAX - volume;
    return sample >> shift;
}

int parse_wav_header(int audio_fd, struct wav_header *wh)
{
    ssize_t header_size = sizeof(struct wav_header);
    ssize_t read_ret;

    /* Reposition file offset to start of file */
    lseek(audio_fd, 0, SEEK_SET);

    read_ret = read(audio_fd, wh, header_size);
    if (read_ret < header_size) {
        fprintf(stderr, "Couldn't read whole wav header.\n");
        return -1;
    }

    if (wh->file_blkID != WAV_FILE_BLKID) {
        fprintf(stderr, "[%s] Wrong file block ID. Found : 0x%X\n", __func__, wh->file_blkID);
        return -1;
    }

    if (wh->fmt_blkID != WAV_FMT_BLKID) {
        fprintf(stderr, "[%s] Wrong audio format block ID\n", __func__);
        return -1;
    }

    if (wh->data_blkID != WAV_DATA_BLKID) {
        fprintf(stderr, "[%s] Wrong data block ID\n", __func__);
        return -1;
    }

    return 0;
}

int write_wav_header(int audio_fd)
{
    ssize_t header_size = sizeof(struct wav_header);
    ssize_t write_ret;

    struct wav_header wh;
    wh.file_blkID = WAV_FILE_BLKID;
    wh.file_fmtID = WAV_FILE_FMT_ID;

    wh.fmt_blkID = WAV_FMT_BLKID;
    wh.blk_size = 16;
    wh.audio_fmt = 1;
    wh.num_chan = 2;
    wh.freq = 48000;
    wh.bytes_per_sec = 192000;
    wh.bytes_per_blk = 4;
    wh.bits_per_sample = 16;

    wh.data_blkID = WAV_DATA_BLKID;
    wh.data_size = 0;


    /* Reposition file offset to start of file */
    lseek(audio_fd, 0, SEEK_SET);

    write_ret = write(audio_fd, &wh, header_size);
    if (write_ret < 0) {
        perror("Writing file header");
        return -1;
    }
    else if (write_ret < header_size) {
        fprintf(stderr, "Couldn't write whole wav header");
        return -1;
    }

    return 0;
}

int copy_wav_data(int audio_fd, const struct wav_file *wf)
{
    size_t to_read = wf->wh.data_size;
    ssize_t read_ret;
    void *audio_datas_p = wf->audio_datas;

    lseek(audio_fd, 44, SEEK_SET);
    do {
        read_ret = read(audio_fd, audio_datas_p, to_read);
        if (read_ret < 0) {
            perror("Reading audio data from wav file");
            return -1;
        }
        else if (read_ret == 0 && to_read > 0) {
            fprintf(stderr, "Reached EOF but data remains to be read\n");
            return -1;
        }

        to_read -= read_ret;
        audio_datas_p += read_ret;

    } while (to_read);

    return 0;
}

int write_wav_data(int audio_fd, const struct wav_file *wf)
{
    size_t to_write = wf->wh.data_size;
    ssize_t write_ret;
    void *audio_datas_p = wf->audio_datas;

    lseek(audio_fd, 44, SEEK_SET);
    do {
        write_ret = write(audio_fd, audio_datas_p, to_write);
        if (write_ret < 0) {
            perror("Writing audio data to a wav file");
            return -1;
        }
        else if (write_ret == 0 && to_write > 0) {
            fprintf(stderr, "Couldn't write to a wav file");
            return -1;
        }

        to_write -= write_ret;
        audio_datas_p += write_ret;

    } while (to_write);

    return 0;
}

int append_wav_data(int audio_fd, const void *samples, size_t size)
{
    uint32_t datasize;
    ssize_t read_ret, write_ret;
    size_t to_write;
    void *samples_p;

    /* Get end of data offset */
    lseek(audio_fd, 40, SEEK_SET);
    read_ret = read(audio_fd, &datasize, sizeof(datasize));
    if (read_ret < 0) {
        fprintf(stderr, "[%s] Error while reading wav file : %s\n", __FUNCTION__, strerror(-read_ret));
        return -1;
    }

    /* Append datas */
    lseek(audio_fd, 44+datasize, SEEK_SET);
    to_write = size;
    samples_p = (void*) samples;

    do {
        write_ret = write(audio_fd, samples_p, to_write);
        if (write_ret < 0) {
            fprintf(stderr, "[%s] Error while writing wav file : %s\n", __FUNCTION__, strerror(-write_ret));
            return -1;
        }

        to_write -= write_ret;
        samples_p += write_ret;
    } while (to_write);

    /* Update data size */
    datasize += size;
    lseek(audio_fd, 40, SEEK_SET);
    write_ret = write(audio_fd, &datasize, sizeof(datasize));
    if (write_ret < 0) {
        fprintf(stderr, "[%s] Error while updating datasize : %s\n", __FUNCTION__, strerror(-write_ret));
        return -1;
    }

    return 0;
}
