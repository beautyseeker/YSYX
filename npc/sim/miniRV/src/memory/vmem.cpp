#include "common.h"

extern word_t pmem_read(paddr_t addr, int len);
extern void pmem_write(paddr_t addr, int len, word_t data);

word_t vaddr_ifetch(vaddr_t addr, int len) {
    return pmem_read(addr, len);
}

word_t vaddr_read(vaddr_t addr, int len) {
    return pmem_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
    pmem_write(addr, len, data);
}


