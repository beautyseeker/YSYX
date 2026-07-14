interface SimpleBus_if #(parameter XLEN = 32) (
    input logic clk
);
    logic [XLEN-1:0] addr;
    logic [XLEN-1:0] wdata;
    logic [XLEN-1:0] rdata;
    logic [XLEN/8-1:0] mask;
    logic wen;
    logic reqValid;
    logic reqReady;
    
    logic respValid;
    logic respReady;
    logic [1:0] err;

    // 主机：发出请求，接收响应
    modport Master (
        output addr, wdata, mask, wen, reqValid, respReady,
        input  rdata, respValid, reqReady, err
    );

    // 从机：接收请求，发出响应
    modport Slave (
        input  addr, wdata, mask, wen, reqValid, respReady,
        output rdata, respValid, reqReady, err
    );
endinterface
