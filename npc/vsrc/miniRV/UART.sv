import "DPI-C" function void mmio_write(input int unsigned addr, input int data, input int unsigned mask);

module UART #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,
    AXI4_lite.Slave bus
);

    logic [XLEN-1:0] awaddr_r, wdata_r;
    logic [3:0]      wmask_r;
    logic [XLEN-1:0] full_mask_bus, full_mask_r;

    assign full_mask_bus = {
        {8{bus.Wmask[3]}}, {8{bus.Wmask[2]}},
        {8{bus.Wmask[1]}}, {8{bus.Wmask[0]}}
    };
    assign full_mask_r = {
        {8{wmask_r[3]}}, {8{wmask_r[2]}},
        {8{wmask_r[1]}}, {8{wmask_r[0]}}
    };

    // ---------- 读 ----------
    enum logic {RD_IDLE, RD_RESP} rd_cur, rd_next;

    always_comb begin
        rd_next = rd_cur;
        unique case (rd_cur)
            RD_IDLE: if (bus.ARvalid && bus.ARready) rd_next = RD_RESP;
            RD_RESP: if (bus.Rvalid  && bus.Rready)  rd_next = RD_IDLE;
            default: rd_next = RD_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) rd_cur <= RD_IDLE;
        else        rd_cur <= rd_next;
    end

    assign bus.ARready = (rd_cur == RD_IDLE);
    assign bus.Rvalid  = (rd_cur == RD_RESP);
    assign bus.Rdata   = '0;
    assign bus.Rresp   = 2'b00;

    // ---------- 写 ----------
    enum logic [1:0] {WR_IDLE, WR_WAIT_AW, WR_WAIT_W, WR_RESP} wr_cur, wr_next;

    always_comb begin
        wr_next = wr_cur;
        unique case (wr_cur)
            WR_IDLE: begin
                if (bus.AWvalid && bus.AWready && bus.Wvalid && bus.Wready)
                    wr_next = WR_RESP;
                else if (bus.AWvalid && bus.AWready)
                    wr_next = WR_WAIT_W;
                else if (bus.Wvalid && bus.Wready)
                    wr_next = WR_WAIT_AW;
            end
            WR_WAIT_AW: if (bus.AWvalid && bus.AWready) wr_next = WR_RESP;
            WR_WAIT_W:  if (bus.Wvalid  && bus.Wready)  wr_next = WR_RESP;
            WR_RESP:    if (bus.BrespValid && bus.BrespReady) wr_next = WR_IDLE;
            default: wr_next = WR_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_cur   <= WR_IDLE;
            awaddr_r <= '0;
            wdata_r  <= '0;
            wmask_r  <= '0;
        end else begin
            wr_cur <= wr_next;
            if (bus.AWvalid && bus.AWready)
                awaddr_r <= bus.AWaddr;
            if (bus.Wvalid && bus.Wready) begin
                wdata_r <= bus.Wdata;
                wmask_r <= bus.Wmask;
            end
        end
    end

    always_ff @(posedge clk) begin
        if (wr_cur == WR_IDLE && bus.AWvalid && bus.AWready && bus.Wvalid && bus.Wready)
            mmio_write(bus.AWaddr, bus.Wdata, full_mask_bus);
        else if (wr_cur == WR_WAIT_W && bus.Wvalid && bus.Wready)
            mmio_write(awaddr_r, bus.Wdata, full_mask_bus);
        else if (wr_cur == WR_WAIT_AW && bus.AWvalid && bus.AWready)
            mmio_write(bus.AWaddr, wdata_r, full_mask_r);
    end

    assign bus.AWready    = (wr_cur == WR_IDLE) || (wr_cur == WR_WAIT_AW);
    assign bus.Wready     = (wr_cur == WR_IDLE) || (wr_cur == WR_WAIT_W);
    assign bus.BrespValid = (wr_cur == WR_RESP);
    assign bus.Bresp      = 2'b00;

endmodule
