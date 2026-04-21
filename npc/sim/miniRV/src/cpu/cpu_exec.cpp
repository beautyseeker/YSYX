#include "Simlator.hpp"
#include "npc.h"

extern void difftest_step(vaddr_t npc_pc, vaddr_t npc_next_pc);

void trace_and_difftest(NPC_State *_this) {
    IFDEF(CONFIG_DIFFTEST, difftest_step(_this->current_pc, _this->next_pc););
    IFDEF(CONFIG_ITRACE, puts(_this->logbuf););
    IFDEF(WATCHPOINT, wp_scan_wp(););
}

void cpu_exec(uint64_t n) {
    auto cpu = Simlator::instance;
    if(cpu == nullptr) {
        SIMERROR("Simulator instance is not initialized!");
        return;
    }
    switch(cpu->get_state()) {
        case END: case ABORT: case QUIT:
        printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
        return;
        default: cpu->set_state(RUNNING); break;
    }

    cpu->clock_step(n);
    trace_and_difftest(cpu->npc_state);
}
