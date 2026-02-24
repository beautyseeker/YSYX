module RegisterFile #(REG_COUNT = 32, DATA_WIDTH = 32)
(
    input logic                         clk,
    input logic                         rst_n,
    input logic [REG_ADDR_WIDTH-1:0]  rs1_addr,
    input logic [REG_ADDR_WIDTH-1:0]  rs2_addr,
    input logic [REG_ADDR_WIDTH-1:0]  rd_addr,
    input logic [DATA_WIDTH-1:0]      write_data,
    input logic                       reg_write_en,

    output logic [DATA_WIDTH-1:0] rs1_data,
    output logic [DATA_WIDTH-1:0] rs2_data
);
    parameter REG_ADDR_WIDTH = $clog2(REG_COUNT);
    logic [DATA_WIDTH-1:0] reg_file [REG_COUNT-1:0];
    // Read operation (combinational)
    assign rs1_data = (rs1_addr == 0) ? 0 : reg_file[rs1_addr]; // x0 is always zero
    assign rs2_data = (rs2_addr == 0) ? 0 : reg_file[rs2_addr]; // x0 is always zero

    // Write operation (synchronous)
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            integer i;
            for (i = 0; i < REG_COUNT; i++) begin
                reg_file[i] <= {DATA_WIDTH{1'b0}};
            end
        end else if (reg_write_en && rd_addr != 0) begin  // x0寄存器禁止写入
            reg_file[rd_addr] <= write_data;
        end
    end
endmodule
