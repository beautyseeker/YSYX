// AXI4-Lite 双主机仲裁：IFU / LSU → 单一下行（接 Xbar）
// LSU 优先；锁定到 R 或 B 响应成交
module Arbiter #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    AXI4_lite.Slave  lsu,
    AXI4_lite.Slave  ifu,
    AXI4_lite.Master xbar
);

    enum logic [1:0] {IDLE, SERV_IFU, SERV_LSU} current, next;
    logic serv_is_write; // 1=等 B，0=等 R（仅对 LSU 有意义；IFU 只读）

    logic ifu_req, lsu_req;
    assign ifu_req = ifu.ARvalid;
    assign lsu_req = lsu.ARvalid || lsu.AWvalid || lsu.Wvalid;

    always_comb begin
        next = current;
        unique case (current)
            IDLE: begin
                if (lsu_req)
                    next = SERV_LSU;
                else if (ifu_req)
                    next = SERV_IFU;
            end
            SERV_IFU: begin
                if (xbar.Rvalid && xbar.Rready)
                    next = IDLE;
            end
            SERV_LSU: begin
                if (serv_is_write) begin
                    if (xbar.BrespValid && xbar.BrespReady)
                        next = IDLE;
                end else begin
                    if (xbar.Rvalid && xbar.Rready)
                        next = IDLE;
                end
            end
            default: next = IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current       <= IDLE;
            serv_is_write <= 1'b0;
        end else begin
            current <= next;
            if (current == IDLE && next == SERV_LSU)
                serv_is_write <= lsu.AWvalid || lsu.Wvalid;
            else if (current == IDLE && next == SERV_IFU)
                serv_is_write <= 1'b0;
        end
    end

    always_comb begin
        // 默认下行安静
        xbar.ARaddr = '0;  xbar.ARvalid = 1'b0;
        xbar.AWaddr = '0;  xbar.AWvalid = 1'b0;
        xbar.Wdata  = '0;  xbar.Wmask   = '0; xbar.Wvalid = 1'b0;
        xbar.Rready = 1'b0;
        xbar.BrespReady = 1'b0;

        ifu.ARready = 1'b0;
        ifu.Rvalid  = 1'b0; ifu.Rdata = '0; ifu.Rresp = 2'b00;
        ifu.AWready = 1'b0; ifu.Wready = 1'b0;
        ifu.BrespValid = 1'b0; ifu.Bresp = 2'b00;

        lsu.ARready = 1'b0;
        lsu.Rvalid  = 1'b0; lsu.Rdata = '0; lsu.Rresp = 2'b00;
        lsu.AWready = 1'b0; lsu.Wready = 1'b0;
        lsu.BrespValid = 1'b0; lsu.Bresp = 2'b00;

        unique case (current)
            SERV_IFU: begin
                xbar.ARaddr  = ifu.ARaddr;
                xbar.ARvalid = ifu.ARvalid;
                xbar.Rready  = ifu.Rready;

                ifu.ARready = xbar.ARready;
                ifu.Rvalid  = xbar.Rvalid;
                ifu.Rdata   = xbar.Rdata;
                ifu.Rresp   = xbar.Rresp;
                // IFU 不写：AW/W/B 保持默认
            end
            SERV_LSU: begin
                xbar.ARaddr     = lsu.ARaddr;
                xbar.ARvalid    = lsu.ARvalid;
                xbar.AWaddr     = lsu.AWaddr;
                xbar.AWvalid    = lsu.AWvalid;
                xbar.Wdata      = lsu.Wdata;
                xbar.Wmask      = lsu.Wmask;
                xbar.Wvalid     = lsu.Wvalid;
                xbar.Rready     = lsu.Rready;
                xbar.BrespReady = lsu.BrespReady;

                lsu.ARready    = xbar.ARready;
                lsu.AWready    = xbar.AWready;
                lsu.Wready     = xbar.Wready;
                lsu.Rvalid     = xbar.Rvalid;
                lsu.Rdata      = xbar.Rdata;
                lsu.Rresp      = xbar.Rresp;
                lsu.BrespValid = xbar.BrespValid;
                lsu.Bresp      = xbar.Bresp;
            end
            default: ;
        endcase
    end

endmodule
