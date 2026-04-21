#ifndef __ISA_H__
#define __ISA_H__
#include "common.h"

extern const char *rv32_reg_name[32];

typedef struct __attribute__((packed)) {
  // 注意：检查你的 config，如果你开启了 CONFIG_RVE，这里是 16，否则是 32
  word_t gpr[32]; 
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
