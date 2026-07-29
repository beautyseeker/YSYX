#define FLASH_BASE 0x30000000
#include "klib.h"

extern void putch(char ch);
extern char _rodata_start[], _etext[];

int main(const char *mainargs) {
  printf("Hello, World! from dummy %s\n", mainargs);
  return 0;
}
