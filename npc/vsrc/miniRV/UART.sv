import "DPI-C" function void mmio_write(input int unsigned addr, input int data, input int unsigned mask);

module UART #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,
    SimpleBus_if.Slave bus
);

    logic [XLEN-1:0] full_mask;
    assign full_mask = {
        {8{bus.mask[3]}},
        {8{bus.mask[2]}},
        {8{bus.mask[1]}},
        {8{bus.mask[0]}}
    };

    enum logic [1:0] {IDLE, RESP} current, next;

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
    assign bus.err = 2'b00;

    always_ff @(posedge clk) begin
        if (current == IDLE && bus.reqValid) begin
            if (bus.wen)
                mmio_write(bus.addr, bus.wdata, full_mask);
            else
                bus.rdata <= 32'h0;
        end
    end

endmodule
