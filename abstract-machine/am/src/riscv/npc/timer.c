#include <am.h>

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  //先读取高位保证第一次读取IO是4字节对齐的，否则可能会读到错误的值
  uint32_t high;
  asm volatile (
    "li t0, 0x10000048\n\t"  // RTC高地址存放高位
    "lw %0, 4(t0)"           // 读取高位到high
    : "=r"(high)             // 输出参数：high放入任意寄存器
    :                        // 无输入参数
    : "t0"                   // 告诉编译器 t0 寄存器被修改了
  );
  uint32_t low;
  asm volatile (
    "li t0, 0x10000048\n\t"  // RTC低地址存放低位
    "lw %0, 0(t0)"           // 读取低位到low
    : "=r"(low)              // 输出参数：low放入任意寄存器
    :                        // 无输入参数
    : "t0"                   // 告诉编译器 t0 寄存器被修改了
  );
  uptime->us = ((uint64_t)high << 32) | low;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
