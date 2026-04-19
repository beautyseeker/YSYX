#ifndef __ISA_H__
#define __ISA_H__
#include "common.h"

extern const char *rv32_reg_name[32];

void isa_reg_display();
word_t isa_reg_str2val(const char *s, bool *success);
word_t vaddr_read(vaddr_t addr, int len);
void cpu_exec(uint64_t n);

#endif
