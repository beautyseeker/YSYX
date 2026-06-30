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

    initial begin
        string path = get_img_path();
        $display("RAM initialized from: %s", path);
        $readmemh(path, MEM, 0);
    end

    always_ff @(posedge clk) begin
        if(reqValid) begin
            if(wen) begin
                MEM[addr] <= (MEM[addr] & ~full_mask) | (wdata & full_mask);
                respValid <= 1'b1;
            end
            
            else begin
                rdata <= (MEM[addr]);
                respValid <= 1'b1;
            end
        end
        else
            respValid <= 1'b0;
    end
endmodule
