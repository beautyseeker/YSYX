#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;
//heap address
extern Area heap;
static char *heap_addr;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  if (heap_addr == NULL) {
    heap_addr = (char *)heap.start;
  }
  size = (size_t)ROUNDUP(size, 8);
  char *old = heap_addr;
  heap_addr += size;
  if ((uintptr_t)heap_addr < (uintptr_t)heap.start ||
      (uintptr_t)heap_addr > (uintptr_t)heap.end) {
    printf("heap_start:%x, heap_addr:%x,  heap_end:%x\n", 
      (uintptr_t)heap.start, (uintptr_t)heap_addr, (uintptr_t)heap.end);
    assert(0);
  }
  for (uint32_t *p = (uint32_t *)old; p != (uint32_t *)heap_addr; p ++) {
    *p = 0;
  }
  return old;
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  panic("Not implemented");
#endif
  return NULL;
}

void free(void *ptr) {
}

#endif
