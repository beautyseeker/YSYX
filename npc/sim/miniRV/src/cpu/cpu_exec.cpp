#include "Simlator.hpp"
#include "npc.h"

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

    cpu->execute(n);
}
