#include <iostream>
#include <vector>
#include <iomanip>

#include "verilated.h"
#include "verilated_vcd_c.h" // 【关键步骤 1】引入波形头文件

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
// 移除硬编码的 #include "Vtop.h"
// 使用 Makefile 中定义的动态宏
#include TOSTRING(TOP_HEADER) 

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_example* top = new Vtop_example;

    // 【关键步骤 2】开启追踪功能
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99); // 追踪深度，99表示记录所有子模块
    std::string vcd_path = std::getenv("VCD_FILE") ? 
    std::getenv("VCD_FILE") : "waveform.vcd";
    tfp->open(vcd_path.c_str());

    vluint64_t main_time = 0; // 仿真时间戳

    // 1. 复位系统
    top->clk = 0;
    top->rst = 0;

    top->eval();
    tfp->dump(main_time++); // 【关键步骤 3】记录当前时刻波形

    top->clk = 1; top->eval();
    top->rst = 1; top->eval();
    tfp->dump(main_time++);

    uint8_t a = 1;
    uint8_t b = 1;


    // 2. 运行仿真
    std::cout << "Starting simulation..." << std::endl;
    for (int i = 0; i < SIM_CYCLES; i++) {
        a = rand() % 2;
        b = rand() % 2;
        top->a = a;
        top->b = b;

        // 时钟下降沿
        top->clk = 0; 
        top->eval();
        tfp->dump(main_time++); // 记录每个变化时刻

        // 时钟上升沿
        top->clk = 1; 
        top->eval();
        tfp->dump(main_time++);

        if(top->y != a ^ b) {
            std::cerr << "Mismatch at cycle " << i << ": a=" << (int)a 
                      << ", b=" << (int)b << ", y=" << (int)top->y 
                      << ", expected=" << (int)(a^b) << std::endl;
            assert(false);
        }
    }

    // ... (打印统计结果的代码保持不变) ...

    // 【关键步骤 4】关闭波形文件并清理
    tfp->close();
    delete tfp;
    delete top;
    return 0;
}