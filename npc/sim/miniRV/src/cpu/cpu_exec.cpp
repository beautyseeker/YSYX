#include "Simlator.hpp"
#include "npc.h"

void cpu_exec(uint64_t n) {
    if(Simlator::instance) {
        Simlator::instance->clock_step(n);
    } else {
        SIMERROR("Simulator instance is not initialized!");
    }
}
