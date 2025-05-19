module hello_world_top (
    input wire       clk_i,
    input wire [7:0] a_i,
    input wire [7:0] b_i,
    output reg [8:0] c_o
);
  reg [8:0] c_d;

  adder u_adder(
    .clk_i      (clk_i),
    .a_i        (a_i),
    .b_i        (b_i),
    .c_o        (c_d)
  );

  always_ff @(posedge clk_i) begin
    c_o <= c_d;
  end

endmodule
