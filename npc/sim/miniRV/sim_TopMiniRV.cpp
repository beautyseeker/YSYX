#include "Vtop_TopMiniRV.h"
#include "verilated.h"
#include <capstone/capstone.h>
#define DEFAULT_SIM_CYCLES SIM_CYCLES
#define RING_BUFFER_SIZE 5

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

typedef struct {
    uint32_t PC_current;
    uint32_t instruction;
    char gpr[32][16]; // 32个寄存器，每个寄存器16字节（128位）宽
    char asm_str[32]; // 反汇编字符串
} cpu_state_t;

static const char *img_path = nullptr;
static const char *vcd_path = nullptr;
static Vtop_TopMiniRV *g_top = nullptr;
static cpu_state_t cpu_state;
static cpu_state_t ring_buffer[RING_BUFFER_SIZE]; // 环形缓冲区，保存最近5条指令的状态

#include <capstone/capstone.h>

//输入为文件路径字符串，输出为文件名字符串
inline const char* get_filename(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename) {
        return filename + 1; // 返回文件名部分
    } else {
        return path; // 如果没有路径分隔符，直接返回输入字符串
    }
}

void print_rv_disasm(uint32_t inst, uint32_t pc) {
    csh handle;
    cs_insn *insn;
    size_t count;

    cs_open(CS_ARCH_RISCV, CS_MODE_RISCV32, &handle);
    uint32_t reversed_inst = ((inst & 0xFF) << 24) | ((inst & 0xFF00) << 8) | ((inst & 0xFF0000) >> 8) | ((inst & 0xFF000000) >> 24);
    count = cs_disasm(handle, (uint8_t*)&reversed_inst, 4, pc, 1, &insn);
    // count = cs_disasm(handle, (uint8_t*)&inst, 4, pc, 1, &insn);

    if (count > 0) {
        printf("Assert error in %s PC:0x%08x: inst:0x%08x asm:%s %s\n", 
            get_filename(img_path), pc, reversed_inst, insn[0].mnemonic, insn[0].op_str);
        cs_free(insn, count);
    } else {
        printf("0x%08x: <invalid>\n", pc);
    }
    cs_close(&handle);
}

extern "C" void handle_sys_brk() {
    if (g_top) {
        printf("sys_brk invoked in %s! PC=0x%08x INST=0x%08x\n",
               get_filename(img_path), g_top->PC_current, g_top->instruction);
    } else {
        printf("sys_brk invoked in %s! (top not initialized)\n", get_filename(img_path));
    }
    exit(-1);
}

extern "C" void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
    if (g_top) {

        print_rv_disasm(g_top->instruction, g_top->PC_current);

        // for(int i = 0; i < RING_BUFFER_SIZE; i++) {
        //     printf("Recent instruction %d: PC=0x%08x INST=0x%08x\n",
        //            i,
        //            ring_buffer[i].PC_current,
        //            ring_buffer[i].instruction);
        // }
    } 
    else {
        printf("Memory access error at address: 0x%08x (top not initialized)\n", addr);
    }
    exit(-1);
}

extern "C" const char* get_img_path() {
    //获取img_path文件读入的指令数量，并打印出来
    if (img_path) {
        FILE *fp = fopen(img_path, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fclose(fp);
            printf("Load Image file from: %s, size: %ld bytes, instructions: %ld\n", img_path, size, size / 4);
        } else {
            printf("Failed to open image file: %s\n", img_path);
        }
    }
    return img_path ? img_path : "";
}
static const uint32_t RAM_BASE = 0x80000000;
static const uint32_t RAM_SIZE = 1 << 20; // 1MB

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_TopMiniRV* top = new Vtop_TopMiniRV;
    g_top = top;
    uint64_t arg_cycles = 0;
    g_top->gpr[2] = RAM_BASE + RAM_SIZE; // 初始化栈顶指针为RAM末尾地址
    g_top->gpr[3] = RAM_BASE; // 初始化全局指针为RAM起始地址
    g_top->gpr[4] = 0; // 初始化线程指针为0

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "IMG=", 4) == 0) {
            img_path = argv[i] + 4;
        }
        else if (strncmp(argv[i], "CYCLES=", 7) == 0) {
            arg_cycles = atoi(argv[i] + 7);
        }
        else if (strncmp(argv[i], "VCD=", 4) == 0) {
            vcd_path = argv[i] + 4;
        }
    }

    uint64_t MAX_CYCLES = arg_cycles != 0 ? arg_cycles : DEFAULT_SIM_CYCLES;

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
            printf("Simulation finished at cycle %d / %lu\n", i, MAX_CYCLES);
            break;
        }
        ring_buffer[i % 5].PC_current = top->PC_current;
        ring_buffer[i % 5].instruction = top->instruction;

        printf("pc:0x%08x inst:0x%08x  sim step: %d/%lu\n ", top->PC_current, top->instruction, i, MAX_CYCLES);
        top->clk = 0;
        top->eval();
    }

    delete top;
    return 0;
}