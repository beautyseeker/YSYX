// 双主机仲裁：IFU / LSU → 单一下行（接 Xbar）
// LSU 优先；整笔事务锁定 winner，直到响应成交
module Arbiter #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    SimpleBus_if.Slave  lsu,
    SimpleBus_if.Slave  ifu,

    SimpleBus_if.Master xbar
);

    enum logic [1:0] {IDLE, SERV_IFU, SERV_LSU} current, next;

    always_comb begin
        next = current;
        unique case (current)
            IDLE: begin
                if (lsu.reqValid)
                    next = SERV_LSU;
                else if (ifu.reqValid)
                    next = SERV_IFU;
            end
            SERV_IFU: begin
                if (xbar.respValid && xbar.respReady)
                    next = IDLE;
            end
            SERV_LSU: begin
                if (xbar.respValid && xbar.respReady)
                    next = IDLE;
            end
            default: next = IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) current <= IDLE;
        else        current <= next;
    end

    always_comb begin
        // 默认：下行安静，两侧主机均不授信、无响应
        xbar.addr      = '0;
        xbar.wdata     = '0;
        xbar.mask      = '0;
        xbar.wen       = 1'b0;
        xbar.reqValid  = 1'b0;
        xbar.respReady = 1'b0;

        ifu.reqReady   = 1'b0;
        ifu.respValid  = 1'b0;
        ifu.rdata      = '0;
        ifu.err        = 2'b00;

        lsu.reqReady   = 1'b0;
        lsu.respValid  = 1'b0;
        lsu.rdata      = '0;
        lsu.err        = 2'b00;

        unique case (current)
            SERV_IFU: begin
                xbar.addr      = ifu.addr;
                xbar.wdata     = ifu.wdata;
                xbar.mask      = ifu.mask;
                xbar.wen       = ifu.wen;
                xbar.reqValid  = ifu.reqValid;
                xbar.respReady = ifu.respReady;

                ifu.reqReady   = xbar.reqReady;
                ifu.respValid  = xbar.respValid;
                ifu.rdata      = xbar.rdata;
                ifu.err        = xbar.err;
            end
            SERV_LSU: begin
                xbar.addr      = lsu.addr;
                xbar.wdata     = lsu.wdata;
                xbar.mask      = lsu.mask;
                xbar.wen       = lsu.wen;
                xbar.reqValid  = lsu.reqValid;
                xbar.respReady = lsu.respReady;

                lsu.reqReady   = xbar.reqReady;
                lsu.respValid  = xbar.respValid;
                lsu.rdata      = xbar.rdata;
                lsu.err        = xbar.err;
            end
            default: ;
        endcase
    end

endmodule
