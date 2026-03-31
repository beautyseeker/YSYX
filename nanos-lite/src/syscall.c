#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <proc.h>
#include <am.h>

struct timeval {
  long    tv_sec;         /* 秒 */
  long    tv_usec;        /* 微秒 */
};

struct timezone {
  int tz_minuteswest;     /* 格林威治以西的分钟差 */
  int tz_dsttime;         /* 夏令时修正类型 */
};

Context* do_syscall(Context *c);
int brk(void *addr);
void *_sbrk(intptr_t increment);
void _exit(int status);
int _gettimeofday(struct timeval *tv, struct timezone *tz);

// #define CONFIG_STRACE

#ifdef CONFIG_STRACE

static const char *syscall_name[] = {
  "SYS_exit",
  "SYS_yield",
  "SYS_open",
  "SYS_read",
  "SYS_write",
  "SYS_kill",
  "SYS_getpid",
  "SYS_close",
  "SYS_lseek",
  "SYS_brk",
  "SYS_fstat",
  "SYS_time",
  "SYS_signal",
  "SYS_execve",
  "SYS_fork",
  "SYS_link",
  "SYS_unlink",
  "SYS_wait",
  "SYS_times",
  "SYS_gettimeofday"
};

static void print_strace(uintptr_t *a, uintptr_t ret) {
  static char str[128];
  int id = a[0];
  
  // 针对不同系统调用定制参数说明
  switch (id) {
    case SYS_write:
      snprintf(str, sizeof(str), "%s(fd = %d, buf = 0x%x, count = %d) = %d", 
               syscall_name[id], a[1], a[2], a[3], ret);
      break;
    case SYS_brk:
      // a1 是新的堆顶地址，ret 通常是 0 (或旧地址，取决于你的实现)
      snprintf(str, sizeof(str), "%s(increment = %d) = 0x%x", 
               syscall_name[id], a[1], ret);
      break;
    case SYS_exit:
      snprintf(str, sizeof(str), "%s(status = %d)", syscall_name[id], a[1]);
      break;
    case SYS_yield:
      snprintf(str, sizeof(str), "%s() = %d", syscall_name[id], ret);
      break;
    case SYS_lseek:
      snprintf(str, sizeof(str), "%s(fd = %d, offset = %d, whence = %d) = %d", 
               syscall_name[id], a[1], a[2], a[3], ret);
      break;
    case SYS_open:
      snprintf(str, sizeof(str), "%s(pathname = 0x%x, flags = %d, mode = %d) = %d", 
               syscall_name[id], a[1], a[2], a[3], ret);
      break;
    case SYS_read:
      snprintf(str, sizeof(str), "%s(fd = %d, buf = 0x%x, count = %d) = %d", 
               syscall_name[id], a[1], a[2], a[3], ret);
      break;
    case SYS_close:
      snprintf(str, sizeof(str), "%s(fd = %d) = %d", 
               syscall_name[id], a[1], ret);
      break;
    case SYS_gettimeofday:
      // snprintf(str, sizeof(str), "%s(tv_ptr = 0x%x, tz_ptr = 0x%x) = %d", 
      //          syscall_name[id], a[1], a[2], ret);
      return;
    case SYS_execve:
      snprintf(str, sizeof(str), "%s(filename = 0x%x, argv = 0x%x, envp = 0x%x) = %d", 
               syscall_name[id], a[1], a[2], a[3], ret);
      break;
    default:
      snprintf(str, sizeof(str), "%s(0x%x, 0x%x, 0x%x) = %d", 
               syscall_name[id], a[1], a[2], a[3], ret);
      break;
  }

  printf("\33[1;35m[Strace]: %s\33[0m\n", str);
}
#endif

Context* do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;   // 系统调用号寄存器，决定系统调用类型，通常是a7/a5寄存器
  a[1] = c->GPR_A0; // 系统调用参数寄存器，通常存放文件描述符，也可能被用来存放返回值寄存器
  a[2] = c->GPR_A1; // 系统调用参数寄存器,通常描述缓冲区地址等参数
  a[3] = c->GPR_A2; // 系统调用参数寄存器,通常描述缓冲区字节数

  switch (a[0]) {
    case SYS_yield: c->GPR_A0 = 0; break;
    case SYS_exit: return context_uload(current, "/bin/menu", NULL, NULL); break;
    case SYS_open: c->GPR_A0 = (uintptr_t)fs_open((const char *)a[1], a[2], a[3]); break;
    case SYS_read: c->GPR_A0 = (uintptr_t)fs_read(a[1], (void *)a[2], a[3]); break;
    case SYS_write: c->GPR_A0 = (uintptr_t)fs_write(a[1], (void *)a[2], a[3]); break;
    case SYS_lseek: c->GPR_A0 = (uintptr_t)fs_lseek(a[1], a[2], a[3]); break;
    case SYS_close: c->GPR_A0 = (uintptr_t)fs_close(a[1]); break;
    case SYS_brk: c->GPR_A0 = (uintptr_t)_sbrk(a[1]); break;
    case SYS_gettimeofday: c->GPR_A0 = (uintptr_t)_gettimeofday\
    ((struct timeval *)a[1], (struct timezone *)a[2]); break;
    case SYS_execve:
      const char *filename = (const char *)a[1];
      char *const *argv = (char *const *)a[2];
      char *const *envp = (char *const *)a[3]; 
      Context* new_thread = context_uload(current, filename, argv, envp);
      c = new_thread; // 切换到新线程的上下文
    break;

    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  return c;

#ifdef CONFIG_STRACE
  print_strace(a, c->GPR_A0);
#endif
}

void _exit(int status) {
  printf("Program exited with code %d\n", status);
  halt(status);
}

extern char end;
void *ptr_break = &end;
// 将堆起始地址偶数对齐
void *heap_start = (void *) &end; 
// 设置一个合理的上限，防止越界
void *heap_limit = (void *)0x88000000;

int brk(void *addr) {
  if ((uintptr_t) addr >= (uintptr_t) heap_start \
  && (uintptr_t) addr <= (uintptr_t) heap_limit) {
    ptr_break = addr;
    return 0;
  } 
  return -1;
}

void *_sbrk(intptr_t increment) {
  void *old_break = ptr_break;
  if (brk((void *)((uintptr_t)ptr_break + increment)) == -1) {
    return (void *)-1; // brk失败，返回-1
  }
  else {
    return (void *)old_break; // 返回原来堆地址
  }
}

int _gettimeofday(struct timeval *tv, struct timezone *tz) {
  // 这里暂时不支持时区信息，直接返回0
  if (tv) {
    uint64_t uptime_us = io_read(AM_TIMER_UPTIME).us;
    tv->tv_sec = uptime_us / 1000000; // 秒
    tv->tv_usec = uptime_us % 1000000; // 微秒
    return 0;
  }
  return -1;
}
