import "DPI-C" function string get_img_path();


module ROM #(parameter DATA_WIDTH = 32, SIZE=256)(
    input clk,
    input rst_n,

    input  logic  [ADDR_WIDTH-1:0] raddr,
    output logic  [DATA_WIDTH-1:0] rdata
);
    localparam ADDR_WIDTH = $clog2(SIZE);
    logic [DATA_WIDTH-1:0] MEM [0:SIZE-1];

    initial begin
        string path = get_img_path();
        $display("ROM initialized from: %s", path);
        $readmemh(path, MEM, 0);
    end

    // logic [3:0] LFSR;
    // int random_delay;

    // always_ff @(posedge clk or negedge rst_n) begin
    //     if (!rst_n) begin
    //         LFSR <= 4'b1; // 非零初始状态
    //     end else begin
    //         // 4-bit Fibonacci LFSR with taps at bits 4 and 3 (1-based indexing)
    //         LFSR <= {LFSR[2:0], LFSR[3] ^ LFSR[2]};
    //     end
    // end


    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rdata <= 32'h0000_0013;
        end 
        else begin
            rdata <= MEM[raddr];
        end
    end
endmodule
