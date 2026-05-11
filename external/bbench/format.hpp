
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../src/io/stdout.hpp"
#include "../../src/linux/io.hpp"
#include "../../src/linux/sys/fcntl.hpp"
#include "../../src/memory/cmemory.hpp"
#include "../../src/string/conversions/floating_point.hpp"
#include "../../src/string/conversions/integral.hpp"
#include "../../src/string/string.hpp"
#include "../../src/types.hpp"
#include "../../src/vector.hpp"

#include "events.hpp"
#include "funcs.hpp"

namespace bbench::format
{

struct sink {
  int fd = 1;
  bool owned = false;     // close in destructor

  sink() = default;

  explicit sink(int fd_, bool own = false) : fd(fd_), owned(own) {}

  sink(const sink &) = delete;

  sink(sink &&o) noexcept : fd(o.fd), owned(o.owned) { o.owned = false; }

  ~sink()
  {
    if ( owned && fd != -1 ) micron::close(fd);
  }

  static sink
  stdout_sink(void)
  {
    return sink(1, false);
  }

  static sink
  stderr_sink(void)
  {
    return sink(2, false);
  }

  static sink
  file_sink(const char *path)
  {
    int f = micron::open(path, micron::posix::o_wronly | micron::posix::o_create | micron::posix::o_trunc, 0644);
    return sink(f, true);
  }

  void
  emit(const char *s, usize n) const
  {
    if ( fd < 0 ) return;
    micron::posix::write(fd, s, n);
  }

  void
  emit(const char *s) const
  {
    emit(s, micron::strlen(s));
  }

  void
  emit_int(long long v) const
  {
    char buf[32];
    usize n = micron::format::__impl::fmt_int_to_buf(buf, sizeof(buf), v, 10, false);
    emit(buf, n);
  }

  void
  emit_double(double v) const
  {
    char buf[64];
    usize n = micron::__impl::__ryu::d2s_buffered(v, buf);
    emit(buf, n);
  }

