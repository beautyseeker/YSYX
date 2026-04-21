#include "Vtop_TopMiniRV.h"
#include "Simlator.hpp"

void init_monitor(int argc, char *argv[]);

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_TopMiniRV* top = new Vtop_TopMiniRV;
    auto cpu = Simlator(top);
    cpu.init(argc, argv);
    init_monitor(argc, argv);
    printf("Should not reach here.\n");
    return 0;
}
