#ifndef COMMON_H
#define COMMON_H

#include <sys/ioctl.h>

/* Defines related to IOCL of the board */

#define RTDM_CLASS_MISC     223

/* Swtiches IOCLT */
#define IOCTL_READ_SWITCH           _IOR(RTDM_CLASS_MISC, 0, int)

/* Keys IOCLT */
#define IOCTL_READ_KEY  	        _IOR(RTDM_CLASS_MISC, 1, int)

/* Leds IOCLT */
#define IOCTL_READ_LED  	        _IOR(RTDM_CLASS_MISC, 2, int)
#define IOCTL_WRITE_LED  	        _IOW(RTDM_CLASS_MISC, 3, int)

/* Hex IOCLT */
#define IOCTL_READ_HEX0  	        _IOR(RTDM_CLASS_MISC, 4, int)
#define IOCTL_WRITE_HEX0  	        _IOW(RTDM_CLASS_MISC, 5, int)

#define IOCTL_READ_HEX1  	        _IOR(RTDM_CLASS_MISC, 6, int)
#define IOCTL_WRITE_HEX1  	        _IOW(RTDM_CLASS_MISC, 7, int)

#define IOCTL_READ_HEX2  	        _IOR(RTDM_CLASS_MISC, 8, int)
#define IOCTL_WRITE_HEX2  	        _IOW(RTDM_CLASS_MISC, 9, int)

#define IOCTL_READ_HEX3  	        _IOR(RTDM_CLASS_MISC, 10, int)
#define IOCTL_WRITE_HEX3  	        _IOW(RTDM_CLASS_MISC, 11, int)

#define IOCTL_READ_HEX4  	        _IOR(RTDM_CLASS_MISC, 12, int)
#define IOCTL_WRITE_HEX4  	        _IOW(RTDM_CLASS_MISC, 13, int)

#define IOCTL_READ_HEX5  	        _IOR(RTDM_CLASS_MISC, 14, int)
#define IOCTL_WRITE_HEX5  	        _IOW(RTDM_CLASS_MISC, 15, int)

/* GPIO bank 0 IOCLT */
#define IOCTL_READ_GPIO_EN_B0_0     _IOR(RTDM_CLASS_MISC, 16, int)
#define IOCTL_WRITE_GPIO_EN_B0_0    _IOW(RTDM_CLASS_MISC, 17, int)
#define IOCTL_READ_GPIO_EN_B0_1     _IOR(RTDM_CLASS_MISC, 16, int)
#define IOCTL_WRITE_GPIO_EN_B0_1    _IOW(RTDM_CLASS_MISC, 17, int)

#define IOCTL_READ_GPIO_VAL_B0_0    _IOR(RTDM_CLASS_MISC, 18, int)
#define IOCTL_WRITE_GPIO_VAL_B0_0   _IOW(RTDM_CLASS_MISC, 19, int)
#define IOCTL_READ_GPIO_VAL_B0_1    _IOR(RTDM_CLASS_MISC, 20, int)
#define IOCTL_WRITE_GPIO_VAL_B0_1   _IOW(RTDM_CLASS_MISC, 21, int)

/* GPIO bank 1 IOCLT */
#define IOCTL_READ_GPIO_EN_B1_0     _IOR(RTDM_CLASS_MISC, 22, int)
#define IOCTL_WRITE_GPIO_EN_B1_0    _IOW(RTDM_CLASS_MISC, 23, int)
#define IOCTL_READ_GPIO_EN_B1_1     _IOR(RTDM_CLASS_MISC, 24, int)
#define IOCTL_WRITE_GPIO_EN_B1_1    _IOW(RTDM_CLASS_MISC, 25, int)

#define IOCTL_READ_GPIO_VAL_B1_0    _IOR(RTDM_CLASS_MISC, 26, int)
#define IOCTL_WRITE_GPIO_VAL_B1_0   _IOW(RTDM_CLASS_MISC, 27, int)
#define IOCTL_READ_GPIO_VAL_B1_1    _IOR(RTDM_CLASS_MISC, 28, int)
#define IOCTL_WRITE_GPIO_VAL_B1_1   _IOW(RTDM_CLASS_MISC, 29, int)


#endif /* COMMON_H */
