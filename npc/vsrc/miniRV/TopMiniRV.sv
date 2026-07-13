// `include "defs_pkg.sv"
import defs_pkg::*;

module top_TopMiniRV #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 24, REG_COUNT = 16, RESET_VEC = 32'h8000_0000)
(
    input logic                  clk,
    input logic                  rst_n,

    output logic [DATA_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] PC_next,
    output logic [DATA_WIDTH-1:0] instruction,
    output logic [DATA_WIDTH-1:0] gpr [REG_COUNT-1:0], // 输出整个寄存器文件状态，便于调试
    output CSR_bundle_out csr_bundle,
    output logic                  fire
);
    // 模块实例化
    localparam REG_ADDR_WIDTH = $clog2(REG_COUNT); // 寄存器地址宽度，根据寄存器数量计算
    logic [REG_ADDR_WIDTH-1:0] Rs1_addr, Rs2_addr, Rd_addr;
    logic [DATA_WIDTH-1:0] Rs1_data, Rs2_data, Rd_data;
    logic lsu_ready;
    logic [DATA_WIDTH-1:0] alu_result;
    logic ALU_zero;
    logic [DATA_WIDTH-1:0] mem_load_data;
    logic [DATA_WIDTH-1:0] imm_ext;
    logic [DATA_WIDTH-1:0] CSR_reg;


    Ctrl_sig_t ctrl_sig;
    logic idu_ready;
    logic ifu_valid;

    IFU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH), .RESET_VEC(RESET_VEC)) ifu (
        .clk(clk),
        .rst_n(rst_n),
        .idu_ready(idu_ready),

        .PC_current(PC_current),
        .PC_next(PC_next),
        .instruction(instruction),
        .ifu_valid(ifu_valid)
    );
    assign fire = ifu_valid && idu_ready;

    IDU #(.DATA_WIDTH(DATA_WIDTH), .REG_ADDR_WIDTH(REG_ADDR_WIDTH)) idu (
        .clk(clk),
        .rst_n(rst_n),
        .lsu_ready(lsu_ready),
        .inst(instruction),
        .ifu_valid(ifu_valid),
        .rs1_addr(Rs1_addr),
        .rs2_addr(Rs2_addr),
        .rd_addr(Rd_addr),
        .imm(imm_ext),
        .ctrl_sig(ctrl_sig),
        .idu_ready(idu_ready)
    );

    RegisterFile #(.DATA_WIDTH(DATA_WIDTH), .REG_COUNT(REG_COUNT)) regfile (
        .clk(clk),
        .rst_n(rst_n),
        .rs1_addr(Rs1_addr),
        .rs2_addr(Rs2_addr),
        .rd_addr(Rd_addr),
        .rs1_data(Rs1_data),
        .rs2_data(Rs2_data),
        .write_data(Rd_data),
        .reg_write_en(ctrl_sig.reg_write_en && fire),
        .gpr(gpr)
    );

    logic [DATA_WIDTH-1:0] alu_a;
    logic [DATA_WIDTH-1:0] alu_b;
    always_comb begin : ALU_b_src_sel
        case(ctrl_sig.ALU_b_src_sel)
            B_SRC_REG: alu_b = Rs2_data;
            B_SRC_IMM: alu_b = imm_ext;
            default: alu_b = 'x; // 不应该发生
        endcase
        assert (ctrl_sig.ALU_b_src_sel inside {B_SRC_REG, B_SRC_IMM, B_SRC_PC})
        else $error("Invalid ALU_b_src_sel: %0d at time %t", ctrl_sig.ALU_b_src_sel, $time);
    end

    always_comb begin : ALU_a_src_sel
        case(ctrl_sig.ALU_a_src_sel)
            A_SRC_REG: alu_a = Rs1_data;
            A_SRC_IMM: alu_a = imm_ext;
            A_SRC_PC: alu_a = $unsigned(PC_current);
            default: alu_a = 'x; // 不应该发生
        endcase
        assert (ctrl_sig.ALU_a_src_sel inside {A_SRC_REG, A_SRC_IMM, A_SRC_PC})
        else $error("Invalid ALU_a_src_sel: %0d at time %t", ctrl_sig.ALU_a_src_sel, $time);
    end

    EXU #(.DATA_WIDTH(DATA_WIDTH)) exu (
        .alu_a(alu_a),
        .alu_b(alu_b),
        .ALU_op(ctrl_sig.ALU_op),
        .alu_zero(ALU_zero),
        .ALU_result(alu_result)
    );

    CSRFile #(.XLEN(DATA_WIDTH)) csrfile (
        .clk(clk),
        .rst_n(rst_n),
        .ctrl_sig(ctrl_sig),
        .csr_instruction(instruction),
        .PC_current(PC_current),
        .CSR_RS1(Rs1_data),
        .CSR_imm(imm_ext),
        .csr_write_en(ctrl_sig.WB_sel == CSR && ctrl_sig.reg_write_en),
        .csr_read_out(CSR_reg),
        .csr_bundle_out(csr_bundle)
    );

    logic [DATA_WIDTH-1:0]      x_addr;
    logic [DATA_WIDTH-1:0]      x_wdata;
    logic [3:0]                 x_mask;
    logic                       x_wen;
    logic                       x_reqValid;
    logic                       x_respReady;

    logic [DATA_WIDTH-1:0]      x_rdata;
    logic                       x_respValid;
    logic                       x_reqReady;
    logic [1:0]                 x_err;

    logic [DATA_WIDTH-1:0]      ram_addr;
    logic [DATA_WIDTH-1:0]      ram_wdata;
    logic [3:0]                 ram_mask;
    logic                       ram_wen;
    logic                       ram_reqValid;
    logic                       ram_respReady;
    logic [DATA_WIDTH-1:0]      ram_rdata;
    logic                       ram_respValid;
    logic                       ram_reqReady;
    logic [1:0]                 ram_err;

    logic [DATA_WIDTH-1:0]      uart_addr;
    logic [DATA_WIDTH-1:0]      uart_wdata;
    logic [3:0]                 uart_mask;
    logic                       uart_wen;
    logic                       uart_reqValid;
    logic                       uart_respReady;
    logic [DATA_WIDTH-1:0]      uart_rdata;
    logic                       uart_respValid;
    logic                       uart_reqReady;
    logic [1:0]                 uart_err;

    logic [DATA_WIDTH-1:0]      timer_addr;
    logic [DATA_WIDTH-1:0]      timer_wdata;
    logic [3:0]                 timer_mask;
    logic                       timer_wen;
    logic                       timer_reqValid;
    logic                       timer_respReady;
    logic [DATA_WIDTH-1:0]      timer_rdata;
    logic                       timer_respValid;
    logic                       timer_reqReady;
    logic [1:0]                 timer_err;

    LSU #(.XLEN(DATA_WIDTH)) lsu (
        .clk(clk),
        .rst_n(rst_n),
        .addr(alu_result), // 地址由ALU计算得到
        .store_data(Rs2_data), // 存储数据来自寄存器
        .mem_size(ctrl_sig.mem_size), // 根据指令类型设置
        .mem_sign(ctrl_sig.mem_sign), // 根据指令类型设置
        .mem_write_en(ctrl_sig.mem_write_en),
        .mem_read_en(ctrl_sig.mem_read_en),
        .ifu_valid(ifu_valid),

        .x_addr(x_addr),
        .x_wdata(x_wdata),
        .x_mask(x_mask),
        .x_wen(x_wen),
        .x_reqValid(x_reqValid),
        .x_respReady(x_respReady),

        .x_rdata(x_rdata),
        .x_respValid(x_respValid),
        .x_reqReady(x_reqReady),
        .x_err(x_err),

        .load_data(mem_load_data),
        .lsu_ready(lsu_ready)
    );


    Xbar #(.XLEN(DATA_WIDTH)) xbar (
        .clk(clk),
        .rst_n(rst_n),

        // 主机请求通道
        .m_addr(x_addr),
        .m_reqValid(x_reqValid),
        .m_wen(x_wen),
        .m_wdata(x_wdata),
        .m_mask(x_mask),
        .m_respReady(x_respReady),

        .m_rdata(x_rdata),
        .m_respValid(x_respValid),
        .m_reqReady(x_reqReady),
        .m_err(x_err),

        // 从机RAM端口
        .ram_addr(ram_addr),
        .ram_reqValid(ram_reqValid),
        .ram_wen(ram_wen),
        .ram_wdata(ram_wdata),
        .ram_mask(ram_mask),
        .ram_respReady(ram_respReady),
        .ram_rdata(ram_rdata),
        .ram_respValid(ram_respValid),
        .ram_reqReady(ram_reqReady),
        .ram_err(ram_err),

        // 从机UART端口
        .uart_addr(uart_addr),
        .uart_reqValid(uart_reqValid),
        .uart_wen(uart_wen),
        .uart_wdata(uart_wdata),
        .uart_mask(uart_mask),
        .uart_respReady(uart_respReady),
        .uart_rdata(uart_rdata),
        .uart_respValid(uart_respValid),
        .uart_reqReady(uart_reqReady),
        .uart_err(uart_err),

        // 从机Timer端口
        .timer_addr(timer_addr),
        .timer_reqValid(timer_reqValid),
        .timer_wen(timer_wen),
        .timer_wdata(timer_wdata),
        .timer_mask(timer_mask),
        .timer_respReady(timer_respReady),
        .timer_rdata(timer_rdata),
        .timer_respValid(timer_respValid),
        .timer_reqReady(timer_reqReady),
        .timer_err(timer_err)
    );

    RAM #(.XLEN(DATA_WIDTH)) ram (
        .clk(clk),
        .rst_n(rst_n),
        .addr(ram_addr),
        .wdata(ram_wdata),
        .mask(ram_mask),
        .wen(ram_wen),
        .reqValid(ram_reqValid),
        .respReady(ram_respReady),

        .rdata(ram_rdata),
        .respValid(ram_respValid),
        .reqReady(ram_reqReady),
        .err(ram_err)
    );

    UART #(.XLEN(DATA_WIDTH)) uart (
        .clk(clk),
        .rst_n(rst_n),
        .addr(uart_addr),
        .wdata(uart_wdata),
        .mask(uart_mask),
        .wen(uart_wen),
        .reqValid(uart_reqValid),
        .respReady(uart_respReady),
        .rdata(uart_rdata),
        .respValid(uart_respValid),
        .reqReady(uart_reqReady),
        .err(uart_err)
    );

    Timer #(.XLEN(DATA_WIDTH)) rtc (
        .clk(clk),
        .rst_n(rst_n),
        .addr(timer_addr),
        .wdata(timer_wdata),
        .mask(timer_mask),
        .wen(timer_wen),
        .reqValid(timer_reqValid),
        .respReady(timer_respReady),
        .rdata(timer_rdata),
        .respValid(timer_respValid),
        .reqReady(timer_reqReady),
        .err(timer_err)
    );

    WBU #(.XLEN(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) wbu (
        .alu_result(alu_result),
        .mem_load_data(mem_load_data),
        .ctrl_sig(ctrl_sig),
        .PC_current(PC_current),
        .PC_rel_imm(imm_ext),
        .CSR_tvec(csr_bundle.CSR_tvec),
        .CSR_epc(csr_bundle.CSR_epc),
        .instruction(instruction),
        .CSR_data(CSR_reg),
        .WB_data(Rd_data),
        .PC_next(PC_next)
    );
endmodule
