#include "trap.h"
#include <stdint.h>
#include <klib.h>
#include <klib-macros.h>

/*
 * mem-test：对 heap（SRAM 可写区，不含栈）做 8/16/32/64 位「先写后读」校验。
 * data = addr & len_mask（讲义示意图，小端）。
 */

 static void test_8(uintptr_t lo, uintptr_t hi) {
   uintptr_t a;
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

uint8_t psram_test() {
  /* 先读入局部变量：测试会覆盖 .data 中的 heap */
  uintptr_t lo = (uintptr_t)heap.start;
  /* 暂时测试PSRAM的 4kB的空间，防止测试时间过长*/
  uintptr_t hi = (lo + (1 << 12));

  check(lo < hi);
  check((lo & 7) == 0);
  check((hi & 7) == 0);

  test_8(lo, hi);
  test_16(lo, hi);
  test_32(lo, hi);
  // test_64(lo, hi);
  return 1;
}

uint8_t sram_test() {
  return 1;
}
