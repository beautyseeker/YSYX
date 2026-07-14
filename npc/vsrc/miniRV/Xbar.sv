module Xbar #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    // 面对主机：呈 Slave（接收 LSU 请求）
    SimpleBus_if.Slave  lsu,
    // 面对从机：呈 Master（向外设转发请求）
    SimpleBus_if.Master ram,
    SimpleBus_if.Master uart,
    SimpleBus_if.Master timer
);

    localparam PMEM_BASE  = 32'h8000_0000;
    localparam PMEM_SIZE  = 32'h0040_0000; // 4MB
    localparam UART_BASE  = 32'h1000_0000;
    localparam TIMER_BASE = 32'h1000_0048;

    logic is_uart_access, is_ram_access, is_rtc_access;
    assign is_uart_access = (lsu.addr inside {[UART_BASE : UART_BASE + 4]});
    assign is_ram_access  = (lsu.addr inside {[PMEM_BASE : PMEM_BASE + PMEM_SIZE]});
    assign is_rtc_access  = (lsu.addr inside {[TIMER_BASE : TIMER_BASE + 4]});

    // ------- 请求广播 / 地址片选 -------
    assign ram.addr      = lsu.addr;
    assign ram.reqValid  = lsu.reqValid && is_ram_access;
    assign ram.wen       = lsu.wen;
    assign ram.wdata     = lsu.wdata;
    assign ram.mask      = lsu.mask;
    assign ram.respReady = lsu.respReady;

    assign uart.addr      = lsu.addr;
    assign uart.reqValid  = lsu.reqValid && is_uart_access;
    assign uart.wen       = lsu.wen;
    assign uart.wdata     = lsu.wdata;
    assign uart.mask      = lsu.mask;
    assign uart.respReady = lsu.respReady;

    assign timer.addr      = lsu.addr;
    assign timer.reqValid  = lsu.reqValid && is_rtc_access;
    assign timer.wen       = lsu.wen;
    assign timer.wdata     = lsu.wdata;
    assign timer.mask      = lsu.mask;
    assign timer.respReady = lsu.respReady;

    // ------- 响应回传 -------
    always_comb begin
        if (is_ram_access) begin
            lsu.rdata     = ram.rdata;
            lsu.respValid = ram.respValid;
            lsu.reqReady  = ram.reqReady;
            lsu.err       = ram.err;
        end else if (is_uart_access) begin
            lsu.rdata     = uart.rdata;
            lsu.respValid = uart.respValid;
            lsu.reqReady  = uart.reqReady;
            lsu.err       = uart.err;
        end else if (is_rtc_access) begin
            lsu.rdata     = timer.rdata;
            lsu.respValid = timer.respValid;
            lsu.reqReady  = timer.reqReady;
            lsu.err       = timer.err;
        end else begin
            lsu.rdata     = '0;
            lsu.respValid = 1'b0;
            lsu.reqReady  = 1'b1;
            lsu.err       = 2'b01;
        end
    end
endmodule
