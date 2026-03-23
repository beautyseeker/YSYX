#include <am.h>
#include <klib-macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define HEAP_SIZE (128 * 1024 * 1024) // 128MB 示例
static uint8_t my_heap[HEAP_SIZE];

Area heap = { my_heap, my_heap + HEAP_SIZE };

// extern "C" {
//     int main(const char *args);
//     static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
// }

void putch(char ch) {
    write(1, &ch, 1);
}

void halt(int code) {
    printf("Navy program halted with code %d\n", code);
    exit(code);
}

// void _trm_init() {
//   int ret = main(mainargs);
//   halt(ret);
// }
