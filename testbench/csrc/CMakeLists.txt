#add_verilator_library(dc_ram ../../rtl/dc_ram.sv)
#add_verilator_library(dc_fifo ../../rtl/dc_fifo.sv)

add_xsim_library(dc_ram_xsim TOPLEVEL dc_ram SV_SOURCES ../../rtl/dc_ram.sv)

add_executable(dc_ram_test_xsim
  dc_ram_test_xsim.cpp
)
target_link_xsim_library(dc_ram_test_xsim dc_ram_xsim)
