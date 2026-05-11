
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <linux/perf_event.h>

#include "../../src/types.hpp"
#include "../../src/vector.hpp"
#include "../../src/memory/cmemory.hpp"
#include "../../src/linux/sys/ioctl.hpp"

#include "perf.hpp"   

namespace bbench {

// used by the -e CLI flag
struct event_def {
  const char *name;
  u32 type;
  u64 config;
};

// PERF_TYPE_HW_CACHE config layout: cache_id | (op<<8) | (result<<16)
inline constexpr u64
__cache_cfg(u64 id, u64 op = PERF_COUNT_HW_CACHE_OP_READ,
            u64 result = PERF_COUNT_HW_CACHE_RESULT_ACCESS) {
  return id | (op << 8) | (result << 16);
}

inline constexpr event_def known_events[] = {
  // hardware
  { "cycles",                  PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES },
  { "cpu-cycles",              PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES },
  { "instructions",            PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS },
  { "branches",                PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
  { "branch-instructions",     PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
  { "branch-misses",           PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES },
  { "cache-references",        PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES },
  { "cache-misses",            PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES },
  { "bus-cycles",              PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES },
  { "stalled-cycles-frontend", PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND },
  { "stalled-cycles-backend",  PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND },
  { "ref-cycles",              PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES },

  // software
  { "cpu-clock",               PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK },
  { "task-clock",              PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK },
  { "page-faults",             PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS },
  { "faults",                  PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS },
  { "context-switches",        PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES },
  { "cs",                      PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES },
  { "cpu-migrations",          PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS },
  { "migrations",              PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS },
  { "minor-faults",            PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN },
  { "major-faults",            PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ },
  { "alignment-faults",        PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS },
  { "emulation-faults",        PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS },
  { "dummy",                   PERF_TYPE_SOFTWARE, PERF_COUNT_SW_DUMMY },

  // hw cache (the perf list defaults: op=READ, result=ACCESS or MISS)
  { "L1-dcache-loads",         PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1D) },
  { "L1-dcache-load-misses",   PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "L1-dcache-stores",        PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_WRITE) },
  { "L1-dcache-prefetches",    PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_PREFETCH) },
  { "L1-dcache-prefetch-misses", PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_PREFETCH, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "L1-icache-loads",         PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1I) },
  { "L1-icache-load-misses",   PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_L1I, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "LLC-loads",               PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_LL) },
  { "LLC-load-misses",         PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "LLC-stores",              PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_WRITE) },
  { "LLC-store-misses",        PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "dTLB-loads",              PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_DTLB) },
  { "dTLB-load-misses",        PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_DTLB, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "iTLB-loads",              PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_ITLB) },
  { "iTLB-load-misses",        PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_ITLB, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "branch-loads",            PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_BPU) },
  { "branch-load-misses",      PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_BPU, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
  { "node-loads",              PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_NODE) },
  { "node-load-misses",        PERF_TYPE_HW_CACHE, __cache_cfg(PERF_COUNT_HW_CACHE_NODE, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS) },
};

// case sensitive!
inline const event_def *
lookup_event(const char *name) {
  if (!name) return nullptr;
  for (const auto &e : known_events) {
    if (micron::strcmp(e.name, name) == 0) return &e;
  }
  return nullptr;
}

struct dynamic_event {
  int e_fd = -1;
  int last_errno = 0;
  perf_event_attr attr{};
  const char *pretty_name = nullptr;
  bool valid = false;     // open() succeeded

  dynamic_event() = default;
  dynamic_event(const event_def &def, bool excl_kernel = true) { configure(def, excl_kernel); }
  dynamic_event(const dynamic_event &) = delete;
  dynamic_event(dynamic_event &&o) noexcept
      : e_fd(o.e_fd), last_errno(o.last_errno), attr(o.attr),
        pretty_name(o.pretty_name), valid(o.valid) {
    o.e_fd = -1;
    o.valid = false;
  }
  ~dynamic_event() {
    if (e_fd != -1) micron::close(e_fd);
  }

  void configure(const event_def &def, bool excl_kernel = true) {
    micron::memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.type = def.type;
    attr.config = def.config;
    attr.disabled = 1;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    attr.exclude_kernel = excl_kernel ? 1 : 0;
    attr.exclude_hv = 1;
    pretty_name = def.name;
  }

