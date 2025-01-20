

#if defined(USE_XSIM)
#include "xsim_driver.hpp"

struct dc_fifo_dut {
  static constexpr int L2DEPTH = 3;
  sim_port<uint8_t, 1> wr_clk;
  sim_port<uint8_t, 1> rd_clk;
  sim_port<uint8_t, 1> wr_rstn;
  sim_port<uint8_t, 1> wr_write;
  sim_port<uint8_t, 1> wr_full;
  sim_port<uint8_t, L2DEPTH> wr_usedw;
  sim_port<uint8_t, 1> rd_rstn;
  sim_port<uint8_t, 1> rd_read;
  sim_port<uint8_t, 1> rd_empty;
  sim_port<uint8_t, L2DEPTH> rd_usedw;
  sim_port<uint16_t, 16> wr_din;
  sim_port<uint16_t, 16> rd_dout;

  dc_fifo_dut(xsiHandle handle)
      : sim_port_construct(wr_clk), sim_port_construct(rd_clk),
        sim_port_construct(wr_rstn), sim_port_construct(wr_write),
        sim_port_construct(wr_full), sim_port_construct(wr_usedw),
        sim_port_construct(rd_rstn), sim_port_construct(rd_read),
        sim_port_construct(rd_empty), sim_port_construct(rd_usedw),
        sim_port_construct(wr_din), sim_port_construct(rd_dout)

  {}
};
using driver_t = xsim_driver<dc_fifo_dut>;
#else
#include "Vdc_fifo.h"
#include "verilator_driver.hpp"
#include <cstdint>
#include <cstdlib>

using driver_t = verilator_driver<Vdc_fifo>;
#endif
using namespace std::chrono_literals;
class dc_fifo_test : public driver_t {
protected:
  ClockDriver wr_clockdriver, rd_clockdriver;

public:
  void tick_wr(int ticks = 1) {
    while (ticks--) {
      run_until_rising_edge(wr_clockdriver);
    }
  }
  std::vector<uint16_t> read_data;
  double read_prob;
  void on_read_clock(ClockDriver::edge_e) {
    if (dut->rd_read) {
      // if 2 clocks set the read byte,
      // this clock has the data
      read_data.push_back(dut->rd_dout);
    }
    dut->rd_read = 0;
    if (!dut->rd_empty) {
      if (rand() / double(RAND_MAX) < read_prob) {
        dut->rd_read = 1;
      }
    }
  }

  void do_reset() {
    dut->rd_rstn = 0;
    dut->wr_rstn = 0;
    run(1us);
    dut->rd_rstn = 1;
    dut->wr_rstn = 1;
  }

  void test(float wr_prob, float rd_prob, int test_length) {

    this->read_prob = rd_prob;
    std::vector<uint16_t> write_data;
    for (int i = 0; i < test_length; ++i) {
      write_data.push_back(rand());
    }
    do_reset();
    read_data.clear();
    int stalls = 0;
    dut->wr_write = 0;
    for (int i = 0; i < test_length; ++i) {
      while (1) {

        if (rand() / double(RAND_MAX) < wr_prob) {
          if (dut->wr_full) {
            tick_wr();
            stalls++;
          } else {
            dut->wr_write = 1;
            dut->wr_din = write_data[i];
            tick_wr();
            dut->wr_write = 0;
            break;
          }
        } else {
          tick_wr();
        }
      }
    }
    // debug(stalls);
    run(1us);
    while (!dut->rd_empty) {
      run(1us);
    }

    except_assert(read_data.size() == write_data.size());

    for (unsigned i = 0; i < read_data.size(); ++i) {
      if (read_data.at(i) != write_data.at(i)) {
        debug(i);
        except_assert(read_data[i] == write_data[i]);
      }
    }
  }

  dc_fifo_test(int argc, char **argv)
      : driver_t(argc, argv),
        wr_clockdriver(
            ClockDriver([&](uint8_t clk) { dut->wr_clk = clk; }, 10ns)),
        rd_clockdriver(
            ClockDriver([&](uint8_t clk) { dut->rd_clk = clk; }, 11ns)) {
    add_clock(wr_clockdriver);
    add_clock(rd_clockdriver);

    rd_clockdriver.add_callback(
        [&](ClockDriver::edge_e e) { on_read_clock(e); });

    dut->rd_read = 0;
    dut->wr_write = 0;

    dut->rd_rstn = 1;
    dut->wr_rstn = 1;

    run(1us);
    dut->rd_rstn = 0;
    dut->wr_rstn = 0;

    tick_wr();

    test(.9, .1, 10);
    test(.1, .9, 10);

    test(.9, .1, 1000);
    test(.1, .9, 1000);
  }
};

int main(int argc, char **argv) {

  try {
    dc_fifo_test test(argc, argv);
  } catch (std::exception &e) {
    printf("Test Failed:\n\t%s\n", e.what());
    return 1;
  }
  printf("Test Passed!\n");
  return 0;
}
