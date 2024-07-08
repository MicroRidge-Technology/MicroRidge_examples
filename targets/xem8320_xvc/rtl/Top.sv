`default_nettype none
/**/
`timescale 1ns / 1ns

module Top (


    input wire sys_clkp,
    input wire sys_clkn,
    input wire pcie_ref_clk_n,
    input wire pcie_ref_clk_p,
    input wire [3:0] pcie_rxn,
    input wire [3:0] pcie_rxp,
    output wire [3:0] pcie_txn,
    output wire [3:0] pcie_txp,
    input wire perstn,
    output wire [5:0] led
);
  // Clock
  wire sys_clk;
  IBUFGDS osc_clk (
      .O (sys_clk),
      .I (sys_clkp),
      .IB(sys_clkn)
  );


  wire clk100;
  logic resetn;
  wire uart_rtl_0_rxd;
  wire uart_rtl_0_txd;

  wire [3:0] config_spi_dat_i;
  wire [3:0] config_spi_dat_o;
  wire [3:0] config_spi_dat_t;
  wire config_spi_sck_i;
  wire config_spi_sck_o;
  wire config_spi_sck_t;
  wire config_spi_ss_i;
  wire config_spi_ss_o;
  wire config_spi_ss_t;
  pciebd pciebd_i (
      .clk100(sys_clk),
      .resetn(resetn),
      .gpio_led_tri_o(led),
      .pcie_ref_clk_n(pcie_ref_clk_n),
      .pcie_ref_clk_p(pcie_ref_clk_p),
      .pcie_rxn(pcie_rxn),
      .pcie_rxp(pcie_rxp),
      .pcie_txn(pcie_txn),
      .pcie_txp(pcie_txp),
      .perstn(perstn)
  );

  logic [8:0] reset_count = 0;
  always_ff @(posedge sys_clk) begin
    reset_count <= reset_count == '1 ? '1 : reset_count + 1;
    resetn <= &(reset_count);
  end

endmodule

`default_nettype wire
