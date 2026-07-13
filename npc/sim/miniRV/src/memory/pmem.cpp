#include "common.h"
#include "monitor.h"
#include "Vtop_TopMiniRV.h"
#include "Vtop_TopMiniRV___024root.h"

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
    auto cpu = Simlator::instance;
    if (!cpu) { panic("Simulator instance is null!"); }
    if (!cpu->top) { panic("cpu->top is null!"); }

    // Verilator 5: 顶层 Vtop_TopMiniRV，内部状态在 rootp (Vtop_TopMiniRV___024root)
    auto* root = cpu->top->rootp;
    if (!root) { panic("Verilator rootp is null!"); }

    pmem = reinterpret_cast<uint8_t*>(&root->top_TopMiniRV__DOT__ram__DOT__MEM[0]);
    Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
    Log("pmem host ptr = %p (RAM.MEM)", pmem);
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
}
