#include <amtest.h>

/*
 * OpenCores SPI Master @ 0x10001000
 * 寄存器手册: ysyxSoC/perip/spi/doc/spi.pdf
 * （不是 flash.pdf —— 那是 Flash 颗粒的数据手册）
 *
 * 与 bitrev.v 的约定:
 *   - LSB=0: MSB 先传
 *   - Tx_NEG=1: MOSI 在 SCK 下降沿更新 → slave 上升沿采样
 *   - Rx_NEG=1: MISO 在 SCK 下降沿采样 → 避开 slave 上升沿改数
 *   - ASS=1: 传输期间自动拉低 SS
 *   - CHAR_LEN=16: 先发 8bit 数据, 再收 8bit 翻转结果
 */

#define SPI_BASE 0x10001000u

#define SPI_RX0     0x00u
#define SPI_TX0     0x00u
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

static inline uint32_t spi_read(uint32_t off) {
  return *(volatile uint32_t *)(SPI_BASE + off);
}

static inline void spi_write(uint32_t off, uint32_t val) {
  *(volatile uint32_t *)(SPI_BASE + off) = val;
}

static uint8_t bitrev8(uint8_t x) {
  x = (uint8_t)(((x & 0x55u) << 1) | ((x & 0xAAu) >> 1));
  x = (uint8_t)(((x & 0x33u) << 2) | ((x & 0xCCu) >> 2));
  x = (uint8_t)(((x & 0x0Fu) << 4) | ((x & 0xF0u) >> 4));
  return x;
}

/* 对 bitrev 做一次 16bit 传输, 返回 slave 回送的 8bit */
static uint8_t bitrev_xfer(uint8_t din) {
  /* CHAR_LEN=16 且 MSB-first: 先发 [15:8]=din, 再发 [7:0]=dummy */
  spi_write(SPI_TX0, (uint32_t)din << 8);

  spi_write(SPI_DIVIDER, 0);           /* SCK = wb_clk / 2, 仿真里尽快 */
  spi_write(SPI_SS, SPI_SS_BITREV);

  uint32_t ctrl = SPI_CTRL_ASS | SPI_CTRL_TX_NEG | SPI_CTRL_RX_NEG |
                  SPI_CTRL_CHAR_LEN(16);
  /* 手册要求: 先写好配置(GO=0), 再写一次置 GO 启动 */
  spi_write(SPI_CTRL, ctrl);
  spi_write(SPI_CTRL, ctrl | SPI_CTRL_GO);

  while (spi_read(SPI_CTRL) & SPI_CTRL_GO);

  /* 前 8bit MISO=1 → RX[15:8]=0xff; 后 8bit 为翻转结果 → RX[7:0] */
  return (uint8_t)(spi_read(SPI_RX0) & 0xffu);
}

void spi_test() {
  static const uint8_t patterns[] = {
      0x00, 0xff, 0x01, 0x80, 0x12, 0xa5, 0x5a, 0xc3
  };

  printf("SPI bitrev test (slave #7)\n");
  int fail = 0;
  for (unsigned i = 0; i < sizeof(patterns); i++) {
    uint8_t in  = patterns[i];
    uint8_t exp = bitrev8(in);
    uint8_t got = bitrev_xfer(in);
    printf("  in=0x%02x expect=0x%02x got=0x%02x %s\n",
           in, exp, got, got == exp ? "OK" : "FAIL");
    if (got != exp) fail++;
  }

  if (fail) {
    printf("SPI bitrev: %d failed\n", fail);
    halt(1);
  }
  printf("SPI bitrev: ALL PASS\n");
}