  void set_inherit(bool on) noexcept { attr.inherit = on ? 1 : 0; }
  void set_pinned(bool on) noexcept { attr.pinned = on ? 1 : 0; }
  void set_enable_on_exec(bool on) noexcept { attr.enable_on_exec = on ? 1 : 0; }

  int open(void) {
    e_fd = static_cast<int>(perf_event_this(attr));
    if (e_fd == -1) { last_errno = errno; valid = false; return -1; }
    last_errno = 0;
    valid = true;
    return e_fd;
  }
  int open(pid_t pid) {
    e_fd = static_cast<int>(perf_event_pid(attr, pid));
    if (e_fd == -1) { last_errno = errno; valid = false; return -1; }
    last_errno = 0;
    valid = true;
    return e_fd;
  }
  int reopen(pid_t pid) {
    if (e_fd != -1) micron::close(e_fd);
    return open(pid);
  }

  inline __attribute__((always_inline)) void begin(void) {
    if (!valid) return;
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_RESET, 0);
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_ENABLE, 0);
  }
  inline __attribute__((always_inline)) void end(void) {
    if (!valid) return;
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_DISABLE, 0);
  }

  // Scaled value (perf-stat semantics): value * time_enabled / time_running.
  long long retrieve(void) {
    if (!valid) return 0;
    struct { u64 value, te, tr; } r{ 0, 0, 0 };
    micron::posix::read(e_fd, &r, sizeof(r));
    if (r.tr == 0) return 0;
    if (r.tr == r.te) return static_cast<long long>(r.value);
    const double scale = static_cast<double>(r.te) / static_cast<double>(r.tr);
    return static_cast<long long>(static_cast<double>(r.value) * scale);
  }
};

struct dynamic_event_group {
  dynamic_event *events = nullptr;
  usize count = 0;

  dynamic_event_group() = default;
  dynamic_event_group(const micron::vector<event_def> &defs, bool excl_kernel = true) {
    count = defs.size();
    if (count == 0) return;
    events = new dynamic_event[count];
    for (usize i = 0; i < count; ++i) events[i].configure(defs[i], excl_kernel);
  }
  dynamic_event_group(const dynamic_event_group &) = delete;
  dynamic_event_group(dynamic_event_group &&o) noexcept
      : events(o.events), count(o.count) { o.events = nullptr; o.count = 0; }
  ~dynamic_event_group() { delete[] events; }

  void set_inherit(bool on) { for (usize i = 0; i < count; ++i) events[i].set_inherit(on); }
  void set_pinned(bool on)  { for (usize i = 0; i < count; ++i) events[i].set_pinned(on); }
  void set_enable_on_exec(bool on) { for (usize i = 0; i < count; ++i) events[i].set_enable_on_exec(on); }

  int open(void) {
    int bad = 0;
    for (usize i = 0; i < count; ++i) if (events[i].open() == -1) bad++;
    return bad == 0 ? 0 : -bad;
  }
  int reopen(pid_t pid) {
    int bad = 0;
    for (usize i = 0; i < count; ++i) if (events[i].reopen(pid) == -1) bad++;
    return bad == 0 ? 0 : -bad;
  }
  void begin(void) { for (usize i = 0; i < count; ++i) events[i].begin(); }
  void end(void)   { for (usize i = 0; i < count; ++i) events[i].end(); }

  template <typename F>
  void for_each(F &&fn) {
    for (usize i = 0; i < count; ++i)
      fn(events[i].pretty_name, events[i].retrieve(), events[i].last_errno);
  }
};

inline bool
parse_event_list(const char *csv, micron::vector<event_def> &out) {
  if (!csv) return true;
  bool all_ok = true;
  char buf[64];
  usize bi = 0;
  for (const char *p = csv;; ++p) {
    if (*p == ',' || *p == '\0') {
      if (bi > 0) {
        buf[bi] = '\0';
        const event_def *e = lookup_event(buf);
        if (e) out.push_back(*e);
        else   all_ok = false;
        bi = 0;
      }
      if (*p == '\0') break;
    } else if (bi < sizeof(buf) - 1) {
      buf[bi++] = *p;
    }
  }
  return all_ok;
}

}; // namespace bbench
