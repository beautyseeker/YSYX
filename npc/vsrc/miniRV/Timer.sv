module Timer #(parameter XLEN = 32) (
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
    output logic [1:0]  err
);

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
    logic [63:0] rtc64;

    always_ff @(posedge clk) begin
        if(current == IDLE && reqValid) begin
            if(wen) begin
                $error("Timer is read-only");
            end
            else begin
                rtc64 <= mmio_read(addr);
            end
        end
    end

    assign rdata = rtc64[31:0];

endmodule
