#include "arch/SoC.h"
#define SPI_RX0     0x00u
#define SPI_TX0     0x00u
#define SPI_RX1     0x04u
#define SPI_TX1     0x04u
#define SPI_CTRL    0x10u
#define SPI_DIVIDER 0x14u
#define SPI_SS      0x18u

#define SPI_CTRL_CHAR_LEN(n) ((n) & 0x7fu)
#define SPI_CTRL_GO          (1u << 8)
#define SPI_CTRL_RX_NEG      (1u << 9)
#define SPI_CTRL_TX_NEG      (1u << 10)
#define SPI_CTRL_LSB         (1u << 11)
#define SPI_CTRL_IE          (1u << 12)
#define SPI_CTRL_ASS         (1u << 13)     

#define SPI_SS_BITREV        (1u << 7) /* slave #7 */
#define SPI_SS_FLASH        (1u << 0) /* slave #0 */

#define FLASH_CFG (SPI_CTRL_ASS | SPI_CTRL_CHAR_LEN(64))

uint32_t spi_read(uint32_t off) {
  return *(volatile uint32_t *)(SPI_BASE + off);
}

void spi_write(uint32_t off, uint32_t val) {
  *(volatile uint32_t *)(SPI_BASE + off) = val;
}


void spi_init(uint32_t cfg) {
  spi_write(SPI_DIVIDER, 1);  // 时钟除数寄存器
  spi_write(SPI_SS, SPI_SS_FLASH);  // 设置SPI从机编号
  spi_write(SPI_CTRL, cfg);   // 设置SPI控制寄存器
}

uint32_t flash_read(uint32_t addr) {
  spi_init(FLASH_CFG);
  spi_write(SPI_TX1, 0x03u << 24 | (addr & 0xffffffu));
  spi_write(SPI_TX0, 0);
  spi_write(SPI_CTRL, FLASH_CFG | SPI_CTRL_GO);
  while (spi_read(SPI_CTRL) & SPI_CTRL_GO);
  return spi_read(SPI_RX0);
}