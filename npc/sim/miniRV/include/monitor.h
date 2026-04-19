#pragma once
#include "npc.h"
#include "Simlator.hpp"
#include "isa.h"

void print_statistic(Simlator* cpu);
void print_trap_state(Simlator* cpu, int state);


uint64_t get_uptime();
uint64_t get_time_internal();
