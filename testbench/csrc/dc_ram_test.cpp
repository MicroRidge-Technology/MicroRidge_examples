
#if defined(USE_XSIM)
#include "dc_ram_xsim.hpp"
#include "xsim_driver.hpp"
using driver_t = xsim_driver<dc_ram_xsim>;
#else
#include "Vdc_ram.h"
#include "verilator_driver.hpp"

using driver_t = verilator_driver<Vdc_ram>;
#endif
using namespace std::chrono_literals;
class dc_ram_test : public driver_t {
  ClockDriver cd_a, cd_b;
  void tick_a(int ticks = 1) {
    while (ticks--) {
      run_until_rising_edge(dut->clk_a);
    }
  }
  void tick_b(int ticks = 1) {
    while (ticks--) {
      run_until_rising_edge(dut->clk_b);
    }
  }

public:
  dc_ram_test(int argc, char **argv)
      : driver_t(argc, argv),
        cd_a(ClockDriver([&](uint8_t clk) { dut->clk_a = clk; }, 15ns)),
        cd_b(ClockDriver([&](uint8_t clk) { dut->clk_b = clk; }, 10ns)) {
    add_clock(dut->clk_a, 15ns);
    add_clock(dut->clk_b, 10ns);
    dut->we_a = 0;
    dut->we_b = 0;
    tick_a(10);

    run_until_rising_edge(dut->clk_a);

    int ram_depth = (1 << 6);

    /* write the ram on port a
     * read back on port a, verify contents
     * read back on port b, verify contents
     *
     * write the ram on port b
     * read back on port b, verify contents
     * read back on port a, verify contents
     */

    for (int i = 0; i < ram_depth; ++i) {
      dut->we_a = 1;
      dut->data_a = ~i;
      dut->addr_a = i;
      tick_a();
      dut->we_a = 0;
      tick_a();
    }

    for (uint8_t i = 0; i < ram_depth; ++i) {
      dut->addr_a = i;
      uint8_t expected_val = ~i;
      except_assert(dut->q_a != expected_val);
      tick_a();
      except_assert(dut->q_a == expected_val);
    }
    dut->addr_b = 17;
    tick_b(10);

    for (uint8_t i = 0; i < ram_depth; ++i) {
      dut->addr_b = i;
      uint8_t expected_val = ~i;
      except_assert(dut->q_b != expected_val);
      tick_b();
      except_assert(dut->q_b == expected_val);
    }

    /*write on port b*/
    for (int i = 0; i < ram_depth; ++i) {
      dut->we_b = 1;
      dut->data_b = ~i + 7;
      dut->addr_b = i;
      tick_b();
      dut->we_b = 0;
      tick_b();
    }

    for (uint8_t i = 0; i < ram_depth; ++i) {
      dut->addr_b = i;
      uint8_t expected_val = ~i + 7;
      except_assert(dut->q_b != expected_val);
      tick_b();
      except_assert(dut->q_b == expected_val);
    }

    tick_a(10);
    for (uint8_t i = 0; i < ram_depth; ++i) {
      dut->addr_a = i;
      uint8_t expected_val = ~i + 7;
      except_assert(dut->q_a != expected_val);
      tick_a();
      except_assert(dut->q_a == expected_val);
    }
  }
};

int main(int argc, char **argv) {

  try {
    dc_ram_test test(argc, argv);
  } catch (std::exception &e) {
    printf("Test Failed:\n\t%s\n", e.what());
    return 1;
  }
  printf("Test Passed!\n");
  return 0;
}
