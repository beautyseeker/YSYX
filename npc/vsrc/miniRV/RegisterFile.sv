module RegisterFile #(ADDR_WIDTH = 5, DATA_WIDTH = 32)
(
    input logic        clk,
    input logic        rst,
    input logic [ADDR_WIDTH-1:0]  rs1_addr,
    input logic [ADDR_WIDTH-1:0]  rs2_addr,
    input logic [ADDR_WIDTH-1:0]  rd_addr,
    input logic [DATA_WIDTH-1:0] rd_data,
    input logic        reg_write_en,

    output logic [DATA_WIDTH-1:0] rs1_data,
    output logic [DATA_WIDTH-1:0] rs2_data
);

    logic [DATA_WIDTH-1:0] reg_file [2**ADDR_WIDTH-1:0];
    // Read operation (combinational)
    assign rs1_data = (rs1_addr != {ADDR_WIDTH{1'b0}}) ? 
    reg_file[rs1_addr] : {DATA_WIDTH{1'b0}}; // x0 is always zero
    assign rs2_data = (rs2_addr != {ADDR_WIDTH{1'b0}}) ? 
    reg_file[rs2_addr] : {DATA_WIDTH{1'b0}}; // x0 is always zero

    // Write operation (synchronous)
    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            // Reset all registers to zero
            integer i;
            for (i = 0; i < 2**ADDR_WIDTH; i++) begin
                reg_file[i] <= {DATA_WIDTH{1'b0}};
            end
        end else if (reg_write_en && rd_addr != {ADDR_WIDTH{1'b0}}) begin
            // Write data to the register file (except x0)
            reg_file[rd_addr] <= rd_data;
        end
    end
endmodule
