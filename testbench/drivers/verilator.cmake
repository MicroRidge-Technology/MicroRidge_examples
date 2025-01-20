find_package(verilator 5.018 HINTS $ENV{VERILATOR_ROOT})

function(add_verilator_library name source)
  add_library(${name} EXCLUDE_FROM_ALL STATIC)
  verilate(${name} SOURCES ${source}
    TRACE_FST TRACE_STRUCT
  )
endfunction()
