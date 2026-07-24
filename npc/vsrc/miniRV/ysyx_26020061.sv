// 对接 ysyxSoC 的 CPU 顶层（与 ysyxSoC/spec/cpu-interface.md / CPU.scala BlackBox 一致）
// 不含 SRAM/UART；含 CLINT(mtime)。请将模块名换成你的 8 位学号后同步改 CPU.scala。
import defs_pkg::*;

module ysyx_26020061 #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 24,
    parameter REG_COUNT  = 16,
    parameter RESET_VEC  = 32'h2000_0000
) (
    input  logic        clock,
    input  logic        reset,
    input  logic        io_interrupt,

    input  logic        io_master_awready,
    output logic        io_master_awvalid,
    output logic [31:0] io_master_awaddr,
    output logic [3:0]  io_master_awid,
    output logic [7:0]  io_master_awlen,
    output logic [2:0]  io_master_awsize,
    output logic [1:0]  io_master_awburst,
    input  logic        io_master_wready,
    output logic        io_master_wvalid,
    output logic [31:0] io_master_wdata,
    output logic [3:0]  io_master_wstrb,
    output logic        io_master_wlast,
    output logic        io_master_bready,
    input  logic        io_master_bvalid,
    input  logic [1:0]  io_master_bresp,
    input  logic [3:0]  io_master_bid,
    input  logic        io_master_arready,
    output logic        io_master_arvalid,
    output logic [31:0] io_master_araddr,
    output logic [3:0]  io_master_arid,
    output logic [7:0]  io_master_arlen,
    output logic [2:0]  io_master_arsize,
    output logic [1:0]  io_master_arburst,
    output logic        io_master_rready,
    input  logic        io_master_rvalid,
    input  logic [1:0]  io_master_rresp,
    input  logic [31:0] io_master_rdata,
    input  logic        io_master_rlast,
    input  logic [3:0]  io_master_rid,

    output logic        io_slave_awready,
    input  logic        io_slave_awvalid,
    input  logic [31:0] io_slave_awaddr,
    input  logic [3:0]  io_slave_awid,
    input  logic [7:0]  io_slave_awlen,
    input  logic [2:0]  io_slave_awsize,
    input  logic [1:0]  io_slave_awburst,
    output logic        io_slave_wready,
    input  logic        io_slave_wvalid,
    input  logic [31:0] io_slave_wdata,
    input  logic [3:0]  io_slave_wstrb,
    input  logic        io_slave_wlast,
    input  logic        io_slave_bready,
    output logic        io_slave_bvalid,
    output logic [1:0]  io_slave_bresp,
    output logic [3:0]  io_slave_bid,
    output logic        io_slave_arready,
    input  logic        io_slave_arvalid,
    input  logic [31:0] io_slave_araddr,
    input  logic [3:0]  io_slave_arid,
    input  logic [7:0]  io_slave_arlen,
    input  logic [2:0]  io_slave_arsize,
    input  logic [1:0]  io_slave_arburst,
    input  logic        io_slave_rready,
    output logic        io_slave_rvalid,
    output logic [1:0]  io_slave_rresp,
    output logic [31:0] io_slave_rdata,
    output logic        io_slave_rlast,
    output logic [3:0]  io_slave_rid
);

    // 仿真调试信号（非 SoC 顶层端口，供 top_TopMiniRV 层次引用）
    logic [DATA_WIDTH-1:0] PC_current  /* verilator public_flat_rd */;
    logic [DATA_WIDTH-1:0] PC_next     /* verilator public_flat_rd */;
    logic [DATA_WIDTH-1:0] instruction /* verilator public_flat_rd */;
    logic [DATA_WIDTH-1:0] gpr [REG_COUNT-1:0] /* verilator public_flat_rd */;
    CSR_bundle_out csr_bundle /* verilator public_flat_rd */;
    logic fire /* verilator public_flat_rd */;
    logic flush;

    logic rst_n;
    assign rst_n = ~reset;

    // 未使用的 slave：输出拉 0；输入悬空（由端口列表接收但不使用）
    assign io_slave_awready = 1'b0;
    assign io_slave_wready  = 1'b0;
    assign io_slave_bvalid  = 1'b0;
    assign io_slave_bresp   = 2'b00;
    assign io_slave_bid     = 4'b0;
    assign io_slave_arready = 1'b0;
    assign io_slave_rvalid  = 1'b0;
    assign io_slave_rresp   = 2'b00;
    assign io_slave_rdata   = 32'b0;
    assign io_slave_rlast   = 1'b0;
    assign io_slave_rid     = 4'b0;

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

    AXI4 #(.XLEN(DATA_WIDTH)) lsu_bus   (clock);
    AXI4 #(.XLEN(DATA_WIDTH)) ifu_bus   (clock);
    AXI4 #(.XLEN(DATA_WIDTH)) cpu_bus   (clock);
    AXI4 #(.XLEN(DATA_WIDTH)) ext_bus   (clock);
    AXI4 #(.XLEN(DATA_WIDTH)) timer_bus (clock);

    IFU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH), .RESET_VEC(RESET_VEC)) ifu (
        .clk(clock), .rst_n(rst_n),
        .idu_ready(idu_ready),
        .PC_current(PC_current), .PC_next(PC_next),
        .instruction(instruction), .ifu_valid(ifu_valid),
        .bus(ifu_bus.Master)
    );
    assign fire = ifu_valid && idu_ready;

    IDU #(.DATA_WIDTH(DATA_WIDTH), .REG_ADDR_WIDTH(REG_ADDR_WIDTH)) idu (
        .clk(clock), .rst_n(rst_n),
        .lsu_ready(lsu_ready), .inst(instruction), .ifu_valid(ifu_valid),
        .rs1_addr(Rs1_addr), .rs2_addr(Rs2_addr), .rd_addr(Rd_addr),
        .imm(imm_ext), .ctrl_sig(ctrl_sig), .idu_ready(idu_ready)
    );

    RegisterFile #(.DATA_WIDTH(DATA_WIDTH), .REG_COUNT(REG_COUNT)) regfile (
        .clk(clock), .rst_n(rst_n),
        .rs1_addr(Rs1_addr), .rs2_addr(Rs2_addr), .rd_addr(Rd_addr),
        .rs1_data(Rs1_data), .rs2_data(Rs2_data), .write_data(Rd_data),
        .reg_write_en(ctrl_sig.reg_write_en && fire),
        .gpr(gpr)
    );

    logic [DATA_WIDTH-1:0] alu_a, alu_b;
    always_comb begin
        unique case (ctrl_sig.ALU_b_src_sel)
            B_SRC_REG: alu_b = Rs2_data;
            B_SRC_IMM: alu_b = imm_ext;
            default:   alu_b = 'x;
        endcase
    end
    always_comb begin
        unique case (ctrl_sig.ALU_a_src_sel)
            A_SRC_REG: alu_a = Rs1_data;
            A_SRC_IMM: alu_a = imm_ext;
            A_SRC_PC:  alu_a = $unsigned(PC_current);
            default:   alu_a = 'x;
        endcase
    end

    EXU #(.DATA_WIDTH(DATA_WIDTH)) exu (
        .alu_a(alu_a), .alu_b(alu_b), .ALU_op(ctrl_sig.ALU_op),
        .alu_zero(ALU_zero), .ALU_result(alu_result)
    );

    CSRFile #(.XLEN(DATA_WIDTH)) csrfile (
        .clk(clock), .rst_n(rst_n),
        .ctrl_sig(ctrl_sig), .csr_instruction(instruction),
        .PC_current(PC_current), .CSR_RS1(Rs1_data), .CSR_imm(imm_ext),
        .csr_write_en(ctrl_sig.WB_sel == CSR && ctrl_sig.reg_write_en),
        .csr_read_out(CSR_reg), .csr_bundle_out(csr_bundle)
    );

    // 外部中断接入（当前仅接线，中断处理可后续扩展）
    logic unused_intr;
    assign unused_intr = io_interrupt;

    LSU #(.XLEN(DATA_WIDTH)) lsu (
        .clk(clock), .rst_n(rst_n),
        .addr(alu_result), .store_data(Rs2_data),
        .mem_size(ctrl_sig.mem_size), .mem_sign(ctrl_sig.mem_sign),
        .mem_write_en(ctrl_sig.mem_write_en), .mem_read_en(ctrl_sig.mem_read_en),
        .ifu_valid(ifu_valid), .load_data(mem_load_data), .lsu_ready(lsu_ready),
        .bus(lsu_bus.Master)
    );

    Arbiter #(.XLEN(DATA_WIDTH)) arbiter (
        .clk(clock), .rst_n(rst_n),
        .lsu(lsu_bus.Slave), .ifu(ifu_bus.Slave), .xbar(cpu_bus.Master)
    );

    ClintXbar #(.XLEN(DATA_WIDTH)) clint_xbar (
        .clk(clock), .rst_n(rst_n),
        .cpu(cpu_bus.Slave), .ext(ext_bus.Master), .clint(timer_bus.Master)
    );

    Timer #(.XLEN(DATA_WIDTH)) clint (
        .clk(clock), .rst_n(rst_n), .bus(timer_bus.Slave)
    );

    WBU #(.XLEN(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) wbu (
        .alu_result(alu_result), .mem_load_data(mem_load_data),
        .ctrl_sig(ctrl_sig), .PC_current(PC_current), .PC_rel_imm(imm_ext),
        .CSR_tvec(csr_bundle.CSR_tvec), .CSR_epc(csr_bundle.CSR_epc),
        .instruction(instruction), .CSR_data(CSR_reg),
        .WB_data(Rd_data), .PC_next(PC_next)
    );

    // ext_bus ↔ io_master_*
    assign io_master_awvalid = ext_bus.AWvalid;
    assign io_master_awaddr  = ext_bus.AWaddr;
    assign io_master_awid    = ext_bus.AWid;
    assign io_master_awlen   = ext_bus.AWlen;
    assign io_master_awsize  = ext_bus.AWsize;
    assign io_master_awburst = ext_bus.AWburst;
    assign ext_bus.AWready   = io_master_awready;

    assign io_master_wvalid = ext_bus.Wvalid;
    assign io_master_wdata  = ext_bus.Wdata;
    assign io_master_wstrb  = ext_bus.Wstrb;
    assign io_master_wlast  = ext_bus.Wlast;
    assign ext_bus.Wready   = io_master_wready;

    assign io_master_bready = ext_bus.Bready;
    assign ext_bus.Bvalid   = io_master_bvalid;
    assign ext_bus.Bresp    = io_master_bresp;
    assign ext_bus.Bid      = io_master_bid;

    assign io_master_arvalid = ext_bus.ARvalid;
    assign io_master_araddr  = ext_bus.ARaddr;
    assign io_master_arid    = ext_bus.ARid;
    assign io_master_arlen   = ext_bus.ARlen;
    assign io_master_arsize  = ext_bus.ARsize;
    assign io_master_arburst = ext_bus.ARburst;
    assign ext_bus.ARready   = io_master_arready;

    assign io_master_rready = ext_bus.Rready;
    assign ext_bus.Rvalid   = io_master_rvalid;
    assign ext_bus.Rresp    = io_master_rresp;
    assign ext_bus.Rdata    = io_master_rdata;
    assign ext_bus.Rlast    = io_master_rlast;
    assign ext_bus.Rid      = io_master_rid;

endmodule
