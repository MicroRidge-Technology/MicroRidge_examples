cmake_minimum_required(VERSION 3.25)

project(MicroRidge_examples)


find_package(verilator 5.018 HINTS $ENV{VERILATOR_ROOT})

function(add_verilator_library name source)
  add_library(${name} EXCLUDE_FROM_ALL STATIC)
  verilate(${name} SOURCES ${source}
    TRACE_FST TRACE_STRUCT
  )
endfunction()
add_verilator_library(dc_ram ../rtl/dc_ram.sv)
add_verilator_library(dc_fifo ../rtl/dc_fifo.sv)


function(create_test name )
   cmake_parse_arguments(MRtest ""
    "VERILATOR_LIBRARY"
    "CXX_SOURCES"
    ${ARGN})


  add_executable(${name} ${MRtest_CXX_SOURCES})
  target_link_libraries(${name} PUBLIC ${MRtest_VERILATOR_LIBRARY})
  set_property(TARGET ${name} PROPERTY CXX_STANDARD 20)
  target_compile_options(${name} PRIVATE -Wall   -Werror)
endfunction()

create_test(dc_ram_test
  CXX_SOURCES csrc/dc_ram_test.cpp
  VERILATOR_LIBRARY dc_ram)

create_test(dc_fifo_test
  CXX_SOURCES csrc/dc_fifo_test.cpp
  VERILATOR_LIBRARY dc_fifo)
