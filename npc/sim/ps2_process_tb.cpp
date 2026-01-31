// Verilator testbench for ps2_process module
// 验证状态机能否正确处理 PS/2 扫描码（通码 make code 和 断码 break code）

#include "verilated.h"
#include "Vtop_ps2keyboard.h" 
#include <verilated_vcd_c.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// 测试向量结构体
struct TestCase {
    uint8_t ps2_data;
    uint8_t expected_keycode;
    uint8_t expected_nextdata_n;
    const char* desc;
};

// 仿真时间和上下文
vluint64_t sim_time = 0;
VerilatedContext* contextp = nullptr;
Vtop_ps2keyboard* top = nullptr;
VerilatedVcdC* tfp = nullptr;

// 辅助函数：推进时钟
void tick() {
    top->clk = 0; top->eval();
    if (tfp) tfp->dump(sim_time++);
    top->clk = 1; top->eval();
    if (tfp) tfp->dump(sim_time++);
}

// 辅助函数：复位
void reset() {
    top->rst = 0; // 低电平复位
    for (int i = 0; i < 10; ++i) tick();
    top->rst = 1;
    tick();
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    contextp = new VerilatedContext;
    top = new Vtop_ps2keyboard{contextp};

    // 启用波形
    Verilated::traceEverOn(true);
    tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("build/waveforms/ps2_process_tb.vcd");

    printf("========== ps2_process 仿真开始 ==========\n");

    // 复位
    reset();

    // 注意：top_ps2keyboard 顶层并没有直接暴露 ps2_process 的内部端口
    // 它封装了 ps2_keyboard 和 ps2_process。
    // 为了验证 ps2_process，我们需要给 ps2_keyboard 提供 ps2_clk 和 ps2_data 信号，
    // 模拟真实的键盘输入。

    // PS/2 协议参数
    const int PS2_CLK_PERIOD = 20; // 模拟 PS/2 时钟周期 (基于 tick)

    // 发送一个字节到 PS/2 接口的函数
    auto send_ps2_byte = [&](uint8_t data) {
        printf("Sending PS/2 byte: 0x%02X\n", data);
        
        uint16_t frame = 0;
        frame |= (0 << 0);           // Start bit (0)
        frame |= (data << 1);        // Data bits
        
        // 奇校验
        int ones = 0;
        for(int i=0; i<8; i++) if((data >> i) & 1) ones++;
        int parity = (ones % 2 == 0) ? 1 : 0; // Odd parity
        
        frame |= (parity << 9);      // Parity bit
        frame |= (1 << 10);          // Stop bit (1)

        // 模拟串行发送 (11 bits)
        for (int i = 0; i < 11; i++) {
            int bit = (frame >> i) & 1;
            
            // PS/2 时钟下降沿采样
            top->ps2_data = bit;
            top->ps2_clk = 1; 
            for(int k=0; k<PS2_CLK_PERIOD/2; k++) tick(); // wait
            
            top->ps2_clk = 0; // Negative edge
            for(int k=0; k<PS2_CLK_PERIOD/2; k++) tick(); // wait
        }
        
        // Idle state
        top->ps2_clk = 1;
        top->ps2_data = 1;
        for(int i=0; i<50; i++) tick(); // Wait for internal processing
    };

    // --- 测试 1: 按下 'A' (1C) ---
    // 预期: ps2_keyboard 接收到 1C -> ready=1
    // -> ps2_process 检测到非 F0 -> OUTPUT 状态 -> keycode=1C -> nextdata_n=0
    send_ps2_byte(0x1C);
    
    // 检查结果 (由于是顶层模块，我们检查导出给七段数码管的 keycode 信号对应的中间逻辑比较困难)
    // 但可以通过观察波形确认 keycode 信号是否有变化
    // 如果想要在 C++ 中自动 verify，需要在顶层把 ps2_process 的 keycode 输出暴露出来，
    // 或者我们直接把这个 tb 文件针对 ps2_process 单独编译（需要单独的 Wrapper 或修改 Makefile）。
    
    // 这里我们假设你是想仿真整个键盘输入处理链路。
    // 如果只想测 ps2_process，建议专门写一个只包含 ps2_process 的 top wrapper。
    
    // 由于当前 top_ps2keyboard 没有直接输出 keycode 原始值（只有七段数码管编码），
    // 我们可以通过肉眼观察仿真打印或波形。
    // 不过，为了演示，我们可以通过 DPI 或者 Verilator public 访问内部信号（较复杂），
    // 或者简单地多跑几个 case 观察波形。

    tick(); tick();

    // --- 测试 2: 松开 'A' (F0, 1C) ---
    // 1. 发送 F0 (Break code)
    // 预期: ps2_process 进入 SKIP 状态，不输出 keycode
    send_ps2_byte(0xF0);

    // 2. 发送 1C
    // 预期: ps2_process 在 SKIP 状态接收数据，回到 IDLE，nextdata_n=0，不更新 keycode (保持 00 或之前值?)
    // 根据代码逻辑：SKIP 状态下 ready 有效 -> next_state = IDLE, nextdata_n=0
    // keycode = (curr_state == OUTPUT) ? ps2_data : 8'h00;
    // 所以 keycode 应该变为 00。
    send_ps2_byte(0x1C);

    // --- 测试 3: 按下 'B' (32) ---
    send_ps2_byte(0x32);

    printf("仿真结束，请查看波形文件 build/waveforms/ps2_process_tb.vcd\n");

    // 结束
    tfp->close();
    delete top;
    delete contextp;
    return 0;
}
