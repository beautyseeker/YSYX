#include "common.h"
#include "monitor.h"

static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};

word_t pmem_read(paddr_t addr, int len) {
    switch (len) {
        case 1: return *(uint8_t  *)(pmem + addr - CONFIG_MBASE);
        case 2: return *(uint16_t *)(pmem + addr - CONFIG_MBASE);
        case 4: return *(uint32_t *)(pmem + addr - CONFIG_MBASE);
        IFDEF(CONFIG_ISA64, case 8: return *(uint64_t *)(pmem + addr - CONFIG_MBASE));
        default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
    }
}

void pmem_write(paddr_t addr, int len, word_t data) {
    switch (len) {
        case 1: *(uint8_t  *)(pmem + addr - CONFIG_MBASE) = data; return;
        case 2: *(uint16_t *)(pmem + addr - CONFIG_MBASE) = data; return;
        case 4: *(uint32_t *)(pmem + addr - CONFIG_MBASE) = data; return;
        IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)(pmem + addr - CONFIG_MBASE) = data; return);
        IFDEF(CONFIG_RT_CHECK, default: assert(0));
    }
}

extern void print_trap_state(Simlator* cpu, int state);

extern "C" {
    void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
        auto cpu = Simlator::instance;
        print_trap_state(cpu, -1);
        SIMERROR("Memory access error at address: 0x%08x, mapped address: 0x%08x\n", addr, mapped_addr);
        cpu->set_state(SimState::ABORT);
    }

    void handle_sys_brk() {
        auto cpu = Simlator::instance;
        print_trap_state(cpu, 0);
        cpu->set_state(SimState::END);
    }

    void register_pmem_args(svOpenArrayHandle ptr, uint32_t size, uint32_t base) {
        if (size > CONFIG_MSIZE) {
            SIMERROR("PMEM size exceeds the configured limit: %u > %u\n", size, CONFIG_MSIZE);
            return;
        }
        if (base != CONFIG_MBASE) {
            SIMERROR("PMEM base address mismatch: 0x%08x != 0x%08x\n", base, CONFIG_MBASE);
            return;
        }
        // 将 Verilator 传入的 PMEM 指针与本地 pmem 数组关联
        // uint8_t* external_pmem = (uint8_t*)ptr;
        // for (uint32_t i = 0; i < size; i++) {
        //     external_pmem[i] = pmem[i];
        // }
    }
}
