#add_verilator_library(dc_ram ../../rtl/dc_ram.sv)
#add_verilator_library(dc_fifo ../../rtl/dc_fifo.sv)

add_xsim_library(dc_ram_xsim TOPLEVEL dc_ram SV_SOURCES ../../rtl/dc_ram.sv)
add_xsim_library(counter TOPLEVEL counter VERILOG_SOURCES ../../rtl/counter.v)

add_executable(dc_ram_test_xsim
  dc_ram_test_xsim.cpp
)
target_link_xsim_library(dc_ram_test_xsim dc_ram_xsim)
add_executable(counter_tb
  ../../xsim/testbench.cpp
)
target_link_xsim_library(counter_tb counter)
