#ifndef __SOC_H__
#define __SOC_H__
#include <stdint.h>
#include <stddef.h>

#define MROM_BASE 0x20000000
#define MROM_SIZE (1 << 12) // 4KB

#define UART_BASE 0x10000000
#define UART_SIZE 0x1000

#define SRAM_BASE 0x0f000000
#define SRAM_SIZE (1 << 13) // 8KB

#define SPI_BASE 0x10001000

#define PSRAM_BASE 0x80000000
#define PSRAM_SIZE (1 << 22) // 4MB

#define SDRAM_BASE 0xa0000000
#define SDRAM_SIZE (1 << 27) // 128MB (4 chips: bit + word expand)

#define GPIO_BASE 0x10002000
typedef enum {
    GPIO_LED = 0x0,
    GPIO_SW = 0x4,
    GPIO_SEGS = 0x8,
    GPIO_UNDEF = 0xc,
} GPIO_OFFSET_T;

#define FSBL __attribute__((section(".text.boot"), noinline, used))
#define SSBL __attribute__((section(".sram.loader"), noinline))

#endif
