#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <sys/types.h>
#include <stdint.h>

#define VOL_MAX     10
#define VOL_MIN     0
#define VOL_MIDDLE  5

#define WAV_FILE_BLKID    0x46464952  // "RIFF" (little-endian)
#define WAV_FILE_FMT_ID   0x45564157  // "WAVE"
#define WAV_FMT_BLKID     0x20746D66  // "fmt " (little-endian)
#define WAV_DATA_BLKID    0x61746164  // "data" (little-endian)

#define AUDIO_FILE      "/dev/snd"

typedef uint16_t data_t;

struct wav_header {
    /* WAVE file format declaration block */
    uint32_t file_blkID; // "RIFF"
    uint32_t file_size;
    uint32_t file_fmtID; // "WAVE"

    /* Audio format description block */
    uint32_t fmt_blkID; // "fmt"
    uint32_t blk_size;
    uint16_t audio_fmt;
    uint16_t num_chan;
    uint32_t freq; // In Hertz
    uint32_t bytes_per_sec;
    uint16_t bytes_per_blk;
    uint16_t bits_per_sample;

    /* Data block */
    uint32_t data_blkID; // "data"
    uint32_t data_size; // In bytes
};

struct wav_file {
    struct wav_header wh;
    void *audio_datas;
};

int init_audio();
void clear_audio();

/**
 * @brief write samples to a sink
 * @param buf A pointer to a buffer containing the samples
 * @param count The number of bytes to write
 * @return The number of bytes written
 */
ssize_t write_samples(const void *buf, size_t size);

/**
 * @brief Reads samples from a source
 * @param buf A pointer to a buffer receiving the samples
 * @param count The number of samples to receive
 * @return The actual number of bytes read
 */
ssize_t read_samples(data_t *buf, size_t size);

/**
 * @brief Gets the volume of a sample depending on the volume
 * @param sample The 16-bit audio sample
 * @param volume The volume, in [0, VOL_MAX]
 * @return The adapted sample
 */
int16_t map_volume(int16_t sample, uint8_t volume);


/**
 * @brief Parses the .wav file header
 * @param audio_fd The .wav file descriptor (previously open)
 * @param wh A structure that will contain the header information
 * @return 0 if everything went well, -1 else
 */
int parse_wav_header(int audio_fd, struct wav_header *wh);

/**
 * @brief Writes a .wav file header to the specified file
 * @param audio_fd An opened, writable file descriptor to which the wav
 * header will be written
 * @return 0 if no error occured, otherwise -1
 */
int write_wav_header(int audio_fd);

/* Copie le contenu audio du fichier wav */
/**
 * @brief Copies the data (samples) from a .wav file to a structure
 * @param audio_fd The .wav file descriptor (previously open)
 * @param wf A structure that will contain the raw samples
 * @return 0 if everything went well, -1 else
 */
int copy_wav_data(int audio_fd, const struct wav_file *wf);

/**
 * @brief Writes data samples to a .wav file
 * @param audio_fd An opened, writable file descriptor of the destination file
 * @param wf Information and data to be written to the file
 * @return 0 if no error occured, otherwise -1
 */
int write_wav_data(int audio_fd, const struct wav_file *wf);

/**
 * @brief Appends data samples to a .wav file
 * @param audio_fd An opened, readable and writable file descriptor of the destination file
 * @param samples A buffer containing interleaved audio samples.
 * @param size The size of the buffer containing the samples.
 * @return 0 if no error occured, otherwise -1
 */
int append_wav_data(int audio_fd, const void *samples, size_t size);

#endif // AUDIO_UTILS_H
