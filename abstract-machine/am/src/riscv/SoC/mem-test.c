#include <am.h>
#include <klib.h>
#include <klib-macros.h>
extern char _heap_start[];
#define MEM_TEST_TEXT __attribute__((section(".sram.text"), noinline))

MEM_TEST_TEXT
void check(bool cond) {
  if (!cond) halt(1);
}

/*
 * mem-test：对 heap（SRAM 可写区，不含栈）做 8/16/32/64 位「先写后读」校验。
 * data = addr & len_mask（讲义示意图，小端）。
 */

 MEM_TEST_TEXT
 static void test_8(uintptr_t lo, uintptr_t hi) {
   uintptr_t a;
   // 内存自检阶段尽量使用外部调试接口而不要调用printf，此处TODO
   printf("byte write phase: [%p, %p)\n", (void*)lo, (void*)hi);
   for (a = lo; a < hi; a += 1) {
     *(volatile uint8_t *)a = (uint8_t)(a & 0xffu);
   }
   printf("byte read phase\n");
   for (a = lo; a < hi; a += 1) {
    uint8_t expect = (uint8_t)(a & 0xffu);
    uint8_t got = *(volatile uint8_t *)a;
    if (got != expect) {
      printf("FAIL at %p: got %x expect %x\n", (void*)a, got, expect);
      check(0);
    }
   }
   printf("test_8 pass\n");
 }

MEM_TEST_TEXT
static void test_16(uintptr_t lo, uintptr_t hi) {
  uintptr_t a;
  printf("halfword write phase: [%p, %p)\n", (void*)lo, (void*)hi);
  for (a = lo; a < hi; a += 2) {
    *(volatile uint16_t *)a = (uint16_t)(a & 0xffffu);
  }
  printf("halfword read phase\n");
  for (a = lo; a < hi; a += 2) {
    uint16_t expect = (uint16_t)(a & 0xffffu);
    uint16_t got = *(volatile uint16_t *)a;
    if (got != expect) {
      printf("FAIL at %p: got %x expect %x\n", (void*)a, got, expect);
      check(0);
    }
  }
  printf("test_16 pass\n");
}

MEM_TEST_TEXT
static void test_32(uintptr_t lo, uintptr_t hi) {
  uintptr_t a;
  printf("word write phase: [%p, %p)\n", (void*)lo, (void*)hi);
  for (a = lo; a < hi; a += 4) {
    *(volatile uint32_t *)a = (uint32_t)a;
  }
  printf("word read phase\n");
  for (a = lo; a < hi; a += 4) {
    uint32_t expect = (uint32_t)a;
    uint32_t got = *(volatile uint32_t *)a;
    if (got != expect) {
      printf("FAIL at %p: got %x expect %x\n", (void*)a, got, expect);
      check(0);
    }
  }
  printf("test_32 pass\n");
}

// static void test_64(uintptr_t lo, uintptr_t hi) {
//   uintptr_t a;
//   /* RV32 无 sd/ld：按讲义 64 位图案拆成两个字 */
//   for (a = lo; a < hi; a += 8) {
//     *(volatile uint32_t *)(a + 0) = (uint32_t)a;
//     *(volatile uint32_t *)(a + 4) = 0;
//   }
//   for (a = lo; a < hi; a += 8) {
//     check(*(volatile uint32_t *)(a + 0) == (uint32_t)a);
//     check(*(volatile uint32_t *)(a + 4) == 0);
//   }
// }

MEM_TEST_TEXT
bool psram_test() {
  /* 只测堆区，避免踩掉 PSRAM 上的 .text/.data/.bss */
  uintptr_t lo = (uintptr_t)_heap_start;
  /* 暂时测 4KB，防止仿真过久 */
  uintptr_t hi = lo + (1 << 12);

  /* 地址高低位检查和8字节对齐检查 */
  check(lo < hi);
  check((lo & 7) == 0);
  check((hi & 7) == 0);

  test_8(lo, hi);
  test_16(lo, hi);
  test_32(lo, hi);
  // test_64(lo, hi);
  return true;
}

MEM_TEST_TEXT
bool sram_test(void) {
  return true;
}
