// Verilated -*- C++ -*-
// DESCRIPTION: main() calling loop, created with Verilator --main

#include "verilated.h"
#include "Vtop.h"
#include <stdlib.h>
#include <stdio.h>
#include <verilated_vcd_c.h>
#include <string>
#include <filesystem>

//======================

int main(int argc, char** argv, char**) {
    // Setup context, defaults, and parse command line
    Verilated::debug(0);
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->commandArgs(argc, argv);

    // Construct the Verilated model, from Vtop.h generated from Verilating
    const std::unique_ptr<Vtop> topp{new Vtop{contextp.get()}};
    VerilatedVcdC* tfp = new VerilatedVcdC;
    contextp->traceEverOn(true);
    topp->trace(tfp, 99);
    
    std::string exe_name = std::filesystem::path(argv[0]).filename().string();
    std::string vcd_name = "build/waveforms/" + exe_name + ".vcd";
    tfp->open(vcd_name.c_str());

    uint8_t a = 0;
    uint8_t b = 0;
    vluint64_t max_time = 5000;
    if(argc > 1) {
        max_time = atoi(argv[1]);
    }

    // Simulate until $finish or max_time reached
    while (!contextp->gotFinish() && contextp->time() < max_time) {
        // Evaluate model
        a = rand() & 1;
        b = rand() & 1;
        topp->a = a;
        topp->b = b;
        topp->eval();
        tfp->dump(contextp->time());
        VL_DEBUG_IF(VL_PRINTF("+ Time: %llu ns | a=%d b=%d f=%d\n",
                                contextp->time(), topp->a, topp->b, topp->f););
        
        
        assert(topp->f == (a ^ b));
        // Advance time
        contextp->timeInc(1);

    }

    if (!contextp->gotFinish()) {
        VL_DEBUG_IF(VL_PRINTF("+ Exiting without $finish; no events left\n"););
    }

    // Final model cleanup
    topp->final();
    tfp->close();
    return 0;
}
