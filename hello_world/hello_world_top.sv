module hello_world_top (
    input wire       clk_i,
    input wire [7:0] a_i,
    output reg [7:0] b_o
);

  reg [7:0] b_o;
  always_ff @(posedge clk) begin
    b_o <= a_i;
  end

endmodule
