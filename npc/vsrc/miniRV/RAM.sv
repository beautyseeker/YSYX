module RAM #(parameter DATA_WIDTH = 32, SIZE=256) (
    input clk,
    input rst_n,

    input  logic  [ADDR_WIDTH-1:0] addr,
    input  logic                   wen,
    input  logic  [DATA_WIDTH-1:0] wdata,
    input  logic  [BYTES-1:0]      wmask,
    output logic  [DATA_WIDTH-1:0] rdata
);

    localparam ADDR_WIDTH = $clog2(SIZE);
    localparam BYTES = DATA_WIDTH / 8;
    logic [DATA_WIDTH-1:0] MEM [0:SIZE-1];
    logic [DATA_WIDTH-1:0] full_mask = {
    {8{wmask[3]}}, 
    {8{wmask[2]}}, 
    {8{wmask[1]}}, 
    {8{wmask[0]}}  
};

    initial begin
        string path = get_img_path();
        $display("RAM initialized from: %s", path);
        $readmemh(path, MEM, 0);
    end

    always_ff @(posedge clk) begin
        rdata <= wen ? wdata : MEM[addr];
        if(wen) begin
            MEM[addr] <= (MEM[addr] & ~full_mask) | (wdata & full_mask);
        end
    end
endmodule
