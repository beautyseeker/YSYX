interface AXI4_lite #(parameter XLEN = 32) (
    input clk
);
    logic [XLEN-1:0] ARaddr;
    logic            ARvalid;
    logic            ARready;

    logic [XLEN-1:0] AWaddr;
    logic            AWvalid;
    logic            AWready;

    logic [XLEN-1:0] Wdata;
    logic [3:0]      Wmask;
    logic            Wvalid;
    logic            Wready;

    logic [XLEN-1:0] Rdata;
    logic [1:0]      Rresp;
    logic            Rvalid;
    logic            Rready;

    logic [1:0]      Bresp;
    logic            BrespValid;
    logic            BrespReady;

    modport Master(
        output ARaddr, ARvalid,    // 读地址请求
        input  ARready,            // 读地址握手
        
        input Rvalid,              // 读数据请求
        output Rready,             // 读数据握手
        input  Rdata, Rresp,       // 读数据响应
        
        output AWaddr, AWvalid,    // 写地址请求
        input  AWready,            // 写地址握手

        output Wdata, Wmask, Wvalid,// 写数据请求
        input  Wready,              // 写数据握手

        input Bresp, BrespValid,    // 写数据响应
        output BrespReady           // 写响应握手
    );

    modport Slave(
        input  ARaddr, ARvalid,    // 读地址请求
        output ARready,            // 读地址握手
        
        output  Rvalid,             // 读数据请求
        input   Rready,             // 读数据握手
        output Rdata, Rresp,       // 读数据响应
        
        input  AWaddr, AWvalid,    // 写地址请求
        output AWready,            // 写地址握手

        input Wdata, Wmask, Wvalid,// 写数据请求
        output Wready,             // 写数据握手

        output Bresp, BrespValid,   // 写数据响应
        input BrespReady          // 写响应握手
    );
endinterface
