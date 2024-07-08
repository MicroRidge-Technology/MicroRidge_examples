set tclDir [file dirname [info script]]
set PROJ_NAME xem8320_xvc
source -notrace $tclDir/../../common/vivado_create_proj.tcl

set rtl_files [list]
set constr_files [list]
set include_dirs [list]
set bd_tcl_files [list]
lappend rtl_files $tclDir/../rtl/Top.sv
lappend bd_tcl_files $tclDir/pciebd.tcl
lappend constr_files $tclDir/../constr/xem8320.xdc

set dobackend 0
foreach arg $::argv {
    if { $arg == "dobackend" } {
        set dobackend 1
    }
} 


create_proj xem8320_xvc  xcau25p-ffvb676-2-e $rtl_files $constr_files $include_dirs $bd_tcl_files
if { $dobackend } {
    run_backend
}
