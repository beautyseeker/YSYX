#include <am.h>
#include "arch/SoC.h"
#include <klib-macros.h>
#include <klib.h>

int main(const char *args);

void halt(int code) __attribute__((__noreturn__));

static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
extern char _data_start[], _edata[], _data_LMA_start[];
extern char _bss[], _ebss[];
extern char _stack_top[], _stack_pointer[];
extern char _sram_text_start[], _sram_text_end[], _sram_text_lma[];
extern char _heap_start[], _etext[], _text_start[], _text_lma[];
extern char _sram_loader_start[], _sram_loader_end[], _sram_loader_lma[];

void uart_init(void);
void uart_putch(char ch);
void _trm_init(void);
void _sram_loader(void);

extern bool psram_test();
extern bool sram_test();

Area heap = RANGE(&_heap_start, PSRAM_BASE + PSRAM_SIZE);

#define FSBL __attribute__((section(".text.boot"), noinline, used))
#define SSBL __attribute__((section(".sram.loader"), noinline, used))

// static bool mem_test();
// static void POST_phase();
static void print_vendor_info();

void putch(char ch) {
  uart_putch(ch);
}

void halt(int code) {
  asm volatile("mv a0, %0; ebreak" : :"r"(code));
  while (1);
}

/* 不依赖 klib：Flash stub / SRAM loader 共用同一实现风格 */
FSBL static void boot_memcpy(void *dst, const void *src, size_t n) {
  uint32_t *d = (uint32_t *)dst;
  const uint32_t *s = (const uint32_t *)src;
  size_t words = (n + 3) / 4;
  for (size_t i = 0; i < words; i++) {
    d[i] = s[i];
  }
}

SSBL static void loader_memcpy(void *dst, const void *src, size_t n) {
  uint32_t *d = (uint32_t *)dst;
  const uint32_t *s = (const uint32_t *)src;
  size_t words = (n + 3) / 4;
  for (size_t i = 0; i < words; i++) {
    d[i] = s[i];
  }
}

/*
 * 仅在 Flash 运行：把 SRAM loader 拷进 SRAM，再跳转。
 * 绝不能在此处 call 普通 .text（仍在 PSRAM VMA、尚未装载）。
 */
FSBL void _flash_boot(void) {
  size_t n = (size_t)(_sram_loader_end - _sram_loader_start);
  boot_memcpy(_sram_loader_start, _sram_loader_lma, n);

  void (*loader)(void) = (void (*)(void))(uintptr_t)_sram_loader;
  loader();
  while (1);
}

/*
 * 在 SRAM 取指：Flash → PSRAM 搬运 .text / .data，再进 _trm_init。
 */
SSBL void _sram_loader(void) {
  loader_memcpy(_text_start, _text_lma, (size_t)(_etext - _text_start));
  loader_memcpy(_data_start, _data_LMA_start, (size_t)(_edata - _data_start));

  void (*init)(void) = (void (*)(void))(uintptr_t)_trm_init;
  init();
  while (1);
}

// static void bss_seg_init() {
//   size_t n = (size_t)(_ebss - _bss);
//   if (n) {
//     memset(_bss, 0, n);
//   }
// }

// static void POST_phase() {
//   putstr("POST...\n");
//   mem_test();
// }

// static bool mem_test() {
//   bool success = true;
//   success &= psram_test();
//   success &= sram_test();
//   if (!success) {
//     putstr("Memory test failed\n");
//     halt(1);
//   }
//   return success;
// }

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

/* 链接到 PSRAM；由 _sram_loader 装入 .text/.data 之后进入 */
void _trm_init() {
  uart_init();
  putstr("uart_init done\n");
  // POST_phase();
  print_vendor_info();
  int ret = main(mainargs);
  halt(ret);
}
