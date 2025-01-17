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
  if(DEFINED XSIMtest_SVLOG)
    set(SV_CMD ${XVLOG} -work work_${XSIMtest_TOPLEVEL} ${XSIMtest_VLOG_ARGS} --incr -sv ${XSIMtest_SVLOG})
  else()
    set(SV_CMD true)
  endif()
  add_custom_command(OUTPUT xsim.dir/${XSIMtest_TOPLEVEL}/xsimk.so
    DEPENDS ${XSIMtest_VERILOG_SOURCES} ${XSIMtest_SV_SOURCES}
    COMMAND ${XVLOG} -work work_${XSIMtest_TOPLEVEL} ${XSIMtest_VLOG_ARGS} --incr ${XSIMtest_VERILOG_SOURCES}
    COMMAND ${SV_CMD}
    COMMAND ${XELAB} work_${XSIMtest_TOPLEVEL}.${XSIMtest_TOPLEVEL} ${XSIMtest_ELAB_ARGS} -dll -s ${XSIMtest_TOPLEVEL} -debug wave)
  add_custom_target(${name}
    DEPENDS  xsim.dir/${XSIMtest_TOPLEVEL}/xsimk.so)
endfunction()


create_xsim_library(counter TOPLEVEL counter VERILOG_SOURCES ../rtl/counter.v)

add_executable(testbench
  testbench.cpp
  xsi_loader.cpp
)
add_dependencies(testbench counter)
target_include_directories(testbench PUBLIC ${VIVADO_BIN_DIR}/../data/xsim/include/)
