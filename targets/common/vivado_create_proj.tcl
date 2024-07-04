
set mr_common_create_proj [file normalize [info script]]
proc mr_export_bd { design_name output_file } {
    open_bd_design [get_files $design_name.bd]
    validate_bd_design
    write_bd_tcl -force $output_file.tmp
    close_bd_design [get_bd_designs]
    
    # if new file is differnt than old file copy over old file
    set f1 [open $output_file.tmp.tcl]
    set f2 [open $output_file]
    if { ! [string equal [read $f1] [read $f1]] } {
        file copy -force $output_file.tmp.tcl $output_file        
    }
    file delete $output_file.tmp.tcl
}


proc create_proj { project_name part rtl_files xdc_files include_dirs bd_tcl_files } {
    global mr_common_create_proj
    set proj_dir "vivado_[clock format [clock seconds]  -format %y%m%d_%H%M]"
    create_project ${project_name} -part ${part} $proj_dir
    if { [file exists vivado_latest] } {
        file delete vivado_latest
    }
    file link vivado_latest $proj_dir
    if { [llength $rtl_files] } {
        add_files $rtl_files
    }
    if { [llength $xdc_files] } {
        add_files -fileset constrs_1 $xdc_files
    }
    if { [llength $include_dirs] } {
        set_property include_dirs $include_dirs [get_filesets sim_1]
        set_property include_dirs $include_dirs [get_filesets sources_1]
    }
    set export_bd_fname "[get_property DIRECTORY [current_project]]/export_bd.tcl" 
    set bd_export_tcl [open $export_bd_fname w]
    puts $bd_export_tcl "source -notrace $mr_common_create_proj"
    foreach bdtcl $bd_tcl_files {
        set filecontents [read [open $bdtcl]]
        regexp {design_name (\w+)} $filecontents _ bd_name
        puts $bd_export_tcl "mr_export_bd $bd_name [file normalize $bdtcl]"
        unset bd_name
        source $bdtcl
        close_bd_design [get_bd_designs]
    }
    close $bd_export_tcl
    #add_files -fileset utils_1 -norecurse $export_bd_fname 
    set_property STEPS.SYNTH_DESIGN.TCL.POST $export_bd_fname [get_runs synth_1]

    update_compile_order -fileset sources_1
    set_property STEPS.WRITE_BITSTREAM.ARGS.BIN_FILE true [get_runs impl_1]
    
}


proc run_backend {} {
    launch_runs synth_1 -jobs 8;
    wait_on_run synth_1;

    launch_runs impl_1 -to_step write_bitstream;
    wait_on_run impl_1;
    
}
