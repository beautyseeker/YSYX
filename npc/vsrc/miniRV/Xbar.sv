// AXI4-Lite 交叉开关：单主机 → RAM / UART / Timer
// 请求按 ARaddr / AWaddr 译码转发；响应按从机 valid 回传
module Xbar #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    AXI4_lite.Slave  cpu,
    AXI4_lite.Master ram,
    AXI4_lite.Master uart,
    AXI4_lite.Master timer
);

    localparam PMEM_BASE  = 32'h8000_0000;
    localparam PMEM_SIZE  = 32'h0040_0000; // 4MB
    localparam UART_BASE  = 32'h1000_0000;
    localparam TIMER_BASE = 32'h1000_0048;

    logic ar_ram, ar_uart, ar_timer, ar_err;
    logic aw_ram, aw_uart, aw_timer, aw_err;

    assign ar_ram   = cpu.ARaddr inside {[PMEM_BASE  : PMEM_BASE  + PMEM_SIZE]};
    assign ar_uart  = cpu.ARaddr inside {[UART_BASE  : UART_BASE  + 4]};
    assign ar_timer = cpu.ARaddr inside {[TIMER_BASE : TIMER_BASE + 4]};
    assign ar_err   = !(ar_ram || ar_uart || ar_timer);

    assign aw_ram   = cpu.AWaddr inside {[PMEM_BASE  : PMEM_BASE  + PMEM_SIZE]};
    assign aw_uart  = cpu.AWaddr inside {[UART_BASE  : UART_BASE  + 4]};
    assign aw_timer = cpu.AWaddr inside {[TIMER_BASE : TIMER_BASE + 4]};
    assign aw_err   = !(aw_ram || aw_uart || aw_timer);

    // ---------- AR ----------
    assign ram.ARaddr    = cpu.ARaddr;
    assign uart.ARaddr   = cpu.ARaddr;
    assign timer.ARaddr  = cpu.ARaddr;
    assign ram.ARvalid   = cpu.ARvalid && ar_ram;
    assign uart.ARvalid  = cpu.ARvalid && ar_uart;
    assign timer.ARvalid = cpu.ARvalid && ar_timer;
    assign cpu.ARready   = ar_ram   ? ram.ARready   :
                           ar_uart  ? uart.ARready  :
                           ar_timer ? timer.ARready :
                           cpu.ARvalid; // 非法地址：收下后由 ERR 状态回 DECERR

    // ---------- R ----------
    assign cpu.Rvalid = ram.Rvalid | uart.Rvalid | timer.Rvalid | err_rvalid;
    assign cpu.Rdata  = ram.Rvalid   ? ram.Rdata   :
                        uart.Rvalid  ? uart.Rdata  :
                        timer.Rvalid ? timer.Rdata :
                        '0;
    assign cpu.Rresp  = err_rvalid ? 2'b11 :
                        ram.Rvalid   ? ram.Rresp   :
                        uart.Rvalid  ? uart.Rresp  :
                        timer.Rvalid ? timer.Rresp :
                        2'b00;
    assign ram.Rready   = cpu.Rready;
    assign uart.Rready  = cpu.Rready;
    assign timer.Rready = cpu.Rready;

    // ---------- AW ----------
    assign ram.AWaddr    = cpu.AWaddr;
    assign uart.AWaddr   = cpu.AWaddr;
    assign timer.AWaddr  = cpu.AWaddr;
    assign ram.AWvalid   = cpu.AWvalid && aw_ram;
    assign uart.AWvalid  = cpu.AWvalid && aw_uart;
    assign timer.AWvalid = cpu.AWvalid && aw_timer;
    assign cpu.AWready   = aw_ram   ? ram.AWready   :
                           aw_uart  ? uart.AWready  :
                           aw_timer ? timer.AWready :
                           cpu.AWvalid;

    // ---------- W ----------
    assign ram.Wdata    = cpu.Wdata;
    assign uart.Wdata   = cpu.Wdata;
    assign timer.Wdata  = cpu.Wdata;
    assign ram.Wmask    = cpu.Wmask;
    assign uart.Wmask   = cpu.Wmask;
    assign timer.Wmask  = cpu.Wmask;
    // W 无地址：跟随 AW 译码（单 outstanding 下 AW/W 成对）
    assign ram.Wvalid   = cpu.Wvalid && aw_ram;
    assign uart.Wvalid  = cpu.Wvalid && aw_uart;
    assign timer.Wvalid = cpu.Wvalid && aw_timer;
    assign cpu.Wready   = aw_ram   ? ram.Wready   :
                          aw_uart  ? uart.Wready  :
                          aw_timer ? timer.Wready :
                          cpu.Wvalid;

    // ---------- B ----------
    assign cpu.BrespValid = ram.BrespValid | uart.BrespValid | timer.BrespValid | err_bvalid;
    assign cpu.Bresp      = err_bvalid ? 2'b11 :
                            ram.BrespValid   ? ram.Bresp   :
                            uart.BrespValid  ? uart.Bresp  :
                            timer.BrespValid ? timer.Bresp :
                            2'b00;
    assign ram.BrespReady   = cpu.BrespReady;
    assign uart.BrespReady  = cpu.BrespReady;
    assign timer.BrespReady = cpu.BrespReady;

    // ---------- 非法地址：单拍 DECERR ----------
    logic err_rvalid, err_bvalid;
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

            // 写：AW 与 W 都需被非法路径收下；简化为 AW 非法时两端都 ready
            if (cpu.AWvalid && cpu.AWready && aw_err &&
                cpu.Wvalid  && cpu.Wready)
                err_wr_pend <= 1'b1;
            if (err_bvalid && cpu.BrespReady)
                err_wr_pend <= 1'b0;
        end
    end

    assign err_rvalid = err_rd_pend;
    assign err_bvalid = err_wr_pend;

endmodule
