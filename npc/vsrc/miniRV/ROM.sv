import "DPI-C" function string get_img_path();

module ROM #(parameter XLEN = 32, DEPTH=1024*1024)(
    input clk,
    input rst_n,

    AXI4_lite.Slave bus
);
    localparam BYTES = XLEN / 8;
    localparam BASE = 32'h8000_0000;
    localparam ADDR_WIDTH = $clog2(DEPTH);
    localparam SIZE = DEPTH * BYTES;
    logic [XLEN-1:0] MEM [0:DEPTH-1] /* verilator public_flat */;

    logic [ADDR_WIDTH-1:0] rom_idx;
    assign rom_idx = ADDR_WIDTH'((bus.ARaddr - BASE) >> 2);

    logic [$clog2(BYTES)-1:0] byte_idx;
    logic addr_in_rom;
    assign byte_idx = bus.ARaddr[1:0];
    assign addr_in_rom = bus.ARaddr inside {[BASE : BASE + SIZE]};
    // assign bus.Rresp = addr_in_rom && !bus.wen && byte_idx == 0 ? 2'b00 : 2'b10;

    initial begin
        string path = get_img_path();
        $display("ROM initialized from: %s", path);
        $readmemh(path, MEM, 0);
    end

    logic [3:0] cnt;
    localparam LATENCY = 4'd1;
    enum logic [2:0] {
    RD_IDLE, RD_BUSY, RD_RESP
    } rd_trans_cur, rd_trans_next;

    enum logic [2:0] {
        WR_IDLE, WR_WAIT_AW, WR_WAIT_W, 
        WR_BUSY, WR_RESP
    } wr_trans_cur, wr_trans_next;


    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            cnt <= 0;
        end
        else begin
            cnt <= (rd_trans_cur == RD_BUSY || wr_trans_cur == WR_BUSY) ? 
            cnt + 1 : 0;
        end
    end

    always_comb begin
        rd_trans_next = rd_trans_cur;
        unique case (rd_trans_cur)
            RD_IDLE:begin
                if (bus.ARvalid && bus.ARready) rd_trans_next = RD_BUSY;
            end
            RD_BUSY:
                if (cnt == LATENCY - 1) rd_trans_next = RD_RESP;
            RD_RESP:
                if (bus.Rvalid && bus.Rready) rd_trans_next = RD_IDLE;
            default: begin
                rd_trans_next = RD_IDLE;
            end
        endcase
    end

    always_comb begin
        wr_trans_next = wr_trans_cur;
        unique case (wr_trans_cur)
            WR_IDLE: begin
                if (bus.AWvalid && bus.AWready) wr_trans_next = WR_WAIT_W;
                else if (bus.Wvalid && bus.Wready) wr_trans_next = WR_WAIT_AW;
            end
            WR_WAIT_AW: begin
                if (bus.AWvalid && bus.AWready) wr_trans_next = WR_BUSY;
            end
            WR_WAIT_W: begin
                if (bus.Wvalid && bus.Wready) wr_trans_next = WR_BUSY;
            end
            WR_BUSY: begin
                if (cnt == LATENCY - 1) wr_trans_next = WR_RESP;
            end
            WR_RESP: begin
                if (bus.BrespValid && bus.BrespReady) wr_trans_next = WR_IDLE;
            end
            default: begin
                wr_trans_next = WR_IDLE;
            end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            rd_trans_cur <= RD_IDLE;
            wr_trans_cur <= WR_IDLE;
        end
        else begin
            rd_trans_cur <= rd_trans_next;
            wr_trans_cur <= wr_trans_next;
        end
    end

    assign bus.ARready = rd_trans_cur == RD_IDLE;
    assign bus.Rvalid = rd_trans_cur == RD_RESP;
    always_ff @(posedge clk) begin
        if(rd_trans_cur == RD_BUSY && cnt == LATENCY - 1) begin
            bus.Rdata <= MEM[rom_idx]; // 此处还未处理读写冲突
        end
    end
    assign bus.Rresp = 2'b00;

    assign bus.AWready = wr_trans_cur == WR_WAIT_AW || wr_trans_cur == WR_IDLE;
    assign bus.Wready = wr_trans_cur == WR_WAIT_W || wr_trans_cur == WR_IDLE;
    always_ff @(posedge clk) begin
        if(wr_trans_cur == WR_BUSY && cnt == LATENCY - 1) begin
            // 此处还未处理读写冲突
            // MEM[rom_idx] <= (bus.Wdata & full_mask) | (MEM[rom_idx] & ~bus.Wmask);
            // ROM从机没有写行为
        end
    end
    assign bus.BrespValid = wr_trans_cur == WR_RESP;
    assign bus.Bresp = 2'b00;

endmodule
