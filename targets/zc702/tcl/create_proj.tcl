set tclDir [file dirname [info script]]

source -notrace $tclDir/../../common/create_proj.tcl

set rtl_files [list]
set constr_files [list]
set include_dirs [list]
set bd_tcl_files [list]
lappend rtl_files $tclDir/../rtl/Top.sv
lappend bd_tcl_files $tclDir/psbd.tcl
lappend constr_files $tclDir/../constr/zc702.xdc

set dobackend 0
foreach arg $::argv {
    if { $arg == "dobackend" } {
        set dobackend 1
    }
} 


create_proj zc702  xc7z020clg484-1 $rtl_files $constr_files $include_dirs $bd_tcl_files
if { $dobackend } {
    run_backend
}
