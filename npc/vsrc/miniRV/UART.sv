module UART #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    input logic [31:0] addr,
    input logic [31:0] wdata,
    input logic [3:0]  mask,
    input logic        wen,
    input logic        reqValid,
    input logic        respReady,

    output logic [31:0] rdata,
    output logic        respValid,
    output logic        reqReady,
    output logic        err
);

    logic [XLEN-1:0] full_mask;
    assign full_mask = {
        {8{mask[3]}}, 
        {8{mask[2]}}, 
        {8{mask[1]}}, 
        {8{mask[0]}}  
    };

    enum logic [1:0] {IDLE, RESP} current, next;

    always_comb begin
        case(current)
            IDLE: begin
                if(reqValid) begin
                    next = RESP;
                end
            end
            RESP: begin
                if(respReady)
                    next = IDLE;
            end
            default: begin
                next = IDLE;
            end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            current <= IDLE;
        end
        else begin
            current <= next;
        end
    end

    assign respValid = (current == RESP);
    assign reqReady = (current == IDLE);

    always_ff @(posedge clk) begin
        if(current == IDLE && reqValid) begin
            if(wen) begin
                mmio_write(addr, wdata, full_mask);
            end
            else begin
                rdata <= 32'h0;
            end
        end
    end

endmodule
