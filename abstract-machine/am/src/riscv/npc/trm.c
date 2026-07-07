#include <am.h>
#include <klib-macros.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch) {
    asm volatile (
        "li t0, 0x10000000\n\t"  // 加载串口地址
        "sw %0, 0(t0)"           // 将第一个操作数 (%0) 写入地址
        :                        // 无输出
        : "r"(ch)                // 输入参数：ch 放入任意寄存器
        : "t0"                   // 告诉编译器 t0 寄存器被修改了
    );
}

void halt(int code) {
  asm volatile("mv a0, %0; ebreak" : :"r"(code));
  while (1);
}

void _trm_init() {
  int ret = main(mainargs);
  halt(ret);
}
