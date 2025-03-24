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
#include "sim_driver.hpp"
#include <cstdint>
#include <filesystem>
#include <type_traits>
#include <xsi.h>

template <int WIDTH> class sim_port {
  // create unsigne type at least WIDTH bits wide
  using T = typename std::conditional<
      WIDTH <= 8, uint8_t,
      typename std::conditional<
          WIDTH <= 16, uint16_t,
          typename std::conditional<
              WIDTH <= 32, uint32_t,
              typename std::conditional<
                  WIDTH <= 64, uint64_t,
                  typename std::conditional<WIDTH <= 128, __uint128_t, void>::
                      type>::type>::type>::type>::type;
  xsiHandle xsi_handle;
  int port_num;
  std::string name;
  T mask_val;

public:
  sim_port(xsiHandle handle, std::string name) {
    this->xsi_handle = handle;
    this->name = name;
    this->port_num = xsi_get_port_number(handle, name.c_str());
    if (port_num < 0) {
      fprintf(stderr, "ERROR: simulation port '%s' not found\n", name.c_str());
      exit(1);
    }
    mask_val = (1UL << WIDTH) - 1;
  }
  void set_val(uint32_t aval, uint32_t bval) {
    s_xsi_vlog_logicval vlog_val = {aval, bval};
    xsi_put_value(xsi_handle, port_num, &vlog_val);
  }
  s_xsi_vlog_logicval get_val() {
    s_xsi_vlog_logicval vlog_val = {0, 0};
    xsi_get_value(xsi_handle, port_num, &vlog_val);
    return vlog_val;
  }

  const T &operator=(const T &setval) {
    set_val(setval & mask_val, 0);
    return setval;
  }
  operator T() {
    s_xsi_vlog_logicval v;
    v = get_val();
    return (T)(v.aVal & mask_val);
  }
};
#define sim_port_construct(name) name(handle, #name)

template <typename dut_t> class xsim_driver : protected sim_driver {
  xsiHandle xsi_handle;

protected:
  dut_t *dut;
  /*set timout relative to NOW */
  void set_sim_timeout(duration_t timeout) {
    sim_timeout = get_now() + timeout;
  }
  xsim_driver(int argc, char **argv) {
    std::string waveform_file = parse_cmd_line_args(argc, argv);
    s_xsi_setup_info info = {0};
    info.logFileName = NULL;

    // for some reason the xsim generated `xsi_open` must run from the 2
    // directories up from the xsimk.so for that reason we do some hackery here
    // to save the current directory, jump to that directory then got back to
    // the saved directory. the waveform file needs to be transformed to a
    // absolute path to make this work

    if (waveform_file != "") {
      waveform_file =
          std::filesystem::absolute(std::filesystem::path(waveform_file));
      info.wdbFileName = (char *)waveform_file.c_str();
    } else {
      info.wdbFileName = NULL;
    }

    auto saved_cwd = std::filesystem::current_path();
    std::filesystem::current_path(dut_t::get_sim_dir());
    xsi_handle = xsi_open(&info);
    std::filesystem::current_path(saved_cwd);

    if (waveform_file != "") {
      xsi_trace_all(xsi_handle);
    }
    dut = new dut_t(xsi_handle);

    sim_timeout = std::chrono::milliseconds(10);
  }
  ~xsim_driver() { delete dut; }

  virtual duration_t get_now() final {
    return duration_t(xsi_get_time(xsi_handle));
  }
  virtual duration_t update() final {
    duration_t start = get_now();
    duration_t min_update = duration_t::max();
    for (auto &cd : m_clocks) {
      auto x = cd.next_update();
      if (x < min_update)
        min_update = x;
    }
    xsi_run(xsi_handle, (min_update - start).count());

    duration_t now = get_now();

    for (auto &cd : m_clocks) {
      if (cd.next_update() == now) {
        cd.update(now);
      }
    }
    for (auto &cd : m_clocks) {
      if (cd.last_update() == now) {
        cd.exec_callbacks();
      }
    }

    if (sim_timeout != std::chrono::milliseconds(0) && now >= sim_timeout) {
      throw std::runtime_error("Simulation timed out\n");
    }

    return get_now() - start;
  }
};

#endif // XSIM_DRIVER_HPP
