#include <proc.h>
#include <memory.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot __attribute__((used)) = {};
PCB *current = NULL;
extern void naive_uload(PCB *pcb, const char *filename);
extern uintptr_t loader(PCB *pcb, const char *filename);
extern Area heap;
static uintptr_t parse_args_ustack(char *const argv[], char *const envp[], uintptr_t sp);

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    for(int volatile i = 0; i < 1; i++);
    // Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {

  Log("Initializing processes...");
  context_kload(&pcb[0], hello_fun, (void *)0x12345678);
  // context_kload(&pcb[1], hello_fun, (void *)0x87654321);
  // char *argv[] = {"/bin/exec-test", "1", NULL};
  context_uload(&pcb[1], "/bin/nterm", NULL, NULL);
  // context_uload(&pcb[1], "/bin/hello");
  switch_boot_pcb();

  // naive_uload(NULL, "/bin/timer-test");

}

Context* schedule(Event ev, Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  // printf("Switching to process from %p to %p due to event %d\n", prev, current->cp, ev.event);
  return current->cp;
}

Context* context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  pcb->as = (AddrSpace){.pgsize = 4096, .area = {0}, .ptr = NULL};
  pcb->max_brk = 0;
  pcb->cp = kcontext((Area){pcb->stack, pcb->stack + STACK_SIZE}, entry, arg);
  sprintf(pcb->name, "kernel_pcb_%p", entry);
  printf("Creating kernel pcb in addr %p where entry = %p arg = %p\n", 
  pcb, entry, arg);
  return pcb->cp;
}

Context* context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  pcb->as = (AddrSpace){.pgsize = 4096, .area = {0}, .ptr = NULL};
  pcb->max_brk = 0;
  uintptr_t entry = loader(pcb, filename);
  pcb->cp = ucontext(&pcb->as, (Area){pcb->stack,
  pcb->stack + STACK_SIZE}, (void *)entry);

  uintptr_t sp = (uintptr_t)new_page(STACK_SIZE / PGSIZE); // 为用户栈分配一页物理内存(32KB)
  sp = parse_args_ustack(argv, envp, sp); // 将传入的参数按照ABI规范压入用户栈，并返回新的栈顶地址

  pcb->cp->GPR_A0 = sp;  // 栈指针赋值给a0寄存器，便于外部用户程序通过a0解析出argc、argv、envp
  sprintf(pcb->name, "user_pcb_%s", filename);
  printf("Creating user pcb from external file %s in addr %p where entry = %p\n",
  filename, pcb, entry);
  return pcb->cp;
}

static uintptr_t parse_args_ustack(char *const argv[], char *const envp[], uintptr_t sp) {
  int argc = 0; while (argv && argv[argc]) argc++;
  int envc = 0; while (envp && envp[envc]) envc++;

  // 3. 第一阶段：拷贝字符串内容，并记录它们在【用户栈】里的新地址
  uintptr_t argv_ptrs[argc];
  uintptr_t envp_ptrs[envc];

  for (int i = envc - 1; i >= 0; i--) {
      sp -= (strlen(envp[i]) + 1);
      strcpy((char *)sp, envp[i]);
      envp_ptrs[i] = sp; // 记录下这个内容在栈里的位置
  }
  for (int i = argc - 1; i >= 0; i--) {
      sp -= (strlen(argv[i]) + 1);
      strcpy((char *)sp, argv[i]);
      argv_ptrs[i] = sp; 
  }
  // 4. 第二阶段：填充指针表
  // 强制 16 字节对齐（RISC-V 规范）
  sp &= ~0xf; 

  // 按照布局逆向压栈：NULL, envp[n...0], NULL, argv[n...0], argc
  // 或者顺着填：先预留空间，再从低到高填
  size_t table_size = (1 + argc + 1 + envc + 1) * sizeof(uintptr_t);
  sp -= table_size;
  uintptr_t *ptr_table = (uintptr_t *)sp;

  int idx = 0;
  ptr_table[idx++] = argc;
  for (int i = 0; i < argc; i++) ptr_table[idx++] = argv_ptrs[i];
  ptr_table[idx++] = 0; // argv 结束的 NULL
  for (int i = 0; i < envc; i++) ptr_table[idx++] = envp_ptrs[i];
  ptr_table[idx++] = 0; // envp 结束的 NULL
  return sp;
}
