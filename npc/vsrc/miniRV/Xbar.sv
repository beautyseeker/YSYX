module Xbar #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    // 面对主机侧（Arbiter 下行）：呈 Slave
    SimpleBus_if.Slave  cpu,
    // 面对从机：呈 Master
    SimpleBus_if.Master ram,
    SimpleBus_if.Master uart,
    SimpleBus_if.Master timer
);

    localparam PMEM_BASE  = 32'h8000_0000;
    localparam PMEM_SIZE  = 32'h0040_0000; // 4MB
    localparam UART_BASE  = 32'h1000_0000;
    localparam TIMER_BASE = 32'h1000_0048;

    logic is_uart_access, is_ram_access, is_rtc_access;
    assign is_uart_access = (cpu.addr inside {[UART_BASE : UART_BASE + 4]});
    assign is_ram_access  = (cpu.addr inside {[PMEM_BASE : PMEM_BASE + PMEM_SIZE]});
    assign is_rtc_access  = (cpu.addr inside {[TIMER_BASE : TIMER_BASE + 4]});

    // ------- 请求广播 / 地址片选 -------
    assign ram.addr      = cpu.addr;
    assign ram.reqValid  = cpu.reqValid && is_ram_access;
    assign ram.wen       = cpu.wen;
    assign ram.wdata     = cpu.wdata;
    assign ram.mask      = cpu.mask;
    assign ram.respReady = cpu.respReady;

    assign uart.addr      = cpu.addr;
    assign uart.reqValid  = cpu.reqValid && is_uart_access;
    assign uart.wen       = cpu.wen;
    assign uart.wdata     = cpu.wdata;
    assign uart.mask      = cpu.mask;
    assign uart.respReady = cpu.respReady;

    assign timer.addr      = cpu.addr;
    assign timer.reqValid  = cpu.reqValid && is_rtc_access;
    assign timer.wen       = cpu.wen;
    assign timer.wdata     = cpu.wdata;
    assign timer.mask      = cpu.mask;
    assign timer.respReady = cpu.respReady;

    // ------- 响应回传 -------
    always_comb begin
        if (is_ram_access) begin
            cpu.rdata     = ram.rdata;
            cpu.respValid = ram.respValid;
            cpu.reqReady  = ram.reqReady;
            cpu.err       = ram.err;
        end else if (is_uart_access) begin
            cpu.rdata     = uart.rdata;
            cpu.respValid = uart.respValid;
            cpu.reqReady  = uart.reqReady;
            cpu.err       = uart.err;
        end else if (is_rtc_access) begin
            cpu.rdata     = timer.rdata;
            cpu.respValid = timer.respValid;
            cpu.reqReady  = timer.reqReady;
            cpu.err       = timer.err;
        end else begin
            cpu.rdata     = '0;
            cpu.respValid = 1'b0;
            cpu.reqReady  = 1'b1;
            cpu.err       = 2'b01;
        end
    end
endmodule
