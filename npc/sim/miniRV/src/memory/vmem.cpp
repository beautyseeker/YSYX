#include "common.h"

extern word_t paddr_read(paddr_t addr, int len);
extern void pmem_write(paddr_t addr, int len, word_t data);
extern bool in_mrom(paddr_t addr);
extern bool in_sram(paddr_t addr);
/* DPI / pmem.cpp 里是 extern "C"，声明必须一致，否则链接找的是 C++ 修饰名 */
extern "C" void mrom_read(int32_t addr, int32_t *data);

word_t vaddr_ifetch(vaddr_t addr, int len) {
    int32_t inst;
    mrom_read((int32_t)addr, &inst);
    return (word_t)inst;
}

/* sdb / 表达式求值：按地址落到 MROM 或 SRAM，不能一律当 SRAM */
word_t vaddr_read(vaddr_t addr, int len) {
    return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
    Assert(in_sram(addr), "vaddr_write: 0x%08x not in SRAM (MROM is read-only)", (uint32_t)addr);
    pmem_write(addr, len, data);
}
