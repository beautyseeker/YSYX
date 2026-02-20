module ALU (
    input logic [3:0] a,
    input logic [3:0] b,
    input logic [2:0] op,
    output logic [3:0] result,
    output logic cout,
    output logic zero,
    output logic overflow,
    output logic less
);

    logic [4:0] sum;
    logic [3:0] b_inverted;
    logic add_sub;
    
    // Determine if we are adding or subtracting
    assign add_sub = (op == 3'b001) ? 1'b1 : 1'b0; // 1 for subtraction, 0 for addition
    assign b_inverted = add_sub ? ~b : b; // Invert b for subtraction

    // Perform addition or subtraction
    assign sum = a + b_inverted + add_sub;

    // Set outputs based on operation
    always_comb begin
        result   = 4'b0;
        cout     = 1'b0;
        overflow = 1'b0;
        less     = 1'b0;
        zero = (result == 4'b0000) ? 1'b1 : 1'b0;
        case (op)
            3'b000, 3'b001: begin // Addition, Subtraction
                result = sum[3:0];
                cout = sum[4];
                overflow = (a[3] == b[3]) && (result[3] != a[3]);
            end
            3'b010: result = ~a;
            3'b011: result = a & b;
            3'b100: result = a | b;
            3'b101: result = a ^ b;
            3'b110: begin // SLT (Set Less Than)
                less = sum[3] ^ ((a[3] != b[3]) && (result[3] != a[3]));
                result = {3'b000, less};
            end
            3'b111: result = {3'b000, zero};

            default: begin
                result = 4'b0000;
                cout = 1'b0;
                overflow = 1'b0;
            end
        endcase
    end
    
endmodule


module top_ALU (
    input logic        clk,
    input logic        rst,
    input logic [3:0] a,
    input logic [3:0] b,
    input logic [2:0] op,
    output logic [3:0] result,
    output logic cout,
    output logic zero,
    output logic overflow,
    output logic less,
    output logic [7:0] seg_res
);

    logic [3:0] alu_result;
    ALU alu_inst (
        .a(a),
        .b(b),
        .op(op),
        .result(alu_result),
        .cout(cout),
        .zero(zero),
        .overflow(overflow),
        .less(less)
    );
    seg7_encoder seg7_inst (
        .in(alu_result),
        .out(seg_res)
    );
    assign result = alu_result;
    
endmodule
