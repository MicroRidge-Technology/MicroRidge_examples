VIVADO_VERSION?=2023.2

VIVADO_SEARCH:=$(shell which vivado)
VIVADO_SEARCH+=$(wildcard /tools/Xilinx/Vivado/$(VIVADO_VERSION)/bin/vivado)
VIVADO_SEARCH+=$(wildcard /opt/Xilinx/Vivado/$(VIVADO_VERSION)/bin/vivado)
VIVADO_BIN=$(firstword $(VIVADO_SEARCH))

usage:
	@printf "targets:\n"
	@printf "   gui:       Open vivado gui, generate the project\n"
	@printf "   build:     Run vivado in batch mode, generate the project, run tools to create bitstream\n"
	@printf "   gui-build: Run vivado in GUI mode, generate the project, run tools to create bitstream\n"
	@printf "   reopen:    Reopen  vivado in GUI mode, generate the project, run tools to create bitstream\n"
	@printf "   clean:     delete old builds\n"


gui:
	$(VIVADO_BIN) -nolog -nojournal -mode gui -source tcl/create_proj.tcl &

gui-build:
	$(VIVADO_BIN)  -nolog -nojournal -mode gui -source tcl/create_proj.tcl -tclargs dobackend &

build:
	$(VIVADO_BIN)  -nolog -nojournal -mode batch -source tcl/create_proj.tcl -tclargs dobackend

LATEST_PROJ=$(wildcard vivado_latest/*.xpr)
reopen:
ifeq ($(LATEST_PROJ),)
	@printf "ERROR: No recent projects to open\n";exit 1
else
	$(VIVADO_BIN)  -nolog -nojournal -mode gui $(LATEST_PROJ) &
endif
clean:
	rm -rf vivado_* *.log *.jou
