cmake_minimum_required(VERSION 3.22)

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

set(CLANGFORMAT_DL_URL https://github.com/muttleyxd/clang-tools-static-binaries/releases/download/master-32d3ac78/clang-format-18_linux-amd64)
set(VERIBLE_DL_URL https://github.com/chipsalliance/verible/releases/download/v0.0-3756-gda9a0f8c/verible-v0.0-3756-gda9a0f8c-linux-static-x86_64.tar.gz)
if(NOT EXISTS  ${CMAKE_BINARY_DIR}/verible.tar.gz)
  file(DOWNLOAD ${VERIBLE_DL_URL} ${CMAKE_BINARY_DIR}/verible.tar.gz EXPECTED_MD5 c57f0583e2f6fae18e5d83887afcf039)
  execute_process(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E tar xzf verible.tar.gz)
endif()
if(NOT EXISTS  ${CMAKE_BINARY_DIR}/clang-format)
  file(DOWNLOAD ${CLANGFORMAT_DL_URL} ${CMAKE_BINARY_DIR}/clang-format EXPECTED_MD5 2c7e34412c47b61a64262691a87d4abe)
   execute_process(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND chmod +x ${CMAKE_BINARY_DIR}/clang-format)
endif()
add_custom_target(format_source ALL
  COMMAND git ls-files  ${CMAKE_SOURCE_DIR}/../*.cpp  ${CMAKE_SOURCE_DIR}/../*.hpp  ${CMAKE_SOURCE_DIR}/../*.h  ${CMAKE_SOURCE_DIR}/../*.c | xargs ${CMAKE_BINARY_DIR}/clang-format -i
  COMMAND git ls-files  ${CMAKE_SOURCE_DIR}/../*.sv  ${CMAKE_SOURCE_DIR}/../*.v  ${CMAKE_SOURCE_DIR}/../*.svh  ${CMAKE_SOURCE_DIR}/../*.vh | xargs ${CMAKE_BINARY_DIR}/verible-v0.0-3756-gda9a0f8c/bin/verible-verilog-format --inplace )
enable_testing()

function(create_test name )
   cmake_parse_arguments(MRtest ""
    "VERILATOR_LIBRARY"
    "CXX_SOURCES"
    ${ARGN})


  add_executable(${name} ${MRtest_CXX_SOURCES})
  target_link_libraries(${name} PUBLIC ${MRtest_VERILATOR_LIBRARY})
  set_property(TARGET ${name} PROPERTY CXX_STANDARD 20)
  target_compile_options(${name} PRIVATE -Wall   -Werror)
  add_dependencies(${name} format_source)
  add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endfunction()

create_test(dc_ram_test
  CXX_SOURCES csrc/dc_ram_test.cpp
  VERILATOR_LIBRARY dc_ram)

create_test(dc_fifo_test
  CXX_SOURCES csrc/dc_fifo_test.cpp
  VERILATOR_LIBRARY dc_fifo)
