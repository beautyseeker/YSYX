#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot __attribute__((used)) = {};
PCB *current = NULL;
extern void naive_uload(PCB *pcb, const char *filename);

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
  context_kload(&pcb[0], hello_fun, (void *)0x12345678);
  context_kload(&pcb[1], hello_fun, (void *)0x87654321);
  switch_boot_pcb();

  Log("Initializing processes...");

  // naive_uload(NULL, "/bin/timer-test");

}

Context* schedule(Event ev, Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  printf("Switching to process from %p to %p\n", prev, current->cp);
  return current->cp;
}

Context* context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  pcb->cp = kcontext((Area){pcb->stack, pcb->stack + STACK_SIZE}, entry, arg);
  pcb->as = (AddrSpace){.pgsize = 4096, .area = {0}, .ptr = NULL};
  pcb->max_brk = 0;
  printf("pcb created in %p where context = %p entry = %p arg = %p\n", 
  pcb, pcb->cp, entry, arg);
  return pcb->cp;
}
