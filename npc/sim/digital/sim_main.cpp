#include <iostream>
#include <vector>
#include <iomanip>
#include "Vtop_barrel_shifter.h"
#include "verilated.h"
#include "verilated_vcd_c.h" // 【关键步骤 1】引入波形头文件

#define SIM_CYCLES 10000

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_barrel_shifter* top = new Vtop_barrel_shifter;

    // 【关键步骤 2】开启追踪功能
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99); // 追踪深度，99表示记录所有子模块
    tfp->open("waveform.vcd"); // 保存的文件名

    std::vector<int> distribution(256, 0);
    vluint64_t main_time = 0; // 仿真时间戳

    // 1. 复位系统
    top->clk = 0;
    top->rst = 0;
    top->seed = 0x12;
    top->eval();
    tfp->dump(main_time++); // 【关键步骤 3】记录当前时刻波形

    top->clk = 1; top->eval();
    top->rst = 1; top->eval();
    tfp->dump(main_time++);

    // 2. 运行仿真
    std::cout << "Starting simulation..." << std::endl;
    for (int i = 0; i < SIM_CYCLES; i++) {
        // 时钟下降沿
        top->clk = 0; 
        top->eval();
        tfp->dump(main_time++); // 记录每个变化时刻

        // 时钟上升沿
        top->clk = 1; 
        top->eval();
        tfp->dump(main_time++);

        uint8_t val = top->dout;
        distribution[val]++;
    }

    // ... (打印统计结果的代码保持不变) ...

    // 【关键步骤 4】关闭波形文件并清理
    tfp->close();
    delete tfp;
    delete top;
    return 0;
}