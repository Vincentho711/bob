`timescale 1ns/1ps

module tb_hello_world;
  reg clk = 0;
  reg rst = 0;
  reg [7:0] a;
  reg [7:0] b;

  always begin
    #(CLK_PERIOD) clk = ~clk;
  end

  initial begin
    $dumpvars();
    $dumpfile("tb_hello_world.vcd");
  end

endmodule
