#include <am.h>
#include <klib-macros.h>
#include <stdint.h>

/* ---------------- UART16550 @ 0x10000000 ---------------- */
#define UART_BASE 0x10000000u

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
#define UART_DLL_VAL    8u
#define UART_DLM_VAL    0u

static inline uint8_t uart_read(unsigned reg) {
  return *(volatile uint8_t *)(UART_BASE + reg);
}

static inline void uart_write(unsigned reg, uint8_t val) {
  *(volatile uint8_t *)(UART_BASE + reg) = val;
}

static inline int uart_tx_ready(void) {
  return (uart_read(UART_LSR) & UART_LSR_THRE) != 0;
}

#define SRAM_BASE 0x0f000000
#define SRAM_SIZE 0x00002000

int main(const char *args);

void halt(int code) __attribute__((__noreturn__));

static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
extern char _data_start[], _edata[], _data_LMA_start[];
extern char _bss[], _ebss[];
extern char _stack_top[], _stack_pointer[];

/* 堆：SRAM 起始 → 栈区低端（绕开栈）；栈顶初值在 _stack_pointer */
Area heap = RANGE(SRAM_BASE, _stack_top);

void putch(char ch) {
  while (!uart_tx_ready())
    ;
  uart_write(UART_THR, (uint8_t)ch);
}

void halt(int code) {
  asm volatile("mv a0, %0; ebreak" : :"r"(code));
  while (1);
}

static void data_seg_init() {
  uint32_t *dst = (uint32_t *)_data_start;
  uint32_t *src = (uint32_t *)_data_LMA_start;
  uint32_t *dend = (uint32_t *)_edata;
  while (dst < dend) {
    *dst++ = *src++;
  }
}

static void uart_init(void) {
  uart_write(UART_LCR, UART_LCR_DLAB);              /* 打开除数锁存 */
  uart_write(UART_DLL, UART_DLL_VAL);
  uart_write(UART_DLM, UART_DLM_VAL);
  uart_write(UART_LCR, UART_LCR_WLS_8);             /* 关掉 DLAB，8N1 */
}

void _trm_init() {
  data_seg_init();
  uart_init();
  int ret = main(mainargs);
  halt(ret);
}
