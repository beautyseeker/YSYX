#include "common.h"
#include "monitor.h"
#include "Simlator.hpp"
#include <cstring>

static uint8_t* pmem = nullptr;

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

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

void init_mem() {
    // SoC 模式下不再绑定 RTL 内 RAM.MEM；host 侧独占 pmem（DiffTest / SDB / load_img）
    if (pmem == nullptr) {
        pmem = (uint8_t*)malloc(CONFIG_MSIZE);
        Assert(pmem, "Cannot allocate host pmem, size = %d", CONFIG_MSIZE);
        memset(pmem, 0, CONFIG_MSIZE);
    }
    Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
    Log("pmem host ptr = %p (malloc)", pmem);
    Log("Memory Trace: %s", MUXDEF(CONFIG_MTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
}

extern void print_trap_state(Simlator* cpu, int state);

extern "C" {
    void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
        auto cpu = Simlator::instance;
        print_trap_state(cpu, -1);
        SIMERROR("%s Memory access error at address: 0x%08x, mapped address: 0x%08x\n",
                cpu->get_img_name(), addr, mapped_addr);
        cpu->set_state(SimState::ABORT);
        exit(-1);
    }

    void handle_sys_brk() {
        auto cpu = Simlator::instance;
        print_trap_state(cpu, cpu->get_gpr(10));
        cpu->set_state(SimState::END);
    }

    // SoC DPI：后续按讲义接 flash/mrom 镜像；先保证可链接、不立刻 assert
    void flash_read(int32_t addr, int32_t *data) {
        // flash XIP 地址空间在 SoC 侧；此处暂返回 0（nop），避免未实现就 fatal
        *data = 0;
        (void)addr;
        assert(0);
    }

    void mrom_read(int32_t addr, int32_t *data) {
        if(addr >= 0x20000000 && addr < 0x20001000) {
            *data = 0x00100073;
        } else {
            *data = 0;
            assert(0);
        }
    }
}
