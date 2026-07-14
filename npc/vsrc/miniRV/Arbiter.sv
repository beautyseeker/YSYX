module Arbiter #(parameter XLEN = 32) (
    input logic clk,
    input logic rst_n,

    SimpleBus_if.Slave  lsu,
    SimpleBus_if.Slave  ifu,

    SimpleBus_if.Master xbar
);

endmodule