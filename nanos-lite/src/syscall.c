#include <common.h>
#include "syscall.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;   // 系统调用号寄存器，决定系统调用类型，通常是a7/a5寄存器
  a[1] = c->GPR_A0; // 系统调用参数寄存器，通常存放文件描述符，也可能被用来存放返回值寄存器
  a[2] = c->GPR_A1; // 系统调用参数寄存器,通常描述缓冲区地址等参数
  a[3] = c->GPR_A2; // 系统调用参数寄存器,通常描述缓冲区字节数

  switch (a[0]) {
    case SYS_yield: c->GPR_A0 = 0; break;
    case SYS_exit: printf("Program exited with code %d\n", a[1]); halt(a[1]);
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
