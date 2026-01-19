// Verilated -*- C++ -*-
// DESCRIPTION: main() calling loop, created with Verilator --main

#include "verilated.h"
#include "Vtop.h"
#include <stdlib.h>
#include <stdio.h>
#include <verilated_vcd_c.h>
#include <string>
#include <filesystem>
#include <nvboard.h>


static TOP_NAME dut;
void nvboard_bind_all_pins(TOP_NAME* top);
void vcd_init(VerilatedVcdC* tfp, VerilatedContext* contextp, Vtop* topp, const char* exe_path);

//======================

int main(int argc, char** argv, char**) {
    Verilated::debug(0);
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->commandArgs(argc, argv);
    VerilatedVcdC* tfp = new VerilatedVcdC;

    vluint64_t max_time = 0;
    if(argc > 1) {
        max_time = atoi(argv[1]);
    }

    const std::unique_ptr<Vtop> topp{new Vtop{contextp.get()}};
    if(max_time > 0) {
      vcd_init(tfp, contextp.get(), topp.get(), argv[0]);
    }


    nvboard_bind_all_pins(&dut);
    nvboard_init();

    uint8_t a = 0;
    uint8_t b = 0;

    while (!contextp->gotFinish()) {
        a = rand() & 1;
        b = rand() & 1;
        // a = nvboard_get_pin_value("TOP.a");
        // b = nvboard_get_pin_value("TOP.b");
        topp->a = a;
        topp->b = b;
        topp->eval();
        
        if(contextp->time() <= max_time)
          tfp->dump(contextp->time());
        VL_DEBUG_IF(VL_PRINTF("+ Time: %llu ns | a=%d b=%d f=%d\n",
                                contextp->time(), topp->a, topp->b, topp->f););
        assert(topp->f == (a ^ b));

        dut.eval();
        nvboard_update();
        contextp->timeInc(1);
    }

    topp->final();
    if(max_time > 0 && tfp != nullptr) {
      tfp->close();
      delete tfp;
    }
    return 0;
}

void vcd_init(VerilatedVcdC* tfp, VerilatedContext* contextp, Vtop* topp, const char* exe_path) {
    contextp->traceEverOn(true);
    topp->trace(tfp, 99);
    std::string exe_name = std::filesystem::path(exe_path).filename().string();
    std::string vcd_name = "build/waveforms/" + exe_name + ".vcd";
    tfp->open(vcd_name.c_str());
}