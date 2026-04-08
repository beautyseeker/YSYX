#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  // 由于页表指针pdir永远是4KB页对齐的，所以pdir的低12位一定是0，因此直接右移12位即可得到页表物理页号
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  // satp寄存器的低22位是页表物理页号，因此左移12位即可得到4KB页对齐的页表物理地址pdir
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
  // 作用: 把内核的一级页表内容完整拷贝到用户的一级页表中。这样，无论切换到哪个进程，
  // 地址空间的高位（内核部分）都是一模一样的。这使得进程陷入内核态时，不需要切换页表也能正常执行代码
}

void unprotect(AddrSpace *as) {
  pgfree_usr(as->ptr);
  as->ptr = NULL;
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  PTE *pg_dir = (PTE *)as->ptr;
  uintptr_t vpn1 = VPN1(va);
  uintptr_t vpn0 = VPN0(va);

  // 1. 处理一级页表项
  if (!(pg_dir[vpn1] & PTE_V)) {
    // 分配二级页表
    PTE *new_pt = (PTE *)pgalloc_usr(PGSIZE);
    // 清零新分配的页表，防止旧数据干扰
    memset(new_pt, 0, PGSIZE); 
    // 填入一级页表：注意 PPN 转换
    pg_dir[vpn1] = (((uintptr_t)new_pt >> 12) << 10) | PTE_V;
  }

  // 2. 找到二级页表并填入最终映射
  PTE *pg_table = (PTE *)((pg_dir[vpn1] >> 10) << 12);
  // 必须加上 PTE_U，否则用户程序无法访问！
  // 假设 prot 传入时已经包含了必要的 R/W/X 权限
  pg_table[vpn0] = (((uintptr_t)pa >> 12) << 10) | prot | PTE_V | PTE_U;
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  protect(as);
  Context *ctx = (Context *)(kstack.end - sizeof(Context));
  ctx->mstatus = 0x80; // MPP=U-mode, MPIE=1
  ctx->mepc = (uintptr_t)entry;
  ctx->pdir = as->ptr;
  return ctx;
}
