module Xbar #(parameter XLEN = 32)
(
    input logic                  clk,
    input logic                  rst_n,

    // 主机请求通道
    input  logic  [XLEN-1:0]       m_addr,
    input  logic                   m_reqValid,
    input  logic                   m_wen,
    input  logic  [XLEN-1:0]       m_wdata,
    input  logic  [3:0]            m_mask,
    input  logic                   m_respReady,

    output logic  [XLEN -1:0]      m_rdata,
    output logic                   m_respValid,
    output logic                   m_reqReady,
    output logic   [1:0]           m_err,


    // 从机RAM端口
    output logic  [XLEN-1:0]       ram_addr,
    output logic                   ram_reqValid,
    output logic                   ram_wen,
    output logic  [XLEN-1:0]       ram_wdata,
    output logic  [3:0]            ram_mask,
    output logic                   ram_respReady,

    input  logic  [XLEN -1:0]      ram_rdata,
    input  logic                   ram_respValid,
    input  logic                   ram_reqReady,
    input  logic   [1:0]           ram_err,

    // 从机UART端口
    output logic  [XLEN-1:0]       uart_addr,
    output logic                   uart_reqValid,
    output logic                   uart_wen,
    output logic  [XLEN-1:0]       uart_wdata,
    output logic  [3:0]            uart_mask,
    output logic                   uart_respReady,

    input  logic  [XLEN -1:0]      uart_rdata,
    input  logic                   uart_respValid,
    input  logic                   uart_reqReady,
    input  logic   [1:0]           uart_err,

    // 从机Timer端口
    output logic  [XLEN-1:0]       timer_addr,
    output logic                   timer_reqValid,
    output logic                   timer_wen,
    output logic  [XLEN-1:0]       timer_wdata,
    output logic  [3:0]            timer_mask,
    output logic                   timer_respReady,

    input  logic  [XLEN -1:0]      timer_rdata,
    input  logic                   timer_respValid,
    input  logic                   timer_reqReady,
    input  logic   [1:0]           timer_err
);
    localparam PMEM_BASE = 32'h8000_0000;
    localparam PMEM_SIZE = 32'h0040_0000; // 4MB
    localparam UART_BASE = 32'h1000_0000;
    localparam TIMER_BASE = 32'h1000_0048;

    assign ram_addr = m_addr;
    assign ram_reqValid = m_reqValid;
    assign ram_wen = m_wen;
    assign ram_wdata = m_wdata;
    assign ram_mask = m_mask;
    assign ram_respReady = m_respReady;

    assign uart_addr = m_addr;
    assign uart_reqValid = m_reqValid && (m_addr inside {[UART_BASE : UART_BASE + 4]});
    assign uart_wen = m_wen;
    assign uart_wdata = m_wdata;
    assign uart_mask = m_mask;
    assign uart_respReady = m_respReady;

    assign timer_addr = m_addr;
    assign timer_reqValid = m_reqValid && (m_addr inside {[TIMER_BASE : TIMER_BASE + 4]});
    assign timer_wen = m_wen;
    assign timer_wdata = m_wdata;
    assign timer_mask = m_mask;
    assign timer_respReady = m_respReady;

    logic is_uart_access, is_ram_access, is_rtc_access;
    assign is_uart_access = (m_addr inside {[UART_BASE : UART_BASE + 4]});
    assign is_ram_access = (m_addr inside {[PMEM_BASE : PMEM_BASE + PMEM_SIZE]});
    assign is_rtc_access = (m_addr inside {[TIMER_BASE : TIMER_BASE + 4]});

    always_comb begin
        if (is_ram_access) begin
            m_rdata = ram_rdata;
            m_respValid = ram_respValid;
            m_reqReady = ram_reqReady;
            m_err = ram_err;
        end
        else if (is_uart_access) begin
            m_rdata = uart_rdata;
            m_respValid = uart_respValid;
            m_reqReady = uart_reqReady;
            m_err = uart_err;
        end
        else if (is_rtc_access) begin
            m_rdata = timer_rdata;
            m_respValid = timer_respValid;
            m_reqReady = timer_reqReady;
            m_err = timer_err;
        end
        else begin
            m_rdata = '0;
            m_respValid = 1'b0;
            m_reqReady = 1'b1; // 对于无效地址，立即准备好请求
            m_err = 2'b01; // 错误码，表示无效地址
        end
    end
endmodule