  void
  newline(void) const
  {
    emit("\n", 1);
  }
};

inline void
__emit_row(const sink &s, const char *label, long long v, bool color)
{
  if ( color ) s.emit("\033[34m", 5);
  s.emit(label);
  if ( color ) s.emit("\033[0m", 4);
  s.emit_int(v);
  s.newline();
}

inline void
emit_human_one(const sink &out, const benchmark_t &b, u32 detail, bool color, u32 n_runs)
{
  if ( color ) out.emit("\033[34m", 5);
  out.emit("Attempted ");
  out.emit_int(static_cast<long long>(n_runs));
  out.emit(" runs");
  if ( color ) out.emit("\033[0m", 4);
  out.newline();

  if ( color ) out.emit("\033[34m", 5);
  out.emit("Results for: ");
  if ( color ) out.emit("\033[0m", 4);
  out.emit(b.name.c_str());
  out.newline();

  if ( color ) out.emit("\033[34m", 5);
  out.emit("Total time elapsed:   ");
  if ( color ) out.emit("\033[0m", 4);
  out.emit_double(b.time);
  out.emit(" microseconds");
  out.newline();

  __emit_row(out, "Cycles Spent:         ", b.cycles, color);
  __emit_row(out, "Total Instructions:   ", b.instructions, color);
  __emit_row(out, "Total Branches:       ", b.total_branches, color);
  __emit_row(out, "Branch Misses:        ", b.branch_misses, color);
  __emit_row(out, "Total CPU Cycles:     ", b.total_cycles, color);
  __emit_row(out, "Total CPU Time Spent: ", b.cpu_time, color);
  __emit_row(out, "Context Switches:     ", b.context_switches, color);
  __emit_row(out, "Core Migrations:      ", b.migrations, color);
  __emit_row(out, "Cache Misses:         ", b.cache_misses, color);
  __emit_row(out, "L1 Cache:             ", b.l1_cache, color);
  __emit_row(out, "L1T Cache:            ", b.l1t_cache, color);
  __emit_row(out, "Last Level Cache:     ", b.ll_cache, color);
  __emit_row(out, "Cache Accesses:       ", b.access, color);
  __emit_row(out, "Branch Predictions:   ", b.bpu, color);

  if ( detail >= 2 ) {
    __emit_row(out, "Page Faults:          ", b.page_faults, color);
    __emit_row(out, "Bus Cycles:           ", b.bus_cycles, color);
    __emit_row(out, "Stalled Frontend:     ", b.stalled_front, color);
    __emit_row(out, "Stalled Backend:      ", b.stalled_back, color);
    __emit_row(out, "dTLB Loads:           ", b.dtlb_access, color);
    __emit_row(out, "dTLB Load Misses:     ", b.dtlb_miss, color);
    __emit_row(out, "iTLB Loads:           ", b.itlb_access, color);
    __emit_row(out, "iTLB Load Misses:     ", b.itlb_miss, color);
    __emit_row(out, "L1d Load Misses:      ", b.l1d_miss, color);
    __emit_row(out, "LLC Load Misses:      ", b.llcache_miss, color);
  }
  if ( detail >= 3 ) {
    __emit_row(out, "L1i Load Misses:      ", b.l1t_miss, color);
    __emit_row(out, "L1d Prefetches:       ", b.l1d_prefetch, color);
    __emit_row(out, "L1d Prefetch Misses:  ", b.l1d_prefetch_miss, color);
    __emit_row(out, "Minor Faults:         ", b.minor_faults, color);
    __emit_row(out, "Major Faults:         ", b.major_faults, color);
    __emit_row(out, "Alignment Faults:     ", b.alignment_faults, color);
    __emit_row(out, "Emulation Faults:     ", b.emulation_faults, color);
  }
}

// CSV header row matching emit_csv_one()
inline void
emit_csv_header(const sink &out, u32 detail, char sep)
{
  const char s[2] = { sep, '\0' };
  out.emit("name");
  out.emit(s);
  out.emit("time_us");
  out.emit(s);
  out.emit("cycles");
  out.emit(s);
  out.emit("instructions");
  out.emit(s);
  out.emit("branches");
  out.emit(s);
  out.emit("branch_misses");
  out.emit(s);
  out.emit("total_cycles");
  out.emit(s);
  out.emit("cpu_time");
  out.emit(s);
  out.emit("context_switches");
  out.emit(s);
  out.emit("migrations");
  out.emit(s);
  out.emit("cache_misses");
  out.emit(s);
  out.emit("l1_cache");
  out.emit(s);
  out.emit("l1t_cache");
  out.emit(s);
  out.emit("ll_cache");
  out.emit(s);
  out.emit("cache_node");
  out.emit(s);
  out.emit("bpu");
  if ( detail >= 2 ) {
    out.emit(s);
    out.emit("page_faults");
    out.emit(s);
    out.emit("bus_cycles");
    out.emit(s);
    out.emit("stalled_front");
    out.emit(s);
    out.emit("stalled_back");
    out.emit(s);
    out.emit("dtlb_access");
    out.emit(s);
    out.emit("dtlb_miss");
    out.emit(s);
    out.emit("itlb_access");
    out.emit(s);
    out.emit("itlb_miss");
    out.emit(s);
    out.emit("l1d_miss");
    out.emit(s);
    out.emit("llcache_miss");
  }
  if ( detail >= 3 ) {
    out.emit(s);
    out.emit("l1t_miss");
    out.emit(s);
    out.emit("l1d_prefetch");
    out.emit(s);
    out.emit("l1d_prefetch_miss");
    out.emit(s);
    out.emit("minor_faults");
    out.emit(s);
    out.emit("major_faults");
    out.emit(s);
    out.emit("alignment_faults");
    out.emit(s);
    out.emit("emulation_faults");
  }
  out.newline();
}

inline void
emit_csv_one(const sink &out, const benchmark_t &b, u32 detail, char sep)
{
  const char s[2] = { sep, '\0' };
  out.emit(b.name.c_str());
  out.emit(s);
  out.emit_double(b.time);
  out.emit(s);
  out.emit_int(b.cycles);
  out.emit(s);
  out.emit_int(b.instructions);
  out.emit(s);
  out.emit_int(b.total_branches);
  out.emit(s);
  out.emit_int(b.branch_misses);
  out.emit(s);
  out.emit_int(b.total_cycles);
  out.emit(s);
  out.emit_int(b.cpu_time);
  out.emit(s);
  out.emit_int(b.context_switches);
  out.emit(s);
  out.emit_int(b.migrations);
  out.emit(s);
  out.emit_int(b.cache_misses);
  out.emit(s);
  out.emit_int(b.l1_cache);
  out.emit(s);
  out.emit_int(b.l1t_cache);
  out.emit(s);
  out.emit_int(b.ll_cache);
  out.emit(s);
  out.emit_int(b.access);
  out.emit(s);
  out.emit_int(b.bpu);
  if ( detail >= 2 ) {
    out.emit(s);
    out.emit_int(b.page_faults);
    out.emit(s);
    out.emit_int(b.bus_cycles);
    out.emit(s);
    out.emit_int(b.stalled_front);
    out.emit(s);
    out.emit_int(b.stalled_back);
    out.emit(s);
    out.emit_int(b.dtlb_access);
    out.emit(s);
    out.emit_int(b.dtlb_miss);
    out.emit(s);
    out.emit_int(b.itlb_access);
    out.emit(s);
    out.emit_int(b.itlb_miss);
    out.emit(s);
    out.emit_int(b.l1d_miss);
    out.emit(s);
    out.emit_int(b.llcache_miss);
  }
  if ( detail >= 3 ) {
    out.emit(s);
    out.emit_int(b.l1t_miss);
    out.emit(s);
    out.emit_int(b.l1d_prefetch);
    out.emit(s);
    out.emit_int(b.l1d_prefetch_miss);
    out.emit(s);
    out.emit_int(b.minor_faults);
    out.emit(s);
    out.emit_int(b.major_faults);
    out.emit(s);
    out.emit_int(b.alignment_faults);
    out.emit(s);
    out.emit_int(b.emulation_faults);
  }
  out.newline();
}

struct stats_t {
  double mean, stddev, mn, mx;
};

template <typename V, typename F>
inline stats_t
compute_stats(const V &runs, F field_accessor)
{
  stats_t s{ 0, 0, 0, 0 };
  if ( runs.size() == 0 ) return s;
  double sum = 0.0;
  double mn = static_cast<double>(field_accessor(runs[0]));
  double mx = mn;
  for ( const auto &r : runs ) {
    double v = static_cast<double>(field_accessor(r));
    sum += v;
    if ( v < mn ) mn = v;
    if ( v > mx ) mx = v;
  }
  s.mean = sum / static_cast<double>(runs.size());
  if ( runs.size() > 1 ) {
    double sq = 0.0;
    for ( const auto &r : runs ) {
      double v = static_cast<double>(field_accessor(r));
      double d = v - s.mean;
      sq += d * d;
    }
    s.stddev = sq / static_cast<double>(runs.size() - 1);
    double x = s.stddev > 0 ? s.stddev : 0;
    if ( x > 0 ) {
      double r = x;
      for ( int i = 0; i < 12; ++i ) r = 0.5 * (r + x / r);
      s.stddev = r;
    }
  }
  s.mn = mn;
  s.mx = mx;
  return s;
}

inline void
emit_table(const sink &out, const micron::vector<benchmark_t> &runs)
{
  if ( runs.size() == 0 ) return;
  auto s = compute_stats(runs, [](const benchmark_t &b) { return b.time; });
  out.emit("# Table of individual time measurements (us):\n");
  for ( const auto &r : runs ) {
    out.emit("  ");
    out.emit_double(r.time);
    out.emit("  (");
    double dev = r.time - s.mean;
    if ( dev >= 0 ) out.emit("+");
    out.emit_double(dev);
    out.emit(")\n");
  }
  out.emit("# Final: mean=");
  out.emit_double(s.mean);
  out.emit(" stddev=");
  out.emit_double(s.stddev);
  out.emit(" min=");
  out.emit_double(s.mn);
  out.emit(" max=");
  out.emit_double(s.mx);
  out.newline();
}

};     // namespace bbench::format
