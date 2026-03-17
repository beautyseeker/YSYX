#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot __attribute__((used)) = {};
PCB *current = NULL;
extern void naive_uload(PCB *pcb, const char *filename);
extern uintptr_t loader(PCB *pcb, const char *filename);
extern Area heap;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    for(int volatile i = 0; i < 100000; i++);
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {

  Log("Initializing processes...");
  context_kload(&pcb[0], hello_fun, (void *)0x12345678);
  // context_kload(&pcb[1], hello_fun, (void *)0x87654321);
  context_uload(&pcb[1], "/bin/event-test");
  // context_uload(&pcb[1], "/bin/hello");
  switch_boot_pcb();

  // naive_uload(NULL, "/bin/timer-test");

}

Context* schedule(Event ev, Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  printf("Switching to process from %p to %p\n", prev, current->cp);
  return current->cp;
}

Context* context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  pcb->as = (AddrSpace){.pgsize = 4096, .area = {0}, .ptr = NULL};
  pcb->max_brk = 0;
  pcb->cp = kcontext((Area){pcb->stack, pcb->stack + STACK_SIZE}, entry, arg);
  sprintf(pcb->name, "kernel_pcb_%p", entry);
  printf("kernel pcb created in addr %p where entry = %p arg = %p\n", 
  pcb, entry, arg);
  return pcb->cp;
}

Context* context_uload(PCB *pcb, const char *filename) {
  pcb->as = (AddrSpace){.pgsize = 4096, .area = {0}, .ptr = NULL};
  pcb->max_brk = 0;
  uintptr_t entry = loader(pcb, filename);
  pcb->cp = ucontext(&pcb->as, (Area){pcb->stack,
  pcb->stack + STACK_SIZE}, (void *)entry);
  // 直接将用户栈指针sp初始化为堆末地址，兜底做法
  pcb->cp->GPR_SP = (uintptr_t)heap.end;
  // 将堆末地址作为参数a0传递给用户程序，便于用户程序启动start.S时正确初始化sp 
  pcb->cp->GPR_A0 = (uintptr_t)heap.end; 
  sprintf(pcb->name, "user_pcb_%s", filename);
  printf("user pcb created from %s in addr %p where entry = %p\n",
  filename, pcb, entry);
  return pcb->cp;
}
