import defs_pkg::*;

module top_TopMiniRV #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 24, REG_COUNT = 16, RESET_VEC = 32'h8000_0000)
(
    input logic                  clk,
    input logic                  rst_n,

    output logic [DATA_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] PC_next,
    output logic [DATA_WIDTH-1:0] instruction,
    output logic [DATA_WIDTH-1:0] gpr [REG_COUNT-1:0],
    output CSR_bundle_out csr_bundle,
    output logic                  fire
);
    localparam REG_ADDR_WIDTH = $clog2(REG_COUNT);
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

    // SimpleBus 接口：每条链路一个实例
    SimpleBus_if #(.XLEN(DATA_WIDTH)) lsu_bus  (clk);
    SimpleBus_if #(.XLEN(DATA_WIDTH)) ifu_bus  (clk);
    SimpleBus_if #(.XLEN(DATA_WIDTH)) ram_bus  (clk);
    SimpleBus_if #(.XLEN(DATA_WIDTH)) uart_bus (clk);
    SimpleBus_if #(.XLEN(DATA_WIDTH)) timer_bus(clk);

    IFU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH), .RESET_VEC(RESET_VEC)) ifu (
        .clk(clk),
        .rst_n(rst_n),
        .idu_ready(idu_ready),
        .PC_current(PC_current),
        .PC_next(PC_next),
        .instruction(instruction),
        .ifu_valid(ifu_valid),
        .bus(ifu_bus.Master)
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

    LSU #(.XLEN(DATA_WIDTH)) lsu (
        .clk(clk),
        .rst_n(rst_n),
        .addr(alu_result),
        .store_data(Rs2_data),
        .mem_size(ctrl_sig.mem_size),
        .mem_sign(ctrl_sig.mem_sign),
        .mem_write_en(ctrl_sig.mem_write_en),
        .mem_read_en(ctrl_sig.mem_read_en),
        .ifu_valid(ifu_valid),
        .load_data(mem_load_data),
        .lsu_ready(lsu_ready),
        .bus(lsu_bus.Master)
    );

    Xbar #(.XLEN(DATA_WIDTH)) xbar (
        .clk(clk),
        .rst_n(rst_n),
        .lsu(lsu_bus.Slave),
        .ram(ram_bus.Master),
        .uart(uart_bus.Master),
        .timer(timer_bus.Master)
    );

    RAM #(.XLEN(DATA_WIDTH)) ram (
        .clk(clk),
        .rst_n(rst_n),
        .bus(ram_bus.Slave)
    );

    ROM #(.XLEN(DATA_WIDTH)) rom (
        .clk(clk),
        .rst_n(rst_n),
        .bus(ifu_bus.Slave)
    );

    UART #(.XLEN(DATA_WIDTH)) uart (
        .clk(clk),
        .rst_n(rst_n),
        .bus(uart_bus.Slave)
    );

    Timer #(.XLEN(DATA_WIDTH)) rtc (
        .clk(clk),
        .rst_n(rst_n),
        .bus(timer_bus.Slave)
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
