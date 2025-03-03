/**
 * Copyright (2024) MicroRidge Technology LTD.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **/

#ifndef SIM_DRIVER_HPP
#define SIM_DRIVER_HPP
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

#define debug(a)                                                               \
  printf("%s:%d %s = %lld\n", __FILE__, __LINE__, #a, (long long)(a))

#define debugx(a)                                                              \
  printf("%s:%d %s = 0x%08llx\n", __FILE__, __LINE__, #a,                      \
         (long long unsigned)(a))

#define debugf(a) printf("%s:%d %s = %f\n", __FILE__, __LINE__, #a, (double)(a))

#define except_assert(test)                                                    \
  do {                                                                         \
    if (!(test)) {                                                             \
      char str[1000];                                                          \
      snprintf(str, 900, "%s:%d Exception: assertion %s failed!", __FILE__,    \
               __LINE__, #test);                                               \
      throw std::runtime_error(str);                                           \
    }                                                                          \
  } while (0)
#define except_assert2(test, msg)                                              \
  do {                                                                         \
    if (!(test)) {                                                             \
      char str[1000];                                                          \
      std::string msg1(msg);                                                   \
      snprintf(str, 900, "%s:%d Exception: assertion %s failed! %s", __FILE__, \
               __LINE__, #test, msg1.c_str());                                 \
      throw std::runtime_error(str);                                           \
    }                                                                          \
  } while (0)

class ClockDriver {
public:
  using duration_t = std::chrono::duration<int64_t, std::pico>;

  enum class edge_e { RISE_EDGE, FALL_EDGE, BOTH_EDGE };

private:
  duration_t down_time, up_time;
  duration_t m_last_update;
  ///< Function to set the value of the clock net
  struct callback_t {
    std::function<void(edge_e)> fun;
    edge_e edge;
  };
  std::function<void(uint8_t)> clock_fun;
  std::vector<callback_t> callback_functions;
  int clk_val;

public:
  ClockDriver(std::function<void(uint8_t)> fun,
              std::chrono::duration<long double, std::nano> period)
      : m_last_update(0), clk_val(0) {
    /**
     * Constructor for floating point nanoseconds
     */

    duration_t dur_period(int(period.count() * 1000));

    up_time = dur_period / 2;
    down_time = dur_period - up_time;
    clock_fun = fun;
    clk_val = 0;
    m_last_update = duration_t(0);
  }

  void add_callback(std::function<void(edge_e)> fun,
                    edge_e e = edge_e::RISE_EDGE) {

    callback_functions.push_back({fun, e});
  }
  duration_t next_update() const {
    if (clk_val) {
      return m_last_update + up_time;
    } else {
      return m_last_update + down_time;
    }
  }
  duration_t get_period() const { return down_time + up_time; }
  duration_t last_update() const { return m_last_update; }
  void update(duration_t now) {
    clk_val = !clk_val;
    clock_fun(clk_val);
    m_last_update = now;
  }
  edge_e get_upcoming_edge() const {
    return clk_val ? edge_e::FALL_EDGE : edge_e::RISE_EDGE;
  }
  void exec_callbacks() {
    for (auto &cb : callback_functions) {
      if (cb.edge == edge_e::BOTH_EDGE || cb.edge != get_upcoming_edge()) {
        cb.fun(get_upcoming_edge());
      }
    }
  }
};

class sim_driver {
protected:
  using duration_t = ClockDriver::duration_t;
  std::unordered_map<std::string, std::string> cmd_line_args;
  std::vector<std::reference_wrapper<ClockDriver>> m_clocks;
  /**
   * \brief Parse the command line arguments of form key=value, stor as
   *unordered_map member variable cmd_line_args \returns the last arg as the
   *waveform file
   **/
  std::string parse_cmd_line_args(int argc, char **argv) {
    std::string waveform_file = "";
    for (int i = 1; i < argc; ++i) {
      std::string arg(argv[i]);
      if (arg[0] == '+') {
        // ignore arguments that start with +
        break;
      }
      auto eq_index = arg.find("=");
      if (eq_index != std::string::npos) {
        auto key = arg.substr(0, eq_index);
        auto value = arg.substr(eq_index, std::string::npos);
        cmd_line_args[key] = value;
      } else {
        waveform_file = arg;
      }
    }
    return waveform_file;
  }
  duration_t sim_timeout;
  /*set timout relative to NOW */
  void set_sim_timeout(duration_t timeout) {
    sim_timeout = get_now() + timeout;
  }
  sim_driver() { sim_timeout = std::chrono::milliseconds(10); }
  ~sim_driver() {}
  void add_clock(ClockDriver &cd) { m_clocks.push_back(cd); }
  virtual duration_t get_now() = 0;
  virtual duration_t update() = 0;

  duration_t run(duration_t run_time) {
    duration_t ran_time = update();
    while (ran_time < run_time) {
      ran_time += update();
    }
    return ran_time;
  }

  duration_t run_until_rising_edge(const ClockDriver &cd) {
    duration_t ran_time(0);
    do {
      ran_time += update();
    } while (!(get_now() == cd.last_update() &&
               cd.get_upcoming_edge() == ClockDriver::edge_e::FALL_EDGE));

    return ran_time;
  }
};

#endif // SIM_DRIVER_HPP
