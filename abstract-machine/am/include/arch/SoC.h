#ifndef __SOC_H__
#define __SOC_H__
#include "stdint.h"
#include "stddef.h"

#define MROM_BASE 0x20000000
#define MROM_SIZE 0x1000

#define UART_BASE 0x10000000
#define UART_SIZE 0x1000

#define SRAM_BASE 0x0f000000
#define SRAM_SIZE 0x00002000

#define SPI_BASE 0x10001000

#define PSRAM_BASE 0x80000000
#define PSRAM_SIZE (1 << 22)

#endif
