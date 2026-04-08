/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])

#define CSRRS(imm, v, old_ptr) do { \
  word_t *p = NULL; \
  switch (imm) { \
    case 0x180: p = &(cpu.csr.satp); break; \
    case 0x300: p = &(cpu.csr.mstatus); break; \
    case 0x305: p = &(cpu.csr.mtvec);   break; \
    case 0x341: p = &(cpu.csr.mepc);    break; \
    case 0x342: p = &(cpu.csr.mcause);  break; \
    default: panic("unsupported csr addr = 0x%03x", imm); \
  } \
  *old_ptr = *p; \
  *p |= v; \
} while (0)

#define CSRRW(imm, v, old_ptr) do { \
  word_t *p = NULL; \
  switch (imm) { \
    case 0x180: p = &(cpu.csr.satp); break; \
    case 0x300: p = &(cpu.csr.mstatus); break; \
    case 0x305: p = &(cpu.csr.mtvec);   break; \
    case 0x341: p = &(cpu.csr.mepc);    break; \
    case 0x342: p = &(cpu.csr.mcause);  break; \
    default: panic("unsupported csr addr = 0x%03x", imm); \
  } \
  *old_ptr = *p; \
  *p = v; \
} while (0)

#define CSRRC(imm, v, old_ptr) do { \
  word_t *p = NULL; \
  switch (imm) { \
    case 0x180: p = &(cpu.csr.satp); break; \
    case 0x300: p = &(cpu.csr.mstatus); break; \
    case 0x305: p = &(cpu.csr.mtvec);   break; \
    case 0x341: p = &(cpu.csr.mepc);    break; \
    case 0x342: p = &(cpu.csr.mcause);  break; \
    default: panic("unsupported csr addr = 0x%03x", imm); \
  } \
  *old_ptr = *p; \
  *p &= ~v; \
} while (0)

#define CSR_SATP cpu.csr.satp
#define CSR_MSTATUS cpu.csr.mstatus
#define CSR_MTVEC cpu.csr.mtvec
#define CSR_MEPC cpu.csr.mepc
#define CSR_MCAUSE cpu.csr.mcause

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

#endif
