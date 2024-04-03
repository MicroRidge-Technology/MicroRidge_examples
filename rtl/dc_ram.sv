`default_nettype none
/**/
`timescale 1ns / 1ns
module dc_ram #(
    parameter DATA_WIDTH = 8,
    parameter ADDR_WIDTH = 6
) (
    input wire [DATA_WIDTH-1:0] data_a,
    input wire [DATA_WIDTH-1:0] data_b,
    input wire [ADDR_WIDTH-1:0] addr_a,
    input wire [ADDR_WIDTH-1:0] addr_b,
    input wire we_a,
    input wire we_b,
    input wire clk_a,
    input wire clk_b,
    output reg [DATA_WIDTH-1:0] q_a,
    output reg [DATA_WIDTH-1:0] q_b
);

  // Declare the RAM variable
  //verilator lint_off MULTIDRIVEN
  reg [DATA_WIDTH-1:0] ram[2**ADDR_WIDTH-1:0];
  //verilator lint_on MULTIDRIVEN
  always @(posedge clk_a) begin
    // Port A
    if (we_a) begin
      ram[addr_a] <= data_a;
      q_a <= data_a;
    end else begin
      q_a <= ram[addr_a];
    end
  end

  always @(posedge clk_b) begin
    // Port B
    if (we_b) begin
      ram[addr_b] <= data_b;
      q_b <= data_b;
    end else begin
      q_b <= ram[addr_b];
    end
  end

endmodule
`default_nettype wire
