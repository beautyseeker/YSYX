// 完整 AXI4（单 beat 即可接 ysyxSoC；len/size/burst/id/last 已齐）
interface AXI4 #(
    parameter int XLEN   = 32,
    parameter int ID_W   = 4,
    parameter int LEN_W  = 8,
    parameter int SIZE_W = 3
) (
    input logic clk
);
    // AR
    logic [XLEN-1:0]   ARaddr;
    logic              ARvalid;
    logic              ARready;
    logic [ID_W-1:0]   ARid;
    logic [LEN_W-1:0]  ARlen;
    logic [SIZE_W-1:0] ARsize;
    logic [1:0]        ARburst;

    // R
    logic [XLEN-1:0]   Rdata;
    logic [1:0]        Rresp;
    logic              Rvalid;
    logic              Rready;
    logic [ID_W-1:0]   Rid;
    logic              Rlast;

    // AW
    logic [XLEN-1:0]   AWaddr;
    logic              AWvalid;
    logic              AWready;
    logic [ID_W-1:0]   AWid;
    logic [LEN_W-1:0]  AWlen;
    logic [SIZE_W-1:0] AWsize;
    logic [1:0]        AWburst;

    // W
    logic [XLEN-1:0]   Wdata;
    logic [XLEN/8-1:0] Wstrb;
    logic              Wvalid;
    logic              Wready;
    logic              Wlast;

    // B
    logic [1:0]        Bresp;
    logic              Bvalid;
    logic              Bready;
    logic [ID_W-1:0]   Bid;

    modport Master(
        output ARaddr, ARvalid, ARid, ARlen, ARsize, ARburst,
        input  ARready,
        input  Rvalid, Rdata, Rresp, Rid, Rlast,
        output Rready,
        output AWaddr, AWvalid, AWid, AWlen, AWsize, AWburst,
        input  AWready,
        output Wdata, Wstrb, Wvalid, Wlast,
        input  Wready,
        input  Bvalid, Bresp, Bid,
        output Bready
    );

    modport Slave(
        input  ARaddr, ARvalid, ARid, ARlen, ARsize, ARburst,
        output ARready,
        output Rvalid, Rdata, Rresp, Rid, Rlast,
        input  Rready,
        input  AWaddr, AWvalid, AWid, AWlen, AWsize, AWburst,
        output AWready,
        input  Wdata, Wstrb, Wvalid, Wlast,
        output Wready,
        output Bvalid, Bresp, Bid,
        input  Bready
    );
endinterface
