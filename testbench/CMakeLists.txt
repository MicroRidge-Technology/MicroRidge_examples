cmake_minimum_required(VERSION 3.22)

project(MicroRidge_examples)

set(SKIP_XSIM N CACHE BOOL "Skip Xsim test (Used when vivado not available)")
set(SKIP_VERILATOR N CACHE BOOL "Skip verilator test (Used when verilator not available)")
if(NOT ${SKIP_VERILATOR})
    include(drivers/verilator.cmake)
endif()
if(NOT ${SKIP_XSIM})
  include(drivers/xsim.cmake)
endif()
enable_testing()

add_subdirectory(csrc)

