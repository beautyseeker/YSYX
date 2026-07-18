// AXI4-Lite 双主机仲裁：IFU / LSU → 单一下行（接 Xbar）
// LSU 优先；锁定到 R 或 B 响应成交
module Arbiter #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    AXI4.Slave  lsu,
    AXI4.Slave  ifu,
    AXI4.Master xbar
);

    enum logic [1:0] {IDLE, SERV_IFU, SERV_LSU} current, next;
    logic serv_is_write;

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
                    if (xbar.Bvalid && xbar.Bready)
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
        xbar.ARaddr = '0; xbar.ARvalid = 1'b0;
        xbar.ARid = '0; xbar.ARlen = '0; xbar.ARsize = '0; xbar.ARburst = '0;
        xbar.AWaddr = '0; xbar.AWvalid = 1'b0;
        xbar.AWid = '0; xbar.AWlen = '0; xbar.AWsize = '0; xbar.AWburst = '0;
        xbar.Wdata = '0; xbar.Wstrb = '0; xbar.Wvalid = 1'b0; xbar.Wlast = 1'b0;
        xbar.Rready = 1'b0;
        xbar.Bready = 1'b0;

        ifu.ARready = 1'b0;
        ifu.Rvalid = 1'b0; ifu.Rdata = '0; ifu.Rresp = 2'b00;
        ifu.Rid = '0; ifu.Rlast = 1'b0;
        ifu.AWready = 1'b0; ifu.Wready = 1'b0;
        ifu.Bvalid = 1'b0; ifu.Bresp = 2'b00; ifu.Bid = '0;

        lsu.ARready = 1'b0;
        lsu.Rvalid = 1'b0; lsu.Rdata = '0; lsu.Rresp = 2'b00;
        lsu.Rid = '0; lsu.Rlast = 1'b0;
        lsu.AWready = 1'b0; lsu.Wready = 1'b0;
        lsu.Bvalid = 1'b0; lsu.Bresp = 2'b00; lsu.Bid = '0;

        unique case (current)
            SERV_IFU: begin
                xbar.ARaddr  = ifu.ARaddr;
                xbar.ARvalid = ifu.ARvalid;
                xbar.ARid    = ifu.ARid;
                xbar.ARlen   = ifu.ARlen;
                xbar.ARsize  = ifu.ARsize;
                xbar.ARburst = ifu.ARburst;
                xbar.Rready  = ifu.Rready;

                ifu.ARready = xbar.ARready;
                ifu.Rvalid  = xbar.Rvalid;
                ifu.Rdata   = xbar.Rdata;
                ifu.Rresp   = xbar.Rresp;
                ifu.Rid     = xbar.Rid;
                ifu.Rlast   = xbar.Rlast;
            end
            SERV_LSU: begin
                xbar.ARaddr  = lsu.ARaddr;
                xbar.ARvalid = lsu.ARvalid;
                xbar.ARid    = lsu.ARid;
                xbar.ARlen   = lsu.ARlen;
                xbar.ARsize  = lsu.ARsize;
                xbar.ARburst = lsu.ARburst;
                xbar.AWaddr  = lsu.AWaddr;
                xbar.AWvalid = lsu.AWvalid;
                xbar.AWid    = lsu.AWid;
                xbar.AWlen   = lsu.AWlen;
                xbar.AWsize  = lsu.AWsize;
                xbar.AWburst = lsu.AWburst;
                xbar.Wdata   = lsu.Wdata;
                xbar.Wstrb   = lsu.Wstrb;
                xbar.Wvalid  = lsu.Wvalid;
                xbar.Wlast   = lsu.Wlast;
                xbar.Rready  = lsu.Rready;
                xbar.Bready  = lsu.Bready;

                lsu.ARready = xbar.ARready;
                lsu.AWready = xbar.AWready;
                lsu.Wready  = xbar.Wready;
                lsu.Rvalid  = xbar.Rvalid;
                lsu.Rdata   = xbar.Rdata;
                lsu.Rresp   = xbar.Rresp;
                lsu.Rid     = xbar.Rid;
                lsu.Rlast   = xbar.Rlast;
                lsu.Bvalid  = xbar.Bvalid;
                lsu.Bresp   = xbar.Bresp;
                lsu.Bid     = xbar.Bid;
            end
            default: ;
        endcase
    end

endmodule
