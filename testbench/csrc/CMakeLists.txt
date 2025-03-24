
function(create_test name )
   cmake_parse_arguments(MRtest ""
    "VERILATOR_LIBRARY;XSIM_LIBRARY"
    "CXX_SOURCES"
    ${ARGN})

  if(DEFINED MRtest_VERILATOR_LIBRARY)
    if(NOT SKIP_VERILATOR)
      add_executable(${name} ${MRtest_CXX_SOURCES})
      target_link_libraries(${name} PUBLIC ${MRtest_VERILATOR_LIBRARY})
      set_property(TARGET ${name} PROPERTY CXX_STANDARD 20)
      target_compile_options(${name} PRIVATE -Wall   -Werror)
      add_dependencies(${name} format_source)
      add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
      set_target_properties( ${name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()
  endif()
  if(DEFINED MRtest_XSIM_LIBRARY)
    if(NOT SKIP_XSIM)
      add_executable(${name}_xsim ${MRtest_CXX_SOURCES})
      set_target_properties( ${name}_xsim
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
      target_link_libraries(${name}_xsim PUBLIC  ${MRtest_XSIM_LIBRARY})
      target_compile_definitions(${name}_xsim PUBLIC -DUSE_XSIM=1)
      set_property(TARGET ${name}_xsim PROPERTY CXX_STANDARD 20)
      target_compile_options(${name}_xsim PRIVATE -Wall   -Werror)
      add_dependencies(${name}_xsim format_source)
      add_test(NAME ${name}_xsim COMMAND $<TARGET_FILE:${name}_xsim>)
    endif()
  endif()

endfunction()

if(NOT SKIP_XSIM)
  add_xsim_library(dc_ram_xsim TOPLEVEL dc_ram SV_SOURCES ../../rtl/dc_ram.sv)
  add_xsim_library(dc_fifo_xsim TOPLEVEL dc_fifo SV_SOURCES ../../rtl/dc_fifo.sv)
endif()
if(NOT SKIP_VERILATOR)
add_verilator_library(dc_ram ../../rtl/dc_ram.sv)

add_verilator_library(dc_fifo ../../rtl/dc_fifo.sv)
verilate(dc_fifo SOURCES ../../rtl/dc_fifo.sv PREFIX Vdc_fifo_fwft TRACE_FST TRACE_STRUCT VERILATOR_ARGS -GFWFT=1'b1)
verilate(dc_fifo SOURCES ../../rtl/dc_fifo.sv PREFIX Vdc_fifo_outreg TRACE_FST TRACE_STRUCT VERILATOR_ARGS -GOUT_REG=1'b1)
endif()
#add_verilator_library(dc_fifo ../../rtl/dc_fifo.sv)

create_test(dc_ram_test
  CXX_SOURCES dc_ram_test.cpp
  VERILATOR_LIBRARY dc_ram
  XSIM_LIBRARY dc_ram_xsim)

create_test(dc_fifo_test
  CXX_SOURCES dc_fifo_test.cpp
  VERILATOR_LIBRARY dc_fifo
  XSIM_LIBRARY dc_fifo_xsim
)
create_test(dc_fifo_fwft_test
  CXX_SOURCES dc_fifo_fwft_test.cpp
  VERILATOR_LIBRARY dc_fifo)

create_test(dc_fifo_outreg_test
  CXX_SOURCES dc_fifo_outreg_test.cpp
  VERILATOR_LIBRARY dc_fifo)
