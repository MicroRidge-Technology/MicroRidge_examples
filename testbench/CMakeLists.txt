cmake_minimum_required(VERSION 3.22)

project(MicroRidge_examples)

include(drivers/verilator.cmake)
include(drivers/xsim.cmake)

add_subdirectory(csrc)
