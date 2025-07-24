# MicroRidge Examples

Example SystemVerilog designs and testbenches demonstrating dual-clock FIFO and RAM implementations for FPGA development.

## Overview

This repository contains:
- **RTL modules**: Dual-clock FIFO and RAM implementations in SystemVerilog
- **Testbenches**: C++ testbenches using Verilator and Xilinx Vivado XSim
- **FPGA targets**: Example projects for various development boards
- **Build system**: CMake-based build with automated formatting

## RTL Modules

### dc_fifo.sv
Parametrizable dual-clock FIFO with configurable:
- Data type and width
- Depth (log2 parameter)
- First-Word Fall-Through (FWFT) mode
- Output register option

### dc_ram.sv
Dual-port RAM with independent clock domains for each port, supporting simultaneous read/write operations.

## Directory Structure

```
├── rtl/                    # SystemVerilog source files
│   ├── dc_fifo.sv         # Dual-clock FIFO implementation
│   └── dc_ram.sv          # Dual-port RAM implementation
└── testbench/             # C++ testbenches and drivers
    ├── csrc/              # Test source files
    └── drivers/           # Simulation driver abstractions
```

## Building and Testing

### Prerequisites
- CMake 3.22+
- C++20 compatible compiler
- Verilator (optional - set `SKIP_VERILATOR=Y` to disable)
- Xilinx Vivado with XSim (optional - set `SKIP_XSIM=Y` to disable)

### Build Commands

```bash
mkdir build && cd build
cmake ../testbench
make -j$(nproc)
```

### Running Tests

```bash
# Run all tests
ctest

# Run specific tests
./bin/dc_fifo_test      # Verilator-based test
./bin/dc_fifo_test_xsim # XSim-based test
./bin/dc_ram_test       # RAM test
```

## Testbench Architecture

This repository demonstrates modern hardware verification methodologies using C++ testbenches with multiple simulator backends:

### Simulator Support
- **Verilator**: Open-source SystemVerilog simulator with cycle-accurate simulation
- **Xilinx XSim**: Commercial simulator from Xilinx Vivado toolchain

### C++ Testbench Framework
The testbenches showcase:
- **Driver abstraction**: Common interface (`sim_driver.hpp`) supporting both Verilator and XSim
- **Clock domain management**: Separate clock drivers for multi-clock designs
- **Unified test execution**: Same C++ test code runs on both simulators
- **Modern C++20**: Template-based design with type safety and performance

### Key Features
- **Polymorphic simulation**: Switch between simulators without changing test code
- **Clock-accurate testing**: Precise timing control for dual-clock domain verification
- **Automated test discovery**: CMake automatically builds tests for available simulators
- **Performance comparison**: Compare simulation speed between open-source and commercial tools

This approach demonstrates how to create portable, maintainable hardware testbenches that can leverage different simulation engines while maintaining a single codebase.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) for details.

Copyright (c) 2024, MicroRidge Technology
