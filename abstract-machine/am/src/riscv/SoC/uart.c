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

/* 硬件 TX FIFO 深度：THRE=1 时 FIFO 全空，一次最多可连写这么多 */
#define UART_HW_TX_FIFO 16
/* 软件写缓冲 */
#define UART_TX_BUF_SZ  64

static char     tx_buf[UART_TX_BUF_SZ];
static unsigned tx_rd; /* 读指针：下一次写出的位置 */
static unsigned tx_wr; /* 写指针：下一次缓存的位置 */
static unsigned tx_len;

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

/* THRE 就绪：把缓冲里的字节连续写入硬件 FIFO，并推进 tx_rd */
static void uart_tx_drain(void) {
  if (!uart_tx_ready() || tx_len == 0)
    return;

  unsigned n = tx_len < UART_HW_TX_FIFO ? tx_len : UART_HW_TX_FIFO;
  while (n--) {
    uart_write(UART_THR, (uint8_t)tx_buf[tx_rd]);
    tx_rd = (tx_rd + 1) % UART_TX_BUF_SZ;
    tx_len--;
  }
}

void uart_tx_flush(void) {
  while (tx_len > 0) {
    while (!uart_tx_ready())
      ;
    uart_tx_drain();
  }
}

void uart_putch(char ch) {
  /* 缓冲满：必须等 THRE，先排空一段再继续缓存 */
  while (tx_len == UART_TX_BUF_SZ) {
    while (!uart_tx_ready())
      ;
    uart_tx_drain();
  }

  /* 未就绪或已就绪都先入队；就绪则马上连写 */
  tx_buf[tx_wr] = ch;
  tx_wr = (tx_wr + 1) % UART_TX_BUF_SZ;
  tx_len++;

  if (uart_tx_ready())
    uart_tx_drain();
}

uint8_t uart_getch(void) {
  if (!(uart_read(UART_LSR) & UART_LSR_DR))
    return 0xFF;
  return uart_read(UART_RBR);
}
