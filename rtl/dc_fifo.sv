`default_nettype none
/**/
`timescale 1ns / 1ns
module dc_fifo #(
    parameter type T = logic [15:0],
    parameter L2DEPTH = 3,
    parameter logic FWFT = 0,
    parameter logic OUT_REG = 0

) (
    input wire wr_clk,
    input wire wr_rstn,
    input T wr_din,
    input wire wr_write,
    output logic wr_full,
    output logic [L2DEPTH-1:0] wr_usedw,

    input wire rd_clk,
    input wire rd_rstn,
    input wire rd_read,
    output T rd_dout,
    output logic rd_empty,
    output logic [L2DEPTH-1:0] rd_usedw
);

  if (!!FWFT && !!OUT_REG) begin
    $fatal("FWFT and OUT_REG are incompatible\n");
  end

  reg [$bits(T)-1:0] ram[2**L2DEPTH-1:0];

  logic [L2DEPTH-1:0] w_rptr, w_wptr;
  logic [L2DEPTH-1:0] r_rptr, r_wptr;
  logic [L2DEPTH-1:0] r_rptr_plus1, w_wptr_plus1, w_wptr_plus2;

  cc_gray #(.WIDTH(L2DEPTH))
      cc_wptr (
          .clk_in(wr_clk),
          .clk_out(rd_clk),
          .in(w_wptr),
          .out(r_wptr)
      ),
      cc_rptr (
          .clk_in(rd_clk),
          .clk_out(wr_clk),
          .in(r_rptr),
          .out(w_rptr)
      );

  logic wr_prot;
  assign wr_prot = wr_write && !wr_full;
  always_ff @(posedge wr_clk) begin
    wr_full  <= w_wptr_plus1 == w_rptr;
    wr_usedw <= w_wptr - w_rptr;
    if (wr_prot) begin
      ram[w_wptr] <= wr_din;
      w_wptr_plus2 <= w_wptr_plus2 + 1;
      w_wptr_plus1 <= w_wptr_plus2;
      w_wptr <= w_wptr_plus1;
      wr_usedw <= w_wptr_plus1 - w_rptr;
      if (w_wptr_plus2 == w_rptr) begin
        wr_full <= 1'b1;
      end
    end

    if (!wr_rstn) begin
      w_wptr <= '0;
      w_wptr_plus1 <= 'd1;
      w_wptr_plus2 <= 'd2;
    end

  end


  logic rd_prot;
  assign rd_prot = rd_read && !rd_empty;
  T rd_data;
  always_ff @(posedge rd_clk) begin
    if (FWFT) begin
      rd_data <= ram[r_rptr];
    end else if (rd_read) rd_data <= ram[r_rptr];
    rd_empty <= r_rptr == r_wptr;
    rd_usedw <= r_wptr - r_rptr;
    if (rd_prot) begin
      if (r_rptr_plus1 == r_wptr) begin
        rd_empty <= 1'b1;
        rd_usedw <= r_wptr - r_rptr_plus1;
      end
      if (FWFT) rd_data <= ram[r_rptr_plus1];
      r_rptr <= r_rptr_plus1;
      r_rptr_plus1 <= r_rptr_plus1 + 1;

    end

    if (!rd_rstn) begin
      r_rptr <= '0;
      r_rptr_plus1 <= 'd1;
    end
  end

  if (OUT_REG) begin : out_reg_gen
    always_ff @(posedge rd_clk) rd_dout <= rd_data;
  end else begin : n_out_reg_gen
    assign rd_dout = rd_data;
  end

`ifdef VERILATOR
  logic [L2DEPTH-1:0] sim_used;
  assign sim_used = w_wptr - r_rptr;
`endif



endmodule  // dc_fifo
//verilator lint_off DECLFILENAME
module cc_gray #(
    parameter WIDTH
)
//verilator lint_on DECLFILENAME
(
    input wire clk_in,
    input wire [WIDTH-1:0] in,
    input wire clk_out,
    output logic [WIDTH-1:0] out
);

  function automatic logic [WIDTH-1:0] gray2bin(logic [WIDTH-1:0] v);
    gray2bin[WIDTH-1] = v[WIDTH-1];
    for (int b = WIDTH - 2; b >= 0; b--) begin
      gray2bin[b] = v[b] ^ gray2bin[b+1];
    end
  endfunction  // grey2bin

  function automatic logic [WIDTH-1:0] bin2gray(logic [WIDTH-1:0] v);
    bin2gray[WIDTH-1] = v[WIDTH-1];
    for (int b = WIDTH - 2; b >= 0; b--) begin
      bin2gray[b] = v[b] ^ v[b+1];
    end
  endfunction


  logic [WIDTH-1:0] ff_gray;
  logic [WIDTH-1:0] ff_gray_out0;
  logic [WIDTH-1:0] ff_gray_out1;
  logic [WIDTH-1:0] ff_bin_out;
  always_ff @(posedge (clk_in)) begin
    ff_gray <= bin2gray(in);
  end
  ;
  always_ff @(posedge (clk_out)) begin
    ff_gray_out0 <= ff_gray;
    ff_gray_out1 <= ff_gray_out0;
    ff_bin_out   <= gray2bin(ff_gray_out1);
  end
  ;
  assign out = ff_bin_out;

endmodule
`default_nettype wire
