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

function(add_xsim_library name)
  cmake_parse_arguments(XSIMtest ""
    "TOPLEVEL"
    "VERILOG_SOURCES;SV_SOURCES;VLOG_ARGS;ELAB_ARGS"
    ${ARGN})


  if(NOT DEFINED XSIMtest_TOPLEVEL)
    message(SEND_ERROR "TOPLEVEL ${XSIMtest_TOPLEVEL} must be defined")
  endif()

  if(DEFINED XSIMtest_SV_SOURCES)
    set(SV_CMD ${XVLOG} -work work_${XSIMtest_TOPLEVEL} ${XSIMtest_VLOG_ARGS} --incr -sv ${XSIMtest_SV_SOURCES})
  else()
    set(SV_CMD true)
  endif()

  set(VERILOG_CMD ${XVLOG} -work work_${XSIMtest_TOPLEVEL} ${XSIMtest_VLOG_ARGS} --incr  ${XSIMtest_VERILOG_SOURCES}  ${VIVADO_BIN_DIR}/../data/verilog/src/glbl.v)

  add_library(${name} INTERFACE)
  add_custom_command(OUTPUT xsim.dir/${XSIMtest_TOPLEVEL}/lib${name}_xsim.so
    DEPENDS ${XSIMtest_VERILOG_SOURCES} ${XSIMtest_SV_SOURCES} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/xsim_header_gen.cpp
    COMMAND ${VERILOG_CMD}
    COMMAND ${SV_CMD}
    COMMAND ${XELAB} work_${XSIMtest_TOPLEVEL}.${XSIMtest_TOPLEVEL}  work_${XSIMtest_TOPLEVEL}.glbl ${XSIMtest_ELAB_ARGS} -dll -s ${XSIMtest_TOPLEVEL} -debug wave
    COMMAND cmake -E create_symlink xsimk.so xsim.dir/${XSIMtest_TOPLEVEL}/lib${name}_xsim.so
    COMMAND ${CMAKE_CXX_COMPILER}  -Wl,--disable-new-dtags ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/xsim_header_gen.cpp
    -o xsim.dir/${XSIMtest_TOPLEVEL}/${XSIMtest_TOPLEVEL}_gen
    -I${VIVADO_BIN_DIR}/../data/xsim/include -L${VIVADO_BIN_DIR}/../lib/lnx64.o
    -L${CMAKE_CURRENT_BINARY_DIR}/xsim.dir/${XSIMtest_TOPLEVEL} -Wl,-rpath,${VIVADO_BIN_DIR}/../lib/lnx64.o:${CMAKE_CURRENT_BINARY_DIR}/xsim.dir/${XSIMtest_TOPLEVEL}
    -lxv_simulator_kernel  -l${name}_xsim
    COMMAND  xsim.dir/${XSIMtest_TOPLEVEL}/${XSIMtest_TOPLEVEL}_gen xsim.dir/${XSIMtest_TOPLEVEL}/${name}.hpp ${name}
    COMMAND ${CMAKE_COMMAND} -E remove xsim.dir/${XSIMtest_TOPLEVEL}/${XSIMtest_TOPLEVEL}_gen
    )
  add_custom_target(${name}_cst_tgt
    DEPENDS xsim.dir/${XSIMtest_TOPLEVEL}/lib${name}_xsim.so)
  target_include_directories(${name} INTERFACE ${VIVADO_BIN_DIR}/../data/xsim/include/ ${CMAKE_CURRENT_BINARY_DIR}/xsim.dir/${XSIMtest_TOPLEVEL})
  target_link_directories(${name} INTERFACE ${VIVADO_BIN_DIR}/../lib/lnx64.o  ${CMAKE_CURRENT_BINARY_DIR}/xsim.dir/${XSIMtest_TOPLEVEL})
  target_link_libraries(${name} INTERFACE xv_simulator_kernel ${name}_xsim)
  target_link_options(${name} INTERFACE -Wl,--disable-new-dtags)
  target_include_directories(${name} INTERFACE ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
  add_dependencies(${name}
    ${name}_cst_tgt
  )
endfunction()

file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/open_wdb
    CONTENT "#!/bin/sh
#\\
${VIVADO_BIN} -source $0 -tclargs $@
#\\
exit $?
set wave $argv
set cfg $wave.wcfg
open_wave_database $wave
if { [file exists $cfg ] } {
    open_wave_config $cfg
}
proc reopen_wave {} {
    global cfg
    global wave
    save_wave_config $cfg
    close_sim
    open_wave_database $wave
    open_wave_config $cfg
}
"
    FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
