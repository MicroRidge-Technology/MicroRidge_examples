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
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <thread>
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
  std::vector<ClockDriver> m_clocks;
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

private:
  // Packed barrier state: low 32 bits = arrived count, high 32 bits =
  // n_active. Combined into one atomic so main can wait on both halves at
  // once — any change to either half wakes the wait. This eliminates the
  // need for a "phantom arrival" trick and ensures main always waits for
  // every alive child to actually arrive each round.
  std::atomic<uint64_t> bar{0};
  std::atomic<size_t> generation{0};
  std::atomic<bool> stopping{false};
  std::thread::id main_tid;
  std::vector<std::thread> threads;
  duration_t m_last_update_duration{0};

  static constexpr uint64_t ARRIVED_MASK = 0xFFFFFFFFULL;
  static constexpr uint64_t N_ACTIVE_INC = 1ULL << 32;
  static uint32_t arrived_of(uint64_t s) { return uint32_t(s); }
  static uint32_t n_active_of(uint64_t s) { return uint32_t(s >> 32); }

  void update_no_threads() { m_last_update_duration = _update(); }

  void update_with_threads() {
    if (std::this_thread::get_id() == main_tid) {
      while (true) {
        uint64_t s = bar.load(std::memory_order_acquire);
        if (arrived_of(s) >= n_active_of(s))
          break;
        bar.wait(s, std::memory_order_acquire);
      }
      m_last_update_duration = _update();
      // Clear the arrived (low) half while preserving n_active (high half).
      // CAS loop because wrappers may concurrently decrement n_active.
      uint64_t old = bar.load(std::memory_order_relaxed);
      while (!bar.compare_exchange_weak(old, old & ~ARRIVED_MASK,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
      }
      generation.fetch_add(1, std::memory_order_release);
      generation.notify_all();
    } else {
      if (stopping.load(std::memory_order_acquire))
        throw thread_stop{};
      auto my_gen = generation.load(std::memory_order_acquire);
      bar.fetch_add(1, std::memory_order_acq_rel);
      bar.notify_one();
      while (true) {
        size_t g = generation.load(std::memory_order_acquire);
        if (g != my_gen)
          break;
        if (stopping.load(std::memory_order_acquire))
          throw thread_stop{};
        generation.wait(my_gen, std::memory_order_acquire);
      }
      if (stopping.load(std::memory_order_acquire))
        throw thread_stop{};
    }
  }

public:
  /// Sentinel exception thrown out of update() (and helpers like run() and
  /// run_until_rising_edge()) when the driver is shutting down. add_thread()
  /// installs a wrapper that catches it; child code may also catch it
  /// explicitly if cleanup is needed before unwinding.
  struct thread_stop {};

protected:
  /// Per-step hook. Defaults to a direct _update() call (no synchronization
  /// overhead). Swapped to a barrier-rendezvous version on the first
  /// add_thread() call. Throws thread_stop from a child thread on shutdown.
  std::function<void()> update;

  sim_driver() : main_tid(std::this_thread::get_id()) {
    sim_timeout = std::chrono::milliseconds(10);
    update = [this] { update_no_threads(); };
  }
  ~sim_driver() { shutdown(); }

  /// Stop and join all child threads. Idempotent. Must be called as the
  /// first line of every derived destructor (before deleting the DUT) so
  /// child threads can't touch a freed DUT.
  void shutdown() noexcept {
    if (stopping.exchange(true, std::memory_order_acq_rel))
      return;
    // Bump generation so any child currently inside generation.wait() sees a
    // value change and returns (atomic::wait would otherwise block forever
    // because notify_all races with wait — notify is not queued for late
    // arrivals).
    generation.fetch_add(1, std::memory_order_release);
    generation.notify_all();
    bar.notify_all();
    for (auto &t : threads) {
      if (t.joinable())
        t.join();
    }
    threads.clear();
  }

  template <typename pin_t>
  ClockDriver &add_clock(pin_t &clock_pin,
                         std::chrono::duration<long double, std::nano> period) {
    ClockDriver cd([&](uint8_t clk_val) { clock_pin = clk_val; }, period);
    m_clocks.push_back(cd);
    return m_clocks.back();
  }

  /// Spawn a child thread that participates in the per-step barrier. The
  /// first call swaps update() over to the barrier-rendezvous version. All
  /// add_thread calls must precede the first update() call. The thread
  /// function is wrapped to catch thread_stop, so child loops can be written
  /// as bare infinite loops and unwind cleanly at shutdown. A thread may also
  /// return early on its own — n_active is decremented and main is notified
  /// so it doesn't deadlock waiting for an arrival that will never come.
  void add_thread(std::function<void()> fn) {
    uint64_t old = bar.fetch_add(N_ACTIVE_INC, std::memory_order_acq_rel);
    if (n_active_of(old) == 0) {
      update = [this] { update_with_threads(); };
    }
    threads.emplace_back([this, fn = std::move(fn)] {
      try {
        fn();
      } catch (thread_stop &) {
      }
      bar.fetch_sub(N_ACTIVE_INC, std::memory_order_acq_rel);
      bar.notify_one();
    });
  }

  duration_t last_update_duration() const { return m_last_update_duration; }

  /// True until shutdown() is called. Use from child-thread loops to break
  /// out cleanly when the driver is being destroyed.
  bool is_running() const { return !stopping.load(std::memory_order_acquire); }

  virtual duration_t get_now() = 0;
  virtual duration_t _update() = 0;

  duration_t run(duration_t run_time) {
    duration_t total(0);
    while (total < run_time) {
      update();
      total += last_update_duration();
    }
    return total;
  }

  template <typename pin_t> duration_t run_until_rising_edge(pin_t &clock_pin) {
    duration_t total(0);
    while (clock_pin != 0) {
      update();
      total += last_update_duration();
    }
    while (clock_pin != 1) {
      update();
      total += last_update_duration();
    }
    return total;
  }
};

#endif // SIM_DRIVER_HPP
