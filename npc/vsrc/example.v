module example(
  input        a,
  input        b,
  output       y
);

assign y = a ^ b;

endmodule

module top_example (
  input       clk,
  input       rst,
  input        a,
  input        b,
  output       y
);

  example u_example (
    .a(a),
    .b(b),
    .y(y)
);
  
endmodule
