#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;
static uint8_t irq_depth = 0;

static void print_etrace(Context *c, Event ev)__attribute__((unused));

static void print_etrace(Context *c, Event ev) {
  printf("-mcause: 0x%08x, -a0: 0x%08x, -irq_depth: %d\n,\
  -mepc: 0x%08x sp: 0x%08x\n", c->mcause, c->GPR1, irq_depth, c->mepc, c->GPR_SP);
}

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 11: 
        if (c->GPR1 == -1) {
          ev.event = EVENT_YIELD;
        } else {
          ev.event = EVENT_SYSCALL;
        }
        c->mepc += 4; 
        break;

      case 0x80000007: // 硬件：机器模式计时器中断 (M-mode Timer Interrupt)
        ev.event = EVENT_IRQ_TIMER;
        break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  //这个函数作用是创建内核进程的上下文Context
  //传递进来的是一个始于栈顶stack终于上下文指针Context*的地址范围
  //还传递进来一个入口函数entry和它的参数arg
  //我猜测这个函数是创建内核进程时的入口函数
  //调用需要返回一个Context*指针
  Context *ctx = (Context *)(kstack.end - sizeof(Context));
  ctx->mstatus = 0x1800; // 设置 MPP=M-mode, MPIE=1
  ctx->mepc = (uintptr_t)entry;
  ctx->GPR_A0 = (uintptr_t)arg;
  return ctx;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
#ifdef __riscv_e
  uint64_t mstatus;
  asm volatile("csrr %0, mstatus" : "=r"(mstatus));
  return (mstatus & 0x8) != 0; // MIE 位
#else
  uint64_t mstatus;
  asm volatile("csrr %0, mstatus" : "=r"(mstatus));
  return (mstatus & 0x8) != 0; // MIE 位
#endif
}

void iset(bool enable) {
#ifdef __riscv_e
  uint64_t mstatus;
  asm volatile("csrr %0, mstatus" : "=r"(mstatus));
  if (enable) {
    mstatus |= 0x8; // 设置 MIE 位
  } else {
    mstatus &= ~0x8; // 清除 MIE 位
  }
  asm volatile("csrw mstatus, %0" : : "r"(mstatus));
#else
  uint64_t mstatus;
  asm volatile("csrr %0, mstatus" : "=r"(mstatus));
  if (enable) {
    mstatus |= 0x8; // 设置 MIE 位
  } else {
    mstatus &= ~0x8; // 清除 MIE 位
  }
  asm volatile("csrw mstatus, %0" : : "r"(mstatus));
#endif
}
