module prioty8_3_encoder(
    input  logic [7:0] in,
    input  logic       en,
    output logic [2:0] out,
    output logic      valid
);
    always_comb begin
        valid = 1'b0;
        out   = 3'b000;
        if (en) begin
            for (int i = 0; i <= 7; i++) begin
                if (in[i]) begin
                    out   = i[2:0];
                    valid = 1'b1;
                end
            end
        end
        else begin
            valid = 1'b0;
            out   = 3'b000;
        end
    end
endmodule

module seg7_encoder(
    input  logic [3:0] in,
    output logic [7:0] out
);
    logic [7:0] seg_code;
    
    always_comb begin
        case (in)
            4'd0: seg_code = 8'b11111101;
            4'd1: seg_code = 8'b01100000;
            4'd2: seg_code = 8'b11011010;
            4'd3: seg_code = 8'b11110010;
            4'd4: seg_code = 8'b01100110;
            4'd5: seg_code = 8'b10110110;
            4'd6: seg_code = 8'b10111110;
            4'd7: seg_code = 8'b11100000;
            4'd8: seg_code = 8'b11111110;
            4'd9: seg_code = 8'b11110110;
            4'd10: seg_code = 8'b11101110; // A
            4'd11: seg_code = 8'b00111110; // b
            4'd12: seg_code = 8'b10011100; // C
            4'd13: seg_code = 8'b01111010; // d
            4'd14: seg_code = 8'b10011110; // E
            4'd15: seg_code = 8'b10001110; // F
            default: seg_code = 8'b00000000;
        endcase
        out = ~seg_code;
    end
endmodule

module top_prioty8_3_encoder(
    input  logic        clk,
    input  logic        rst,
    input  logic[7:0]   SW,
    output logic[3:0]   led_out,  
    output logic        valid,
    output logic[7:0]   seg_out
);
    prioty8_3_encoder u_prioty8_3_encoder (
        .in   (SW),
        .en   (1'b1),
        .out  (led_out[2:0]),
        .valid(valid)
    );

    seg7_encoder u_seg7_encoder (
        .in  (led_out),
        .out (seg_out)
    );
endmodule
