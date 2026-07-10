module RAM #(parameter DATA_WIDTH = 32, SIZE=1024) (
    input clk,
    input rst_n,

    input  logic  [ADDR_WIDTH-1:0] addr,
    input  logic                   reqValid,
    input  logic                   wen,
    input  logic  [DATA_WIDTH-1:0] wdata,
    input  logic  [BYTES-1:0]      mask,

    output logic  [DATA_WIDTH-1:0] rdata,
    output logic                   respValid
);

    localparam ADDR_WIDTH = $clog2(SIZE);
    localparam BYTES = DATA_WIDTH / 8;
    logic [DATA_WIDTH-1:0] MEM [0:SIZE-1];
    logic [DATA_WIDTH-1:0] full_mask;
    assign full_mask = {
        {8{mask[3]}}, 
        {8{mask[2]}}, 
        {8{mask[1]}}, 
        {8{mask[0]}}  
    };

    // logic [DATA_WIDTH-1:0] addr_r;
    // logic [DATA_WIDTH-1:0] wdata_r;
    // logic [BYTES-1:0] mask_r;
    // logic wen_r;
    // logic reqValid_r;

    // always_ff @(posedge clk or negedge rst_n) begin
    //     if(!rst_n) begin
    //         addr_r <= 0;
    //         wdata_r <= 0;
    //         mask_r <= 0;
    //         wen_r <= 0;
    //         reqValid_r <= 0;
    //     end
    //     else begin
    //         addr_r <= addr;
    //         wdata_r <= wdata;
    //         mask_r <= mask;
    //         wen_r <= wen;
    //         reqValid_r <= reqValid;
    //     end
    // end

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

    always_ff @(posedge clk) begin
        if(current == IDLE && reqValid) begin
            if(wen) begin
                MEM[addr] <= (MEM[addr] & ~full_mask) | (wdata & full_mask);
                $display("RAM write: addr=0x%h, wdata=0x%h, mask=0x%h", addr, wdata, mask);
            end
            else begin
                rdata <= MEM[addr];
                $display("RAM read: addr=0x%h, rdata=0x%h", addr, rdata);
            end
        end
    end
endmodule
