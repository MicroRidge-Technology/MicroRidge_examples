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

add_executable(dc_ram_test csrc/dc_ram_test.cpp)
target_link_libraries(dc_ram_test PUBLIC dc_ram)
