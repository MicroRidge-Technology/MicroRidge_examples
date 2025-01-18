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

#ifndef XSIM_DRIVER_HPP
#define XSIM_DRIVER_HPP
#include <chrono>
#include <cstdint>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include <functional>
#include <xsi.h>

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

    duration_t dur_period(int(period.count()) * 1000);
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

template <typename T,int WIDTH=8*sizeof(T)> class sim_port {
    xsiHandle xsi_handle;
    int port_num;
    std::string name;
    T mask_val;
public:
    sim_port(xsiHandle handle, std::string name) {
        this->xsi_handle = handle;
        this->name = name;
        this->port_num = xsi_get_port_number(handle,name.c_str());
        if (port_num < 0) {
            fprintf(stderr, "ERROR: simulation port '%s' not found\n",name.c_str());
            exit(1);
        }
        mask_val = (1UL<<WIDTH) -1;
    }
    void set_val(uint32_t aval, uint32_t bval) {
        s_xsi_vlog_logicval vlog_val = {aval, bval};
        xsi_put_value(xsi_handle,port_num, &vlog_val);
    }
    s_xsi_vlog_logicval get_val() {
        s_xsi_vlog_logicval vlog_val= {0,0};
        xsi_get_value(xsi_handle,port_num, &vlog_val);
        return vlog_val;
    }

    const T &operator=(const T &setval) {
        set_val(setval&mask_val, 0);
        return setval;
    }
    operator T() {
        s_xsi_vlog_logicval v;
        v = get_val();
        return (T)(v.aVal & mask_val);
    }
};
#define sim_port_construct(name) name(handle,#name)

template <typename dut_t> class xsim_driver {
  using duration_t = ClockDriver::duration_t;
  std::vector<std::reference_wrapper<ClockDriver>> m_clocks;

    duration_t sim_timeout;
    xsiHandle xsi_handle;
protected:
  dut_t *dut;
  /*set timout relative to NOW */
  void set_sim_timeout(duration_t timeout) {
    sim_timeout = get_now() + timeout;
  }
  xsim_driver(int argc, char **argv) {
    char *waveform_file = NULL;
    s_xsi_setup_info info = {0};
    info.logFileName = NULL;
    char wdbName[] ="test.wdb";
    info.wdbFileName = wdbName;
    xsi_handle = xsi_open(&info);
    xsi_trace_all(xsi_handle);
    int x = xsi_get_port_number(xsi_handle,"din_a");
    //for (int a = 1; a < argc; a++) {
    //  char *idx = strstr(argv[a], "+xsim");
    //  if (idx == NULL) {
    //    // if +xsim not in arg, grab last arg and use it as waveform file
    //    waveform_file = argv[a];
    //  }
    //}

    dut = new dut_t(xsi_handle);

    sim_timeout = std::chrono::milliseconds(10);
  }
  ~xsim_driver() {
    delete dut;
  }
  void add_clock(ClockDriver &cd) { m_clocks.push_back(cd); }
  duration_t get_now() { return duration_t( xsi_get_time(xsi_handle)); }
  duration_t update() {
    duration_t start = get_now();
    duration_t min_update = duration_t::max();
    for (auto &cd : m_clocks) {
      auto &c = cd.get();
      auto x = c.next_update();
      if (x < min_update)
        min_update = x;
    }
    xsi_run(xsi_handle,(min_update-start).count());

    duration_t now = get_now();

    for (auto &cd : m_clocks) {
      auto &c = cd.get();
      if (c.next_update() == now) {
        c.update(now);
      }
    }
    for (auto &cd : m_clocks) {
      auto &c = cd.get();
      if (c.last_update() == now) {
        c.exec_callbacks();
      }
    }

    if (sim_timeout != std::chrono::milliseconds(0) && now >= sim_timeout) {
      throw std::runtime_error("Simulation timed out\n");
    }

    return get_now() - start;
  }

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

#endif // XSIM_DRIVER_HPP
