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

#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  if(cpu.satp >> _riscv_xlen - 1) {
    return MMU_TRANSLATE;
  }
  return MMU_DIRECT;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  word_t vpn1 = (vaddr >> 22) & 0x3ff;
  word_t vpn0 = (vaddr >> 12) & 0x3ff;
  word_t offset = vaddr & 0xfff;

  // 1. 读取一级页表项
  paddr_t pte1_addr = (cpu.satp.ppn << 12) + vpn1 * 4;
  word_t pte1 = paddr_read(pte1_addr, 4);
  assert(pte1 & PTE_V); // 实际应处理 Page Fault

  // 2. 假设是二级页表结构
  word_t pte1_ppn = pte1 >> 10;
  paddr_t pte0_addr = (pte1_ppn << 12) + vpn0 * 4;
  word_t pte0 = paddr_read(pte0_addr, 4);
  assert(pte0 & PTE_V);

  // 3. 检查权限 (type 为访问类型：R, W, X)
  // if (!(pte0 & type_mask)) raise_exception(...);

  // 4. 组合物理地址
  return (pte0 >> 10 << 12) | offset;
}
