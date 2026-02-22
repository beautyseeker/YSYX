module ps2_keyboard(
    input logic clk,
    input logic resetn,
    input logic ps2_clk,
    input logic ps2_data,

    output logic [7:0]  keycode,
    output logic [7:0]  press_cnt,
    output logic        released  
);

    reg [9:0] buffer;        // ps2_data bits
    reg [3:0] count;  // count ps2_data bits
    reg [2:0] ps2_clk_sync;

    always @(posedge clk) begin
        ps2_clk_sync <=  {ps2_clk_sync[1:0],ps2_clk};
    end

    wire sampling = ps2_clk_sync[2] & ~ps2_clk_sync[1];
    reg ready;

    always @(posedge clk) begin
        ready <= 0;
        if (resetn == 0) begin // reset
            count <= 0;
            ready <= 0;
        end
        else begin
            if (sampling) begin
              if (count == 4'd10) begin
                if ((buffer[0] == 0) &&  // start bit
                    (ps2_data)       &&  // stop bit
                    (^buffer[9:1])) begin      // odd  parity
                end
                count <= 0;     // for next
                ready <= 1;
              end else begin
                buffer[count] <= ps2_data;  // store ps2_data
                count <= count + 3'b1;
              end
            end
        end
    end

    typedef enum logic[1:0] { 
        IDLE,
        PRESS,
        RELEASE
    } key_state;

    key_state curr_state, next_state;

    always_comb begin
        next_state = curr_state;
        if(ready) begin
            case (curr_state)
                IDLE: begin
                    if(buffer[8:1] != 8'h00 && buffer[8:1] != 8'hF0) begin
                        next_state = PRESS;
                    end
                end
                PRESS: begin
                    if(buffer[8:1] == 8'hF0) begin
                        next_state = RELEASE;
                    end
                    $display("press %x", buffer[8:1]);
                end
                RELEASE: begin
                    next_state = IDLE;
                    $display("release %x", buffer[8:1]);
                end
                default:
                    next_state = IDLE;
            endcase
        end

    end

    always_ff @(posedge clk , negedge resetn) begin
        if(!resetn) begin
            curr_state <= IDLE;
            press_cnt <= 8'h00;
        end
        else begin
            curr_state <= next_state;
            if(curr_state == PRESS) begin
                keycode <= buffer[8:1];
                if(next_state == RELEASE) begin
                    press_cnt <= press_cnt + 8'h01;
                end
            end
        end
    end

    assign released = (curr_state == IDLE);

endmodule


module seg7_encoder_en(
    input  logic        en,
    input  logic [3:0]  in,
    output logic [7:0]  out
);
    logic [7:0] seg_code;
    seg7_encoder u_keycode_seg7_encoder_0 (
        .in  (in),
        .out (seg_code)
    );
    assign out = en ? seg_code : 8'b11111111;

endmodule

module top_ps2keyboard(
    input  logic        clk,
    input  logic        rst,
    input  logic        ps2_clk,
    input  logic        ps2_data,

    output logic [7:0]  keycode_hex0,
    output logic [7:0]  keycode_hex1,
    output logic [7:0]  ASCIICode_hex0,
    output logic [7:0]  ASCIICode_hex1,
    output logic [7:0]  press_cnt_bcd0,
    output logic [7:0]  press_cnt_bcd1
);

    logic released;
    logic [7:0] cnt_out;
    logic [7:0] keycode;
    
    ps2_keyboard u_ps2_keyboard (
        .clk        (clk),
        .resetn     (rst),
        .ps2_clk    (ps2_clk),
        .ps2_data   (ps2_data),
        .keycode    (keycode),
        .press_cnt  (cnt_out),
        .released   (released)
    );


    seg7_encoder_en u_keycode_seg7_encoder_0 (
        .en  (~released),
        .in  (keycode[3:0]),
        .out (keycode_hex0)
    );
    seg7_encoder_en u_keycode_seg7_encoder_1 (
        .en  (~released),
        .in  (keycode[7:4]),
        .out (keycode_hex1)
    );

    seg7_encoder_en u_press_cnt_bcd_seg7_encoder_0 (
        .en  (1'b1),
        .in  (cnt_out[3:0]),
        .out (press_cnt_bcd0)
    );
    seg7_encoder_en u_press_cnt_bcd_seg7_encoder_1 (
        .en  (1'b1),
        .in  (cnt_out[7:4]),
        .out (press_cnt_bcd1)
    );

endmodule
