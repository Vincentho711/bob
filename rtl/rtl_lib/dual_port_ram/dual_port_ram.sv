module dual_port_ram
  #(parameter int DATA_WIDTH = 32,
    parameter int ADDR_WIDTH = 3
  ) (
    input wire       wr_clk_i,
    input wire       wr_en_i,
    input wire  [ADDR_WIDTH-1:0] wr_addr_i,
    input wire  [DATA_WIDTH-1:0] wr_data_i,
    input wire  [ADDR_WIDTH-1:0] rd_addr_i,
    output wire  [DATA_WIDTH-1:0] rd_data_o
);
    localparam int DEPTH = 1 << ADDR_WIDTH;
    reg [DATA_WIDTH-1:0] mem [DEPTH];

    assign rd_data_o = mem[rd_addr_i];

    always_ff @(posedge wr_clk_i) begin
        if (wr_en_i) begin
            mem[wr_addr_i] <= wr_data_i;
        end
    end
endmodule
