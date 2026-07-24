#include <am.h>
#include <klib-macros.h>

#define UART_BASE 0x10000000
#define UART_TX   0x00

#define SRAM_BASE 0x0f000000
#define SRAM_SIZE 0x00f00000

int main(const char *args);

void halt(int code) __attribute__((__noreturn__));

// extern char _pmem_start;
// #define PMEM_SIZE (8 * 1024)
// #define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch) {
  *(volatile char *)(UART_BASE + UART_TX) = ch;
}

void halt(int code) {
  asm volatile("mv a0, %0; ebreak" : :"r"(code));
  while (1);
}

void _trm_init() {
  // putch('S');
  int ret = main(mainargs);
  // putch('F');
  halt(ret);
}
