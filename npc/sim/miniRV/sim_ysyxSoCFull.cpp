#include "dut_top.h"
#include "Simlator.hpp"

void init_monitor(int argc, char *argv[]);

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    DutTop* top = new DutTop;
    auto cpu = Simlator(top);
    cpu.init(argc, argv);
    printf("ysyxSoCFull initialized.\n");
    init_monitor(argc, argv);
    printf("monitor initialized.\n");
    printf("Should not reach here.\n");
    return 0;
}
