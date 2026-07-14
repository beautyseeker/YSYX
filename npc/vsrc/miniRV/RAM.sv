module RAM #(parameter XLEN = 32, DEPTH = 1024*1024) (
    input logic clk,
    input logic rst_n,
    SimpleBus_if.Slave bus
);

    localparam BYTES = XLEN / 8;
    localparam BASE = 32'h8000_0000;
    localparam ADDR_WIDTH = $clog2(DEPTH);
    localparam SIZE = DEPTH * BYTES;
    logic [XLEN-1:0] MEM [0:DEPTH-1] /* verilator public_flat */;
    logic [XLEN-1:0] full_mask;
    assign full_mask = {
        {8{bus.mask[3]}},
        {8{bus.mask[2]}},
        {8{bus.mask[1]}},
        {8{bus.mask[0]}}
    };

    logic [ADDR_WIDTH-1:0] mem_idx;
    assign mem_idx = ADDR_WIDTH'((bus.addr - BASE) >> 2);
    logic addr_in_mem;
    assign addr_in_mem = (bus.addr >= BASE) && (bus.addr < BASE + SIZE);
    assign bus.err = addr_in_mem ? 2'b00 : 2'b10;

    initial begin
        string path = get_img_path();
        $display("RAM initialized from: %s", path);
        $readmemh(path, MEM, 0);
    end

    logic [3:0] LFSR;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            LFSR <= 4'b1; // 非零初始状态
        end else begin
            LFSR <= {LFSR[2:0], LFSR[3] ^ LFSR[2]};
        end
    end

    logic [3:0] cnt;
    localparam LATENCY = 4'd2;
    enum logic [1:0] {IDLE, BUSY, RESP} current, next;

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            cnt <= 0;
        end
        else begin
            cnt <= (current == BUSY) ? cnt + 1 : 0;
        end
    end

    always_comb begin
        next = current;
        unique case (current)
            IDLE: if (bus.reqValid) next = BUSY;
            BUSY: if (cnt == LATENCY - 1) next = RESP;
            RESP: if (bus.respReady) next = IDLE;
            default: next = IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            current <= IDLE;
        end
        else begin
            current <= next;
        end
    end

    assign bus.respValid = (current == RESP);
    assign bus.reqReady = (current == IDLE);

    always_ff @(posedge clk) begin
        if (current == IDLE && bus.reqValid) begin
            if (bus.wen)
                MEM[mem_idx] <= (MEM[mem_idx] & ~full_mask) | (bus.wdata & full_mask);
            else
                bus.rdata <= MEM[mem_idx];
        end
    end

endmodule
