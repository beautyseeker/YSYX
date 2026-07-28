#include <am.h>
#include "arch/SoC.h"
#include <klib-macros.h>
#include <klib.h>
#include <stdint.h>

int main(const char *args);

void halt(int code) __attribute__((__noreturn__));

static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
extern char _data_start[], _edata[], _data_LMA_start[];
extern char _bss[], _ebss[];
extern char _stack_top[], _stack_pointer[];

void uart_init(void);
void uart_putch(char ch);

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

static void print_vendor_info() {
  uint32_t mvendorid_val, marchid_val;
  asm volatile ("csrr %0, mvendorid" : "=r"(mvendorid_val));
  asm volatile ("csrr %0, marchid"   : "=r"(marchid_val));

  char vendorid[4] = {0};
  for (int i = 0; i < sizeof(vendorid); i++) {
    vendorid[i] = (mvendorid_val >> ((sizeof(vendorid)-i-1) * 8)) & 0xFF;
  }

  printf("--------------------------------\n");
  printf("vendorid: %s  marchid: %d\n", vendorid, marchid_val);
  printf("--------------------------------\n");
}

void _trm_init() {
  data_seg_init();
  uart_init();
  print_vendor_info();
  int ret = main(mainargs);
  halt(ret);
}
