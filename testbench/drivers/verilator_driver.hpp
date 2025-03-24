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

#ifndef VERILATOR_DRIVER_HPP
#define VERILATOR_DRIVER_HPP
#include "sim_driver.hpp"
#include "verilated.h"
#include <verilated_fst_c.h>
template <typename dut_t> class verilator_driver : protected sim_driver {
  using duration_t = ClockDriver::duration_t;

  VerilatedContext *m_context;
  VerilatedFstC *m_trace;
  duration_t sim_timeout;

protected:
  dut_t *dut;
  /*set timout relative to NOW */
  void set_sim_timeout(duration_t timeout) {
    sim_timeout = get_now() + timeout;
  }
  verilator_driver(int argc, char **argv) {
    std::string waveform_file = parse_cmd_line_args(argc, argv);

    m_context = new VerilatedContext;
    m_context->commandArgs(argc, argv);
    if (waveform_file != "") {
      m_context->traceEverOn(true);
    }

    dut = new dut_t(m_context);

    m_context->timeunit(12);
    m_context->timeprecision(12);

    if (waveform_file != "") {
      m_trace = new VerilatedFstC;
      dut->trace(m_trace, 99);
      m_trace->open(waveform_file.c_str());
    } else {
      m_trace = NULL;
    }
    sim_timeout = std::chrono::milliseconds(10);
  }
  ~verilator_driver() {
    dut->final();
    delete m_trace;
    delete dut;
    delete m_context;
  }
  duration_t get_now() { return duration_t(m_context->time()); }
  duration_t update() {
    dut->eval();
    if (m_trace) {
      m_trace->dump(m_context->time());
    }
    duration_t start = get_now();
    duration_t min_update = duration_t::max();
    if (dut->eventsPending()) {
      min_update = duration_t(dut->nextTimeSlot());
    }
    for (auto &cd : m_clocks) {
      auto x = cd.next_update();
      if (x < min_update)
        min_update = x;
    }
    m_context->time(min_update.count());

    duration_t now = get_now();

    for (auto &cd : m_clocks) {
      if (cd.next_update() == now) {
        cd.update(now);
      }
    }
    dut->eval();
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
  duration_t run(duration_t run_time) {
    duration_t ran_time = update();
    while (ran_time < run_time) {
      ran_time += update();
    }
    return ran_time;
  }
};

#endif // VERILATOR_DRIVER_HPP
