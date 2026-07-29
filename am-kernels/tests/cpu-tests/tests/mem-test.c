#include "trap.h"
#include <stdint.h>

/*
 * mem-test：对 heap（SRAM 可写区，不含栈）做 8/16/32/64 位「先写后读」校验。
 * data = addr & len_mask（讲义示意图，小端）。
 *
 * 注意：不能使用可写全局/静态变量（它们也在 SRAM，会被测试覆盖）；
 * 不用 printf；用 volatile 防止访存被优化掉。
 */

static void test_8(uintptr_t lo, uintptr_t hi) {
  uintptr_t a;
  for (a = lo; a < hi; a += 1) {
    *(volatile uint8_t *)a = (uint8_t)(a & 0xffu);
  }
  for (a = lo; a < hi; a += 1) {
    check(*(volatile uint8_t *)a == (uint8_t)(a & 0xffu));
  }
}

static void test_16(uintptr_t lo, uintptr_t hi) {
  uintptr_t a;
  for (a = lo; a < hi; a += 2) {
    *(volatile uint16_t *)a = (uint16_t)(a & 0xffffu);
  }
  for (a = lo; a < hi; a += 2) {
    check(*(volatile uint16_t *)a == (uint16_t)(a & 0xffffu));
  }
}

static void test_32(uintptr_t lo, uintptr_t hi) {
  uintptr_t a;
  for (a = lo; a < hi; a += 4) {
    *(volatile uint32_t *)a = (uint32_t)a;
  }
  for (a = lo; a < hi; a += 4) {
    check(*(volatile uint32_t *)a == (uint32_t)a);
  }
}

static void test_64(uintptr_t lo, uintptr_t hi) {
  uintptr_t a;
  /* RV32 无 sd/ld：按讲义 64 位图案拆成两个字 */
  for (a = lo; a < hi; a += 8) {
    *(volatile uint32_t *)(a + 0) = (uint32_t)a;
    *(volatile uint32_t *)(a + 4) = 0;
  }
  for (a = lo; a < hi; a += 8) {
    check(*(volatile uint32_t *)(a + 0) == (uint32_t)a);
    check(*(volatile uint32_t *)(a + 4) == 0);
  }
}

int main(const char *mainargs) {
  /* 先读入局部变量：测试会覆盖 .data 中的 heap */
  uintptr_t lo = (uintptr_t)heap.start;
  uintptr_t hi = (uintptr_t)heap.end;
  (void)mainargs;

  check(lo < hi);
  check((lo & 7) == 0);
  check((hi & 7) == 0);

  test_8(lo, hi);
  test_16(lo, hi);
  test_32(lo, hi);
  test_64(lo, hi);
  return 0;
}
