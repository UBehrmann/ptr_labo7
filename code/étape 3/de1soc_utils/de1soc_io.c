#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>

#include "de1soc_io.h"

#define DE1SOC_IO(_off_) (*((uint32_t*)(de1soc_io + (_off_))))

static int devmem_fd;
static void *de1soc_io;

int init_de1soc_io()
{
    devmem_fd = open(MEM_FILE, O_RDWR);
    if (devmem_fd < 0) {
        perror("Failed to open the MEM device file\n");
        return -1;
    }

    // Map the device memory to user space
    de1soc_io = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, DE1SOC_IO_BASE);
    if (de1soc_io == MAP_FAILED) {
        perror("Failed to map device memory");
        close(devmem_fd);
        return -1;
    }

    return 0;
}

void clear_de1soc_io()
{
    close(devmem_fd);
    if (munmap(de1soc_io, 4096) == -1) {
        perror("Unmapping");
        exit(EXIT_FAILURE);
    }
}


uint32_t read_key()
{
    return DE1SOC_IO(IO_KEYS);
}

uint32_t read_switch()
{
    return DE1SOC_IO(IO_SWITCH);
}

void write_led(uint32_t value)
{
    DE1SOC_IO(IO_LEDS) = value;
}


uint32_t read_led()
{
    return DE1SOC_IO(IO_LEDS);
}


void write_hex(unsigned hex, uint32_t value)
{
    if (hex > IO_HEX_NB) return;
    DE1SOC_IO(IO_HEX_BASE + hex * REG_SIZE) = value;
}


uint32_t read_hex(unsigned hex)
{
    if (hex > IO_HEX_NB) return 0;
    return DE1SOC_IO(IO_HEX_BASE + hex * REG_SIZE);
}


void write_gpio_en(unsigned bank, Reg_sel_t sel, uint32_t value)
{
    
    DE1SOC_IO(GPIO_BANKS_BASE + bank*GPIO_BANK_SIZE + sel*REG_SIZE) = value;
}


uint32_t read_gpio_en(unsigned bank, Reg_sel_t sel)
{
    return DE1SOC_IO(GPIO_BANKS_BASE + bank*GPIO_BANK_SIZE + sel*REG_SIZE);
}


void write_gpio_val(unsigned bank, Reg_sel_t sel, uint32_t value)
{
    DE1SOC_IO(GPIO_BANKS_BASE + bank*GPIO_BANK_SIZE + GPIO_BANK_VAL_OFF + sel*REG_SIZE) = value;
}


uint32_t read_gpio_val(unsigned bank, Reg_sel_t sel)
{
    return DE1SOC_IO(GPIO_BANKS_BASE + bank*GPIO_BANK_SIZE + GPIO_BANK_VAL_OFF + sel*REG_SIZE);
}
