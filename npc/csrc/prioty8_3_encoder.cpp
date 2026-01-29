// Verilated -*- C++ -*-
// DESCRIPTION: main() calling loop, created with Verilator --main

#include "verilated.h"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// 移除硬编码的 #include "Vtop.h"
// 使用 Makefile 中定义的动态宏
#include TOSTRING(TOP_HEADER) 

#include <stdlib.h>
#include <stdio.h>
#include <verilated_vcd_c.h>
#include <string>
#include <filesystem>
#include <nvboard.h>


static TOP_NAME dut; 
void single_cycle();
void reset(int n);
void nvboard_bind_all_pins(TOP_NAME* top);
void vcd_init(VerilatedVcdC* tfp, VerilatedContext* contextp, TOP_NAME* topp, const char* exe_path);

//======================

int main(int argc, char** argv, char**) {
    Verilated::debug(0);
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->commandArgs(argc, argv);
    
    // 使用宏实例化
    const std::unique_ptr<TOP_NAME> topp{new TOP_NAME{contextp.get()}};

    nvboard_bind_all_pins(&dut);
    nvboard_init();
    reset(10);

    while (!contextp->gotFinish()) {
        single_cycle();
        nvboard_update();
        contextp->timeInc(1);
    }

    topp->final();
    return 0;
}

// 函数签名中的类型也要修改
void vcd_init(VerilatedVcdC* tfp, VerilatedContext* contextp, TOP_NAME* topp, const char* exe_path) {
    contextp->traceEverOn(true);
    topp->trace(tfp, 99);
    std::string exe_name = std::filesystem::path(exe_path).filename().string();
    std::string vcd_name = "build/waveforms/" + exe_name + ".vcd";
    tfp->open(vcd_name.c_str());
}

void single_cycle() {
    dut.clk = 0; dut.eval();
    dut.clk = 1; dut.eval();
}

void reset(int n) {
    dut.rst = 1;
    while (n -- > 0) single_cycle();
    dut.rst = 0;
}
