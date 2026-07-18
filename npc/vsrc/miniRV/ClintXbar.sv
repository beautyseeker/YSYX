// CPU 内交叉开关：CLINT 本地，其余出站到 SoC master
module ClintXbar #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    AXI4.Slave  cpu,
    AXI4.Master ext,
    AXI4.Master clint
);

    localparam CLINT_BASE = 32'h1000_0048;
    localparam CLINT_SIZE = 32'h8; // mtime lo/hi

    logic ar_clint, aw_clint;
    assign ar_clint = cpu.ARaddr inside {[CLINT_BASE : CLINT_BASE + CLINT_SIZE - 1]};
    assign aw_clint = cpu.AWaddr inside {[CLINT_BASE : CLINT_BASE + CLINT_SIZE - 1]};

    // ---------- AR ----------
    assign clint.ARaddr  = cpu.ARaddr;
    assign clint.ARid    = cpu.ARid;
    assign clint.ARlen   = cpu.ARlen;
    assign clint.ARsize  = cpu.ARsize;
    assign clint.ARburst = cpu.ARburst;
    assign clint.ARvalid = cpu.ARvalid && ar_clint;

    assign ext.ARaddr  = cpu.ARaddr;
    assign ext.ARid    = cpu.ARid;
    assign ext.ARlen   = cpu.ARlen;
    assign ext.ARsize  = cpu.ARsize;
    assign ext.ARburst = cpu.ARburst;
    assign ext.ARvalid = cpu.ARvalid && !ar_clint;

    assign cpu.ARready = ar_clint ? clint.ARready : ext.ARready;

    // ---------- R ----------
    assign cpu.Rvalid = clint.Rvalid | ext.Rvalid;
    assign cpu.Rdata  = clint.Rvalid ? clint.Rdata : ext.Rdata;
    assign cpu.Rresp  = clint.Rvalid ? clint.Rresp : ext.Rresp;
    assign cpu.Rid    = clint.Rvalid ? clint.Rid   : ext.Rid;
    assign cpu.Rlast  = clint.Rvalid ? clint.Rlast : ext.Rlast;
    assign clint.Rready = cpu.Rready;
    assign ext.Rready   = cpu.Rready;

    // ---------- AW ----------
    assign clint.AWaddr  = cpu.AWaddr;
    assign clint.AWid    = cpu.AWid;
    assign clint.AWlen   = cpu.AWlen;
    assign clint.AWsize  = cpu.AWsize;
    assign clint.AWburst = cpu.AWburst;
    assign clint.AWvalid = cpu.AWvalid && aw_clint;

    assign ext.AWaddr  = cpu.AWaddr;
    assign ext.AWid    = cpu.AWid;
    assign ext.AWlen   = cpu.AWlen;
    assign ext.AWsize  = cpu.AWsize;
    assign ext.AWburst = cpu.AWburst;
    assign ext.AWvalid = cpu.AWvalid && !aw_clint;

    assign cpu.AWready = aw_clint ? clint.AWready : ext.AWready;

    // ---------- W ----------
    assign clint.Wdata  = cpu.Wdata;
    assign clint.Wstrb  = cpu.Wstrb;
    assign clint.Wlast  = cpu.Wlast;
    assign clint.Wvalid = cpu.Wvalid && aw_clint;

    assign ext.Wdata  = cpu.Wdata;
    assign ext.Wstrb  = cpu.Wstrb;
    assign ext.Wlast  = cpu.Wlast;
    assign ext.Wvalid = cpu.Wvalid && !aw_clint;

    assign cpu.Wready = aw_clint ? clint.Wready : ext.Wready;

    // ---------- B ----------
    assign cpu.Bvalid = clint.Bvalid | ext.Bvalid;
    assign cpu.Bresp  = clint.Bvalid ? clint.Bresp : ext.Bresp;
    assign cpu.Bid    = clint.Bvalid ? clint.Bid   : ext.Bid;
    assign clint.Bready = cpu.Bready;
    assign ext.Bready   = cpu.Bready;

endmodule
