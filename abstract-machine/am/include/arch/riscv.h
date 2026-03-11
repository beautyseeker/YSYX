#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

struct Context {
  // TODO: fix the order of these members to match trap.S
  uintptr_t gpr[NR_REGS], mcause, mstatus, mepc;
  void *pdir;
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]
#define GPR_RA gpr[1]
#define GPR_SP gpr[2]
#define GPR_GP gpr[3]
#define GPR_TP gpr[4]
#define GPR_FP gpr[8]
#define GPR_A0 gpr[10]
#define GPR_A1 gpr[11]
#define GPR_A2 gpr[12]
#define GPR_A3 gpr[13]
#define GPR_A4 gpr[14]


#endif
