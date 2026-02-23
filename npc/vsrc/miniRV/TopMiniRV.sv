`include "defs_pkg.sv"
import defs_pkg::*;

module TopMiniRV #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 18)
(
    input logic                  clk,
    input logic                  rst_n,

    output logic [ADDR_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] instruction,
    output exception_t            fetch_exception
);
    // 模块实例化
    localparam REG_ADDR_WIDTH = 5; // 寄存器地址宽度，RISC-V有32个寄存器
    logic [REG_ADDR_WIDTH-1:0] Rs1_addr, Rs2_addr, Rd_addr;
    logic [DATA_WIDTH-1:0] Rs1_data, Rs2_data, Rd_data;
    logic [DATA_WIDTH-1:0] alu_result;
    logic [DATA_WIDTH-1:0] mem_load_data;
    logic [DATA_WIDTH-1:0] imm_ext;


    Ctrl_sig_t ctrl_sig;

    IFU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) ifu (
        .clk(clk),
        .rst_n(rst_n),
        .PC_next(jmp_target), // 这里简单地将当前PC作为下一个PC，实际设计中会更复杂
        .PC_current(PC_current),
        .instruction(instruction),
        .exception(fetch_exception)
    );

    RegisterFile #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(5)) regfile (
        .clk(clk),
        .rst_n(rst_n),
        .rs1_addr(Rs1_addr),
        .rs2_addr(Rs2_addr),
        .rd_addr(Rd_addr),
        .rs1_data(Rs1_data),
        .rs2_data(Rs2_data),
        .rd_data(Rd_data), // 写回数据暂时不连接
        .reg_write_en(ctrl_sig.reg_write_en) // 写使能暂时不连接
    );

    IDU idu (
        .inst(instruction),
        .rs1_addr(Rs1_addr),
        .rs2_addr(Rs2_addr),
        .rd_addr(Rd_addr),
        .imm(imm_ext),
        .ctrl_sig(ctrl_sig)
    );

    EXU #(.DATA_WIDTH(DATA_WIDTH)) exu (
        .alu_a(Rs1_data),
        .alu_b(Rs2_data),
        .ALU_op(ctrl_sig.ALU_op),
        .alu_zero(ALU_zero),
        .ALU_result(alu_result)
    );

    logic ALU_zero;
    logic branch_taken;
    assign branch_taken = (ctrl_sig.branch_en && ALU_zero); // 简单的分支判断，实际设计中可能更复杂
    logic [ADDR_WIDTH-1:0] jmp_target;

    always_comb begin
        if(branch_taken)
            case(ctrl_sig.PC_sel)
                PC_PLUS4: jmp_target = PC_current + 4;
                PC_BRANCH: jmp_target = PC_current + imm_ext[ADDR_WIDTH-1:0]; // 分支目标地址
                PC_JALR: jmp_target = imm_ext[ADDR_WIDTH-1:0]; // 长跳目标地址
                default: jmp_target = PC_current + 4;
            endcase
        else
            jmp_target = PC_current + 4; // 默认顺序执行
    end

    LSU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) lsu (
        .clk(clk),
        .rst_n(rst_n),
        .addr(alu_result), // 地址由ALU计算得到
        .store_data(Rs2_data), // 存储数据来自寄存器
        .mem_size(ctrl_sig.mem_size), // 根据指令类型设置
        .mem_sign(ctrl_sig.mem_sign), // 根据指令类型设置
        .mem_write_en(ctrl_sig.mem_write_en),
        .mem_read_en(ctrl_sig.mem_read_en),
        .load_data(mem_load_data),
        .mem_exception(fetch_exception)
    );
    WBU #(.DATA_WIDTH(DATA_WIDTH)) wbu (
        .alu_result(alu_result),
        .mem_load_data(mem_load_data),
        .ctrl_sig(ctrl_sig),
        .rd_data_out(Rd_data) // 写回数据暂时不连接
    );
endmodule
