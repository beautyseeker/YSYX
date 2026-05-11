#ifndef __ISA_H__
#define __ISA_H__
#include "common.h"

#define NR_GPR MUXDEF(CONFIG_RVE, 16, 32)
extern const char *rv32_reg_name[NR_GPR];

typedef struct __attribute__((packed)) {
  word_t gpr[NR_GPR];
  vaddr_t pc;
  struct {
    word_t mstatus;
    word_t mtvec;
    word_t mepc;
    word_t mcause;
    word_t satp;
  } csr;
} DUT_data;

void isa_reg_display();
word_t isa_reg_str2val(const char *s, bool *success);
word_t vaddr_read(vaddr_t addr, int len);
void cpu_exec(uint64_t n);

#endif
