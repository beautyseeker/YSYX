// NPC 仿真壳：实例化官方 CPU 顶层 + 外挂 AXI RAM/UART（替代原核内存储器/串口）
import defs_pkg::*;

module top_TopMiniRV #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 24,
    parameter REG_COUNT  = 16,
    parameter RESET_VEC  = 32'h8000_0000
) (
    input  logic                  clk,
    input  logic                  rst_n,

    output logic [DATA_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] PC_next,
    output logic [DATA_WIDTH-1:0] instruction,
    output logic [DATA_WIDTH-1:0] gpr [REG_COUNT-1:0],
    output CSR_bundle_out         csr_bundle,
    output logic                  fire
);

    logic reset;
    assign reset = ~rst_n;

    // CPU master ↔ 仿真 Xbar
    logic        m_awready, m_awvalid;
    logic [31:0] m_awaddr;
    logic [3:0]  m_awid;
    logic [7:0]  m_awlen;
    logic [2:0]  m_awsize;
    logic [1:0]  m_awburst;
    logic        m_wready, m_wvalid;
    logic [31:0] m_wdata;
    logic [3:0]  m_wstrb;
    logic        m_wlast;
    logic        m_bready, m_bvalid;
    logic [1:0]  m_bresp;
    logic [3:0]  m_bid;
    logic        m_arready, m_arvalid;
    logic [31:0] m_araddr;
    logic [3:0]  m_arid;
    logic [7:0]  m_arlen;
    logic [2:0]  m_arsize;
    logic [1:0]  m_arburst;
    logic        m_rready, m_rvalid;
    logic [1:0]  m_rresp;
    logic [31:0] m_rdata;
    logic        m_rlast;
    logic [3:0]  m_rid;

    ysyx_26020061 #(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .REG_COUNT(REG_COUNT),
        .RESET_VEC(RESET_VEC)
    ) cpu (
        .clock(clk),
        .reset(reset),
        .io_interrupt(1'b0),

        .io_master_awready(m_awready),
        .io_master_awvalid(m_awvalid),
        .io_master_awaddr(m_awaddr),
        .io_master_awid(m_awid),
        .io_master_awlen(m_awlen),
        .io_master_awsize(m_awsize),
        .io_master_awburst(m_awburst),
        .io_master_wready(m_wready),
        .io_master_wvalid(m_wvalid),
        .io_master_wdata(m_wdata),
        .io_master_wstrb(m_wstrb),
        .io_master_wlast(m_wlast),
        .io_master_bready(m_bready),
        .io_master_bvalid(m_bvalid),
        .io_master_bresp(m_bresp),
        .io_master_bid(m_bid),
        .io_master_arready(m_arready),
        .io_master_arvalid(m_arvalid),
        .io_master_araddr(m_araddr),
        .io_master_arid(m_arid),
        .io_master_arlen(m_arlen),
        .io_master_arsize(m_arsize),
        .io_master_arburst(m_arburst),
        .io_master_rready(m_rready),
        .io_master_rvalid(m_rvalid),
        .io_master_rresp(m_rresp),
        .io_master_rdata(m_rdata),
        .io_master_rlast(m_rlast),
        .io_master_rid(m_rid),

        // slave 输入悬空
        .io_slave_awready(),
        .io_slave_awvalid(1'b0),
        .io_slave_awaddr(32'b0),
        .io_slave_awid(4'b0),
        .io_slave_awlen(8'b0),
        .io_slave_awsize(3'b0),
        .io_slave_awburst(2'b0),
        .io_slave_wready(),
        .io_slave_wvalid(1'b0),
        .io_slave_wdata(32'b0),
        .io_slave_wstrb(4'b0),
        .io_slave_wlast(1'b0),
        .io_slave_bready(1'b0),
        .io_slave_bvalid(),
        .io_slave_bresp(),
        .io_slave_bid(),
        .io_slave_arready(),
        .io_slave_arvalid(1'b0),
        .io_slave_araddr(32'b0),
        .io_slave_arid(4'b0),
        .io_slave_arlen(8'b0),
        .io_slave_arsize(3'b0),
        .io_slave_arburst(2'b0),
        .io_slave_rready(1'b0),
        .io_slave_rvalid(),
        .io_slave_rresp(),
        .io_slave_rdata(),
        .io_slave_rlast(),
        .io_slave_rid()
    );

    AXI4 #(.XLEN(DATA_WIDTH)) cpu_bus  (clk);
    AXI4 #(.XLEN(DATA_WIDTH)) ram_bus  (clk);
    AXI4 #(.XLEN(DATA_WIDTH)) uart_bus (clk);

    // 扁平 master ↔ interface
    assign cpu_bus.AWvalid = m_awvalid;
    assign cpu_bus.AWaddr  = m_awaddr;
    assign cpu_bus.AWid    = m_awid;
    assign cpu_bus.AWlen   = m_awlen;
    assign cpu_bus.AWsize  = m_awsize;
    assign cpu_bus.AWburst = m_awburst;
    assign m_awready       = cpu_bus.AWready;

    assign cpu_bus.Wvalid = m_wvalid;
    assign cpu_bus.Wdata  = m_wdata;
    assign cpu_bus.Wstrb  = m_wstrb;
    assign cpu_bus.Wlast  = m_wlast;
    assign m_wready       = cpu_bus.Wready;

    assign cpu_bus.Bready = m_bready;
    assign m_bvalid       = cpu_bus.Bvalid;
    assign m_bresp        = cpu_bus.Bresp;
    assign m_bid          = cpu_bus.Bid;

    assign cpu_bus.ARvalid = m_arvalid;
    assign cpu_bus.ARaddr  = m_araddr;
    assign cpu_bus.ARid    = m_arid;
    assign cpu_bus.ARlen   = m_arlen;
    assign cpu_bus.ARsize  = m_arsize;
    assign cpu_bus.ARburst = m_arburst;
    assign m_arready       = cpu_bus.ARready;

    assign cpu_bus.Rready = m_rready;
    assign m_rvalid       = cpu_bus.Rvalid;
    assign m_rresp        = cpu_bus.Rresp;
    assign m_rdata        = cpu_bus.Rdata;
    assign m_rlast        = cpu_bus.Rlast;
    assign m_rid          = cpu_bus.Rid;

    Xbar #(.XLEN(DATA_WIDTH)) xbar (
        .clk(clk), .rst_n(rst_n),
        .cpu(cpu_bus.Slave), .ram(ram_bus.Master), .uart(uart_bus.Master)
    );

    RAM #(.XLEN(DATA_WIDTH)) ram (
        .clk(clk), .rst_n(rst_n), .bus(ram_bus.Slave)
    );

    UART #(.XLEN(DATA_WIDTH)) uart (
        .clk(clk), .rst_n(rst_n), .bus(uart_bus.Slave)
    );

    // 调试口：层次引用 CPU 内部 public 信号
    assign PC_current  = cpu.PC_current;
    assign PC_next     = cpu.PC_next;
    assign instruction = cpu.instruction;
    assign fire        = cpu.fire;
    assign csr_bundle  = cpu.csr_bundle;
    for (genvar i = 0; i < REG_COUNT; i++) begin : gpr_fwd
        assign gpr[i] = cpu.gpr[i];
    end

endmodule
