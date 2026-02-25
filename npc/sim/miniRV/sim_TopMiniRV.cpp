#include "Vtop_TopMiniRV.h"
#include "verilated.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};


int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_TopMiniRV* top = new Vtop_TopMiniRV;
    const char *sim_cycles_env = std::getenv("SIM_CYCLES");
    uint64_t MAX_CYCLES = sim_cycles_env ? std::atoi(sim_cycles_env) : SIM_CYCLES;

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
    uint32_t gpr_mirror[32] = {0}; // 寄存器镜像备份

for (int i = 0; i < MAX_CYCLES; ++i) {

    top->clk = 1;
    top->eval();
    bool changed = false;
    // 检查是否有任何寄存器发生了变化
    for (int r = 0; r < 32; r++) {
        if (top->gpr[r] != gpr_mirror[r]) {
            changed = true;
            break;
        }
    }

    if (changed) {

        for (int r = 0; r < 32; r++) {
            if (top->gpr[r] != gpr_mirror[r]) {
                printf("[%s:0x%08x] ", regs[r], top->gpr[r]);
                gpr_mirror[r] = top->gpr[r];
            }
        }
        printf("\n");
    }
    if(Verilated::gotFinish()) {
        printf("Simulation finished at cycle %d / %llu\n", i, MAX_CYCLES);
        break;
    }

    printf("pc:0x%08x inst:0x%08x\n ", top->PC_current, top->instruction);
    top->clk = 0;
    top->eval();
}

    delete top;
    return 0;
}