find_package(verilator 5.018 HINTS $ENV{VERILATOR_ROOT})

function(add_verilator_library name source)
  add_library(${name} EXCLUDE_FROM_ALL STATIC)
  verilate(${name} SOURCES ${source}
    TRACE_FST TRACE_STRUCT
    VERILATOR_ARGS --timing
  )
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
  # Verilator's installed headers (verilated_funcs.h, verilated_types.h)
  # contain int-vs-size_t comparisons that trip -Wsign-compare under -Werror.
  # Suppress the warning for any consumer that links this library.
  target_compile_options(${name} PUBLIC -Wno-sign-compare)
endfunction()
