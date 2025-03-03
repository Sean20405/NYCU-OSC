#ifndef GPIO_H
#define GPIO_H

#define MMIO_BASE       0x3f000000

#define GPFSEL1         ((unsigned int*)(MMIO_BASE + 0x00200004))
#define GPPUD           ((unsigned int*)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0       ((unsigned int*)(MMIO_BASE + 0x00200098))

#endif /* GPIO_H */
