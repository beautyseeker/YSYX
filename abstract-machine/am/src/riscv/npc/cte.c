#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  // printf("Handling IRQ: mcause=%d, mepc=%p\n", c->mcause, c->mepc);
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
    // printf("Returned from user_handler: mepc=%p\n", c->mepc);
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
