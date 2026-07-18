module Timer #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,
    AXI4.Slave bus
);
    localparam MTIME_BASE = 32'h1000_0048;
    enum logic [1:0] {RD_IDLE, RD_RESP} current, next;
    logic [63:0] mtime;
    logic [3:0]  arid_r;

    always_comb begin
        next = current;
        unique case (current)
            RD_IDLE:
                if (bus.ARvalid && bus.ARready)
                    next = RD_RESP;
            RD_RESP:
                if (bus.Rvalid && bus.Rready)
                    next = RD_IDLE;
            default: next = RD_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current <= RD_IDLE;
            mtime   <= '0;
            arid_r  <= '0;
        end else begin
            current <= next;
            mtime   <= mtime + 1;
            if (current == RD_IDLE && bus.ARvalid && bus.ARready)
                arid_r <= bus.ARid;
        end
    end

    assign bus.ARready = (current == RD_IDLE);
    assign bus.Rvalid  = (current == RD_RESP);
    assign bus.Rresp   = 2'b00;
    assign bus.Rid     = arid_r;
    assign bus.Rlast   = 1'b1;

    assign bus.AWready = 1'b0;
    assign bus.Wready  = 1'b0;
    assign bus.Bresp   = 2'b00;
    assign bus.Bvalid  = 1'b0;
    assign bus.Bid     = '0;

    always_ff @(posedge clk) begin
        if (current == RD_IDLE && bus.ARvalid && bus.ARready) begin
            if (bus.ARaddr == MTIME_BASE)
                bus.Rdata <= mtime[31:0];
            else if (bus.ARaddr == MTIME_BASE + 4)
                bus.Rdata <= mtime[63:32];
            else
                bus.Rdata <= '0;
        end
    end

endmodule
