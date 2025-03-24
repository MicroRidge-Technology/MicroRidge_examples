#include "Vdc_fifo_outreg.h"
#include "verilator_driver.hpp"
#include <cstdint>
#include <cstdlib>

using namespace std::chrono_literals;
class dc_fifo_test : public verilator_driver<Vdc_fifo_outreg> {
protected:
public:
  void tick_wr(int ticks = 1) {
    while (ticks--) {
      run_until_rising_edge(dut->wr_clk);
    }
  }
  std::vector<uint16_t> read_data;
  double read_prob;
  int rd_read_d;
  void on_read_clock(ClockDriver::edge_e) {
    if (rd_read_d) {
      // if 2 clocks set the read byte,
      // this clock has the data
      read_data.push_back(dut->rd_dout);
    }
    rd_read_d = dut->rd_read;
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
    debug(stalls);
    run(1us);
    while (!dut->rd_empty) {
      run(1us);
    }
    except_assert(read_data.size() == write_data.size());

    for (unsigned i = 0; i < read_data.size(); ++i) {
      if (read_data.at(i) != write_data.at(i)) {
        debug(i);
        debugx(read_data[i]);
        debugx(write_data[i]);
        except_assert(read_data[i] == write_data[i]);
      }
    }
  }

  dc_fifo_test(int argc, char **argv) : verilator_driver(argc, argv) {
    add_clock(dut->wr_clk, 10ns);
    auto &rd_clockdriver = add_clock(dut->rd_clk, 11ns);

    rd_clockdriver.add_callback(
        [&](ClockDriver::edge_e e) { on_read_clock(e); });

    rd_read_d = 0;

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
