import "DPI-C" function string get_img_path();


module ROM #(parameter DATA_WIDTH = 32, SIZE=1024)(
    input clk,
    input rst_n,

    input  logic                   addrValid,
    input  logic  [ADDR_WIDTH-1:0] raddr,
    output logic  [DATA_WIDTH-1:0] rdata,
    output logic                   instValid
);
    localparam ADDR_WIDTH = $clog2(SIZE);
    logic [DATA_WIDTH-1:0] MEM [0:SIZE-1];

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

    localparam THRESHOLD = 4'b1010;
    logic random = (LFSR < THRESHOLD);
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rdata <= 32'h0000_0013;
            instValid <= 1'b0;
        end 
        else begin
            if(addrValid && random) begin
                rdata <= MEM[raddr];
                instValid <= 1'b1;
            end else begin
                instValid <= 1'b0;
            end

        end
    end
endmodule
