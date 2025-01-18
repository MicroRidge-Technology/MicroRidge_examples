cmake_minimum_required(VERSION 3.22)

project(MicroRidge_examples)

find_program(VIVADO_BIN vivado
    PATHS  /tools/Xilinx/Vivado/2024.2/bin/ /opt/Xilinx/Vivado/2024.2/bin/
    REQUIRED
  )
get_filename_component(VIVADO_BIN_DIR ${VIVADO_BIN} DIRECTORY)
find_program(XELAB xelab
    PATH ${VIVADO_BIN_DIR})
find_program(XVLOG xvlog
  PATH ${VIVADO_BIN_DIR})

function(create_xsim_library name)
  cmake_parse_arguments(XSIMtest ""
    "TOPLEVEL"
    "VERILOG_SOURCES;SV_SOURCES;ELAB_ARGS"
    ${ARGN})
  if(DEFINED XSIMtest_SV_SOURCES)
    set(SV_CMD ${XVLOG} -work work_${XSIMtest_TOPLEVEL} ${XSIMtest_VLOG_ARGS} --incr -sv ${XSIMtest_SV_SOURCES})
  else()
    set(SV_CMD true)
  endif()

  if(DEFINED XSIMtest_VERILOG_SOURCES)
    set(VERILOG_CMD ${XVLOG} -work work_${XSIMtest_TOPLEVEL} ${XSIMtest_VLOG_ARGS} --incr  ${XSIMtest_VERILOG_SOURCES})
  else()
    set(VERILOG_CMD true)
  endif()
  add_custom_command(OUTPUT xsim.dir/${XSIMtest_TOPLEVEL}/lib${XSIMtest_TOPLEVEL}_sim.so
    DEPENDS ${XSIMtest_VERILOG_SOURCES} ${XSIMtest_SV_SOURCES}
    COMMAND ${VERILOG_CMD}
    COMMAND ${SV_CMD}
    COMMAND ${XELAB} work_${XSIMtest_TOPLEVEL}.${XSIMtest_TOPLEVEL} ${XSIMtest_ELAB_ARGS} -dll -s ${XSIMtest_TOPLEVEL} -debug wave
    COMMAND cmake -E copy_if_different xsim.dir/${XSIMtest_TOPLEVEL}/xsimk.so xsim.dir/${XSIMtest_TOPLEVEL}/lib${XSIMtest_TOPLEVEL}_sim.so) 
  add_custom_target(${name}
    DEPENDS  xsim.dir/${XSIMtest_TOPLEVEL}/lib${XSIMtest_TOPLEVEL}_sim.so
  )
endfunction()
function(target_link_xsim_library target library_name)
  add_dependencies(${target} ${library_name})
  target_include_directories(${target} PUBLIC ${VIVADO_BIN_DIR}/../data/xsim/include/)
  target_link_directories(${target} PUBLIC ${VIVADO_BIN_DIR}/../lib/lnx64.o  ${CMAKE_BINARY_DIR}/xsim.dir/${library_name}/)
  target_link_libraries(${target} PUBLIC xv_simulator_kernel ${library_name}_sim)
  target_link_options(${target} PUBLIC -Wl,--disable-new-dtags)
endfunction()

create_xsim_library(counter TOPLEVEL counter VERILOG_SOURCES ../rtl/counter.v)
add_executable(testbench
  testbench.cpp
)
target_link_xsim_library(testbench counter)

create_xsim_library(dc_ram TOPLEVEL dc_ram SV_SOURCES ../rtl/dc_ram.sv)
add_executable(dc_ram_test_xsim
  dc_ram_test_xsim.cpp
)
target_link_xsim_library(dc_ram_test_xsim dc_ram)
