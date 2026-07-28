#include <am.h>
#include "arch/SoC.h"
#include <klib-macros.h>
#include <stdint.h>

int main(const char *args);

void halt(int code) __attribute__((__noreturn__));

static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
extern char _data_start[], _edata[], _data_LMA_start[];
extern char _bss[], _ebss[];
extern char _stack_top[], _stack_pointer[];

void uart_init(void);
void uart_putch(char ch);
uint32_t flash_read(uint32_t addr);

/* 堆：SRAM 起始 → 栈区低端（绕开栈）；栈顶初值在 _stack_pointer */
Area heap = RANGE(SRAM_BASE, _stack_top);

void putch(char ch) {
  uart_putch(ch);
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


#define FLASH_OFF  0            // char-test 在 flash[] 的片内偏移
#define SRAM_DST   0x0f001000   // 拷贝落点（避开自己的栈/数据）
#define LEN        56           // char-test.bin 字节数，可写死或宏

static inline uint32_t bswap32(uint32_t x) {
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

void exec_flash_inst(void) {
  uint8_t *dst = (uint8_t *)SRAM_DST;
  for (uint32_t off = 0; off < LEN; off += 4) {
    uint32_t w = bswap32(flash_read(FLASH_OFF + off));
    *(uint32_t *)(dst + off) = w;   // 或按字节写入
  }
  void (*entry)(void) = (void (*)(void))SRAM_DST;
  entry();
}

void _trm_init() {
  data_seg_init();
  uart_init();
  exec_flash_inst();
  int ret = main(mainargs);
  halt(ret);
}
