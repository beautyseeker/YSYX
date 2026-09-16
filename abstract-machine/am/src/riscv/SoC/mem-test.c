#include <am.h>
#include <arch/SoC.h>

#define putstr(s) \
  ({ for (const char *p = (s); *p; p++) putch(*p); })

#define fail_if(cond, msg, ...) do { \
  if (cond) { putstr(msg); __VA_ARGS__; halt(1); } \
} while (0)

/* 按宽度写满 [lo,hi)，再读回校验；图案 = addr 低位截断到该宽度 */
#define DEF_WIDTH_TEST(name, T, step, pattern) \
static void name(uintptr_t lo, uintptr_t hi) { \
  uintptr_t a; \
  for (a = lo; a < hi; a += (step)) \
    *(volatile T *)a = (T)(pattern); \
  for (a = lo; a < hi; a += (step)) { \
    T expect = (T)(pattern); \
    T got = *(volatile T *)a; \
    fail_if(got != expect, #name " failed\n", \
      putstr("got: "), puthex(got), putstr(", "), \
      putstr("expect: "), puthex(expect), putstr("\n")); \
  } \
}

extern void putch(char ch);
extern char _psram_start[], _sdram_start[];

static void puthex(uint32_t value) {
  char buf[9];
  int i = 0;
  do {
    uint32_t digit = value % 16;
    buf[i++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
    value /= 16;
  } while (value > 0);
  while (i--)
    putch(buf[i]);
}

DEF_WIDTH_TEST(test_8,  uint8_t,  1, a & 0xffu)
DEF_WIDTH_TEST(test_16, uint16_t, 2, a & 0xffffu)
DEF_WIDTH_TEST(test_32, uint32_t, 4, a)

static bool region_test(const char *tag, uintptr_t lo, uintptr_t hi) {
  putstr(tag);
  putstr("[");
  puthex(lo);
  putstr(", ");
  puthex(hi);
  putstr("] started\n");

  fail_if(lo >= hi, "FAIL: lo >= hi\n");
  fail_if((lo & 7) != 0, "FAIL: lo is not 8-byte aligned\n");
  fail_if((hi & 7) != 0, "FAIL: hi is not 8-byte aligned\n");

  test_32(lo, hi);
  test_16(lo, hi);
  test_8(lo, hi);

  putstr(tag);
  putstr(" test PASSED\n");
  return true;
}

bool psram_test(void) {
  uintptr_t lo = (uintptr_t)_psram_start;
  return region_test("PSRAM", lo, lo + (1 << 12));
}

// bool sdram_test(void) {
//   uintptr_t lo = (uintptr_t)_sdram_start;
//   return region_test("SDRAM", lo, lo + (1 << 12));
// }

bool sram_test(void) {
  return true;
}

void dram_test(void) {
  psram_test();
  // sdram_test();
}
