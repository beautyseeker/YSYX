import "DPI-C" function string get_img_path();

module ROM #(parameter XLEN = 32, DEPTH=1024*1024)(
    input clk,
    input rst_n,

    SimpleBus_if.Slave bus
);
localparam BYTES = XLEN / 8;
    localparam BASE = 32'h8000_0000;
    localparam ADDR_WIDTH = $clog2(DEPTH);
    localparam SIZE = DEPTH * BYTES;
    logic [XLEN-1:0] MEM [0:DEPTH-1] /* verilator public_flat */;

    logic [ADDR_WIDTH-1:0] rom_idx;
    assign rom_idx = ADDR_WIDTH'((bus.addr - BASE) >> 2);

    logic [$clog2(BYTES)-1:0] byte_idx;
    logic addr_in_rom;
    assign byte_idx = bus.addr[1:0];
    assign addr_in_rom = bus.addr inside {[BASE : BASE + SIZE]};
    assign bus.err = addr_in_rom && !bus.wen && byte_idx == 0 ? 2'b00 : 2'b10;

    initial begin
        string path = get_img_path();
        $display("ROM initialized from: %s", path);
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
    localparam LATENCY = 4'd1;
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
            if (bus.wen) begin
                $error("Invalid ROM addr:%x does not support write", bus.addr);
            end else begin
                bus.rdata <= MEM[rom_idx];
            end
        end
    end
endmodule
