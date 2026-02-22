module shifter (
    input logic [7:0] din,
    input logic [2:0] shamt,
    input logic dir, // 1 for left shift, 0 for right shift
    input logic arith, // 0 for logical shift, 1 for arithmetic shift
    output logic [7:0] dout
);
    always_comb begin
        dout = din; // Default assignment
        case({dir, arith})
            2'b00: // Logical right shift
                dout = din >> shamt;
            2'b01: // Arithmetic right shift
                dout = $signed(din) >>> shamt;
            2'b10: // Logical left shift
                dout = din << shamt;
            2'b11: // Arithmetic left shift (same as logical left shift)
                dout = din << shamt;
        endcase
    end
    
endmodule

module random_gen (
    input logic clk,
    input logic rst,
    input logic en,
    input logic [7:0] seed,
    output logic [7:0] dout
);
    logic [7:0] dout_reg;
    logic feedback;
    assign feedback = dout_reg[4] ^ dout_reg[3] ^ dout_reg[2] ^ dout_reg[0];
    always_ff @(posedge clk or negedge rst) begin
        if (!rst) begin
            dout_reg <= seed == 8'b0 ? 8'hAC : seed;
        end

        else if(en) begin
            dout_reg <= {feedback, dout_reg[7:1]};
        end
    end

    assign dout = dout_reg;
    
endmodule

module top_barrel_shifter (
    input logic clk,
    input logic rst,
    input logic slow_down,
    input logic speed_up,
    input logic [7:0] seed,
    output logic [7:0] dout,
    output logic [7:0] dout_hex0,
    output logic [7:0] dout_hex1
);

logic[31:0] cnt;
logic[31:0] CNT_MAX;
logic tick;


always_ff @( posedge clk or negedge rst ) begin : blockName
    if (!rst) begin
        cnt <= 0;
        tick <= 1'b0;
    end 
    else begin
        cnt <= (cnt == CNT_MAX-1) ? 0 : cnt + 1;
        tick <= (cnt == CNT_MAX-1) ? 1'b1 : 1'b0;
    end
end

always_ff @(posedge clk, negedge rst) begin
    if (!rst) begin
        CNT_MAX <= 32'd100000;
    end 
    else begin
        if (speed_up) begin
            CNT_MAX <= (CNT_MAX > 32'd10) ? CNT_MAX >> 1 : CNT_MAX;
        end 
        else if (slow_down) begin
            CNT_MAX <= (CNT_MAX < 32'd1000000) ? CNT_MAX << 1 : CNT_MAX;
        end 
        else begin
            CNT_MAX <= CNT_MAX;
        end
    end

end

random_gen u_random_gen (
    .clk  (clk),
    .rst  (rst),
    .en   (tick),
    .seed (seed),
    .dout (dout)
);

seg7_encoder u_seg7_encoder_0 (
    .in  (dout[3:0]),
    .out (dout_hex0)
);
seg7_encoder u_seg7_encoder_1 (
    .in  (dout[7:4]),
    .out (dout_hex1)
);
    
endmodule
