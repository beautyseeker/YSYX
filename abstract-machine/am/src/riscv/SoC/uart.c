#include "arch/SoC.h"
/* ---------------- UART16550 @ 0x10000000 ---------------- */
/* 寄存器偏移（与 uart_defines.v / 规格书一致；DLAB=1 时 0/1 为除数） */
enum {
  UART_RBR = 0, /* DLAB=0, R: 接收 */
  UART_THR = 0, /* DLAB=0, W: 发送 */
  UART_IER = 1, /* DLAB=0 */
  UART_DLL = 0, /* DLAB=1, 除数低字节 */
  UART_DLM = 1, /* DLAB=1, 除数高字节 */
  UART_IIR = 2, /* R */
  UART_FCR = 2, /* W */
  UART_LCR = 3,
  UART_MCR = 4,
  UART_LSR = 5,
  UART_MSR = 6,
};

/* LCR */
#define UART_LCR_WLS_8  0x03u /* 8 data bits, 1 stop, no parity */
#define UART_LCR_DLAB   (1u << 7)

/* LSR */
#define UART_LSR_DR     (1u << 0) /* 收数据就绪 */
#define UART_LSR_THRE   (1u << 5) /* 发送保持寄存器/FIFO 可写 */
#define UART_LSR_TEMT   (1u << 6) /* 发送移位寄存器也空 */

/* 波特率除数：baud = uart_clk / (16 * divisor)。按 SoC 时钟自行核算后填入 */
#define UART_DLL_VAL    1u
#define UART_DLM_VAL    0u

uint8_t uart_read(unsigned reg) {
  return *(volatile uint8_t *)(UART_BASE + reg);
}

void uart_write(unsigned reg, uint8_t val) {
  *(volatile uint8_t *)(UART_BASE + reg) = val;
}

void uart_init(void) {
    uart_write(UART_LCR, UART_LCR_DLAB);              /* 打开除数锁存 */
    uart_write(UART_DLL, UART_DLL_VAL);
    uart_write(UART_DLM, UART_DLM_VAL);
    uart_write(UART_LCR, UART_LCR_WLS_8);             /* 关掉 DLAB，8N1 */
}

int uart_tx_ready(void) {
  return (uart_read(UART_LSR) & UART_LSR_THRE) != 0;
}

void uart_putch(char ch) {
  while (!uart_tx_ready())
    ;
  uart_write(UART_THR, (uint8_t)ch);
}
