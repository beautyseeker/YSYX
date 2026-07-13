module RAM #(parameter XLEN = 32, DEPTH=1024*1024) (
    input clk,
    input rst_n,

    input  logic  [XLEN-1:0]       addr,
    input  logic                   reqValid,
    input  logic                   wen,
    input  logic  [XLEN-1:0] wdata,
    input  logic  [XLEN/8-1:0]     mask,
    input  logic                   respReady,

    output logic  [XLEN-1:0] rdata,
    output logic                   respValid,
    output logic                   reqReady,
    output logic   [1:0]           err
);

    localparam BYTES = XLEN / 8;
    localparam BASE = 32'h8000_0000;
    localparam ADDR_WIDTH = $clog2(DEPTH);
    localparam SIZE = DEPTH * BYTES;
    logic [XLEN-1:0] MEM [0:DEPTH-1] /* verilator public_flat */;
    logic [XLEN-1:0] full_mask;
    assign full_mask = {
        {8{mask[3]}}, 
        {8{mask[2]}}, 
        {8{mask[1]}}, 
        {8{mask[0]}}  
    };

    logic [ADDR_WIDTH-1:0] mem_idx;
    assign mem_idx = ADDR_WIDTH'((addr - BASE) >> 2);
    logic addr_in_mem;
    assign addr_in_mem = (addr >= BASE) && (addr < BASE + SIZE);
    assign err = addr_in_mem ? 2'b00 : 2'b01;

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
        case(current)
            IDLE: begin
                if(reqValid) begin
                    next = BUSY;
                end
            end
            BUSY: begin
                if(cnt == LATENCY-1) begin
                    next = RESP;
                end
            end
            RESP: begin
                if(respReady)
                    next = IDLE;
            end
            default: begin
                next = IDLE;
            end
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

    assign respValid = (current == RESP);
    assign reqReady = (current == IDLE);

    always_ff @(posedge clk) begin
        if(current == IDLE && reqValid) begin
            if(wen) begin
                MEM[mem_idx] <= 
                (MEM[mem_idx] & ~full_mask) | (wdata & full_mask);
            end
            else begin
                rdata <= MEM[mem_idx];
            end
        end
    end
endmodule
