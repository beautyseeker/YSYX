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

extern uint8_t psram_test();
extern uint8_t sram_test();
/* 堆：SRAM 起始 → 栈区低端（绕开栈）；栈顶初值在 _stack_pointer */
Area heap = RANGE(PSRAM_BASE, _stack_top);

static int mem_test();
static void POST_phase();
static void data_seg_init();
static void print_vendor_info();

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

static void POST_phase() {
  putstr("POST...\n");
  mem_test();
}

static int mem_test() {
  int success = 0;
  success |= psram_test();
  if (!success) {
    putstr("PSRAM test failed\n");
    halt(1);
  }
  success |= sram_test();
  if (!success) {
    putstr("SRAM test failed\n");
    halt(1);
  }
  return success;
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
  POST_phase();
  print_vendor_info();
  int ret = main(mainargs);
  halt(ret);
}
