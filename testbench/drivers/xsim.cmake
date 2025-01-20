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
    "VERILOG_SOURCES;SV_SOURCES;ELAB_ARGS"
    ${ARGN})


  if(NOT DEFINED XSIMtest_TOPLEVEL)
    message(SEND_ERROR "TOPLEVEL ${XSIMtest_TOPLEVEL} must be defined")
  endif()

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
  add_custom_command(OUTPUT lib${name}_xsim.so
    DEPENDS ${XSIMtest_VERILOG_SOURCES} ${XSIMtest_SV_SOURCES}
    COMMAND ${VERILOG_CMD}
    COMMAND ${SV_CMD}
    COMMAND ${XELAB} work_${XSIMtest_TOPLEVEL}.${XSIMtest_TOPLEVEL} ${XSIMtest_ELAB_ARGS} -dll -s ${XSIMtest_TOPLEVEL} -debug wave
    COMMAND cmake -E create_symlink xsimk.so xsim.dir/${XSIMtest_TOPLEVEL}/lib${name}_xsim.so)
  add_custom_target(${name}
    DEPENDS  lib${name}_xsim.so
  )
  define_property(TARGET PROPERTY XSIM_DIR)
  set_property(TARGET ${name} PROPERTY XSIM_DIR ${CMAKE_CURRENT_BINARY_DIR}/xsim.dir/${XSIMtest_TOPLEVEL})
endfunction()
function(target_link_xsim_library target library_name)
  add_dependencies(${target} ${library_name})
  get_target_property(xsim_dir ${library_name} XSIM_DIR)

  target_include_directories(${target} PUBLIC ${VIVADO_BIN_DIR}/../data/xsim/include/)
  target_link_directories(${target} PUBLIC ${VIVADO_BIN_DIR}/../lib/lnx64.o  ${xsim_dir}/)
  target_link_libraries(${target} PUBLIC xv_simulator_kernel ${library_name}_xsim)
  target_link_options(${target} PUBLIC -Wl,--disable-new-dtags)
  target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
endfunction()
