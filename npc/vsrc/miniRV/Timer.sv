import "DPI-C" function longint unsigned mmio_read(input int unsigned addr);

module Timer #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,
    SimpleBus_if.Slave bus
);

    enum logic [1:0] {IDLE, RESP} current, next;
    logic [63:0] rtc64;

    always_comb begin
        next = current;
        unique case (current)
            IDLE: if (bus.reqValid) next = RESP;
            RESP: if (bus.respReady) next = IDLE;
            default: next = IDLE;
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

    assign bus.respValid = (current == RESP);
    assign bus.reqReady = (current == IDLE);
    assign bus.err = bus.wen ? 2'b10 : 2'b00;
    assign bus.rdata = rtc64[31:0];

    always_ff @(posedge clk) begin
        if (current == IDLE && bus.reqValid) begin
            if (bus.wen)
                $error("Timer is read-only");
            else
                rtc64 <= mmio_read(bus.addr);
        end
    end

endmodule
