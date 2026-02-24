#include "Vtop_TopMiniRV.h"
#include "verilated.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_TopMiniRV* top = new Vtop_TopMiniRV;

    // 初始化信号
    top->clk = 0;
    top->rst_n = 0;

    // 复位
    for (int i = 0; i < 5; ++i) {
        top->clk = !top->clk;
        top->eval();
    }
    top->rst_n = 1;

    // 主循环
    for (int i = 0; i < 1000; ++i) {
        if (top->clk) {
            printf("PC = 0x%08x, inst = 0x%08x, exc = %d\n",
                top->PC_current, top->instruction, top->fetch_exception);
        }
        top->clk = !top->clk;
        top->eval();

    }

    delete top;
    return 0;
}