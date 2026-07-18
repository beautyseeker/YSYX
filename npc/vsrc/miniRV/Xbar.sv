// 仿真用交叉开关：主机 → RAM / UART（SoC 外设由 ysyxSoC 提供，此处仅 NPC 单测）
module Xbar #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    AXI4.Slave  cpu,
    AXI4.Master ram,
    AXI4.Master uart
);

    localparam PMEM_BASE = 32'h8000_0000;
    localparam PMEM_SIZE = 32'h0040_0000;
    localparam UART_BASE = 32'h1000_0000;

    logic ar_ram, ar_uart, ar_err;
    logic aw_ram, aw_uart, aw_err;

    assign ar_ram  = cpu.ARaddr inside {[PMEM_BASE : PMEM_BASE + PMEM_SIZE - 1]};
    assign ar_uart = cpu.ARaddr inside {[UART_BASE : UART_BASE + 4]};
    assign ar_err  = !(ar_ram || ar_uart);

    assign aw_ram  = cpu.AWaddr inside {[PMEM_BASE : PMEM_BASE + PMEM_SIZE - 1]};
    assign aw_uart = cpu.AWaddr inside {[UART_BASE : UART_BASE + 4]};
    assign aw_err  = !(aw_ram || aw_uart);

    // ---------- AR ----------
    assign ram.ARaddr  = cpu.ARaddr;
    assign uart.ARaddr = cpu.ARaddr;
    assign ram.ARid    = cpu.ARid;
    assign uart.ARid   = cpu.ARid;
    assign ram.ARlen   = cpu.ARlen;
    assign uart.ARlen  = cpu.ARlen;
    assign ram.ARsize  = cpu.ARsize;
    assign uart.ARsize = cpu.ARsize;
    assign ram.ARburst = cpu.ARburst;
    assign uart.ARburst= cpu.ARburst;
    assign ram.ARvalid  = cpu.ARvalid && ar_ram;
    assign uart.ARvalid = cpu.ARvalid && ar_uart;
    assign cpu.ARready  = ar_ram  ? ram.ARready  :
                          ar_uart ? uart.ARready :
                          cpu.ARvalid;

    // ---------- R ----------
    logic err_rvalid;
    assign cpu.Rvalid = ram.Rvalid | uart.Rvalid | err_rvalid;
    assign cpu.Rdata  = ram.Rvalid  ? ram.Rdata  :
                        uart.Rvalid ? uart.Rdata : '0;
    assign cpu.Rresp  = err_rvalid ? 2'b11 :
                        ram.Rvalid  ? ram.Rresp  :
                        uart.Rvalid ? uart.Rresp : 2'b00;
    assign cpu.Rid    = ram.Rvalid  ? ram.Rid  :
                        uart.Rvalid ? uart.Rid : '0;
    assign cpu.Rlast  = ram.Rvalid  ? ram.Rlast  :
                        uart.Rvalid ? uart.Rlast :
                        err_rvalid;
    assign ram.Rready  = cpu.Rready;
    assign uart.Rready = cpu.Rready;

    // ---------- AW ----------
    assign ram.AWaddr  = cpu.AWaddr;
    assign uart.AWaddr = cpu.AWaddr;
    assign ram.AWid    = cpu.AWid;
    assign uart.AWid   = cpu.AWid;
    assign ram.AWlen   = cpu.AWlen;
    assign uart.AWlen  = cpu.AWlen;
    assign ram.AWsize  = cpu.AWsize;
    assign uart.AWsize = cpu.AWsize;
    assign ram.AWburst = cpu.AWburst;
    assign uart.AWburst= cpu.AWburst;
    assign ram.AWvalid  = cpu.AWvalid && aw_ram;
    assign uart.AWvalid = cpu.AWvalid && aw_uart;
    assign cpu.AWready  = aw_ram  ? ram.AWready  :
                          aw_uart ? uart.AWready :
                          cpu.AWvalid;

    // ---------- W ----------
    assign ram.Wdata  = cpu.Wdata;
    assign uart.Wdata = cpu.Wdata;
    assign ram.Wstrb  = cpu.Wstrb;
    assign uart.Wstrb = cpu.Wstrb;
    assign ram.Wlast  = cpu.Wlast;
    assign uart.Wlast = cpu.Wlast;
    assign ram.Wvalid  = cpu.Wvalid && aw_ram;
    assign uart.Wvalid = cpu.Wvalid && aw_uart;
    assign cpu.Wready  = aw_ram  ? ram.Wready  :
                         aw_uart ? uart.Wready :
                         cpu.Wvalid;

    // ---------- B ----------
    logic err_bvalid;
    assign cpu.Bvalid = ram.Bvalid | uart.Bvalid | err_bvalid;
    assign cpu.Bresp  = err_bvalid ? 2'b11 :
                        ram.Bvalid  ? ram.Bresp  :
                        uart.Bvalid ? uart.Bresp : 2'b00;
    assign cpu.Bid    = ram.Bvalid  ? ram.Bid  :
                        uart.Bvalid ? uart.Bid : '0;
    assign ram.Bready  = cpu.Bready;
    assign uart.Bready = cpu.Bready;

    logic err_rd_pend, err_wr_pend;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            err_rd_pend <= 1'b0;
            err_wr_pend <= 1'b0;
        end else begin
            if (cpu.ARvalid && cpu.ARready && ar_err)
                err_rd_pend <= 1'b1;
            if (err_rvalid && cpu.Rready)
                err_rd_pend <= 1'b0;
            if (cpu.AWvalid && cpu.AWready && aw_err &&
                cpu.Wvalid  && cpu.Wready)
                err_wr_pend <= 1'b1;
            if (err_bvalid && cpu.Bready)
                err_wr_pend <= 1'b0;
        end
    end
    assign err_rvalid = err_rd_pend;
    assign err_bvalid = err_wr_pend;

endmodule
