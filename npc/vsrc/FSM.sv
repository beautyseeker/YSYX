module continious_check(
    input logic clk,
    input logic rst,
    input logic in,
    output logic out
);

    typedef enum logic[1:0] { 
        couting,
        full
    } state;

    logic [15:0] continuous_cnt;
    logic prev_in;
    localparam CNT_MAX = 16'h0004;
    state curr_state, next_state;

    always_comb begin
        next_state = curr_state;
        case (curr_state)
            couting: begin
                next_state = (continuous_cnt >= CNT_MAX) ? 
                full : couting;
            end
            full: begin
                next_state = (in ^ prev_in) ?
                couting : full;
            end
            default: begin
                next_state = couting;
            end
        endcase
    end

    always_ff @(posedge clk or negedge rst) begin
        if (!rst) begin
            continuous_cnt <= 16'h0001;
            prev_in <= 1'b0;
        end
        else begin
            prev_in <= in;
            if(in == prev_in)
                continuous_cnt <= (continuous_cnt < CNT_MAX) ?
                continuous_cnt + 16'h0001 : continuous_cnt;
            else
                continuous_cnt <= 16'h0001;
        end
    end

    always_ff @(posedge clk or negedge rst) begin
        if (!rst) begin
            curr_state <= couting;
        end
        else begin
            curr_state <= next_state;
        end
    end

    assign out = (curr_state == full);

endmodule

module top_FSM (
    input logic clk,
    input logic rst,
    input logic in,
    output logic out
);

    continious_check u_continious_check (
        .clk (clk),
        .rst (rst),
        .in  (in),
        .out (out)
    );

    
endmodule
