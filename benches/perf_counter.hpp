//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Linux perf_event_open helper for benchmark harnesses.
//
// Opens a 2-event group (CPU_CYCLES + INSTRUCTIONS) so a single read() call
// returns both counters atomically. With exclude_kernel + exclude_hv set,
// the counters work at perf_event_paranoid >= 2 (the typical Fedora/Debian
// default — no CAP_PERFMON required).
//
// Build-time gating: full perf support is amd64-only here. On other arches
// the class compiles but `open()` always returns false (caller should fall
// back to a cycle-counter-only path). This keeps the bench compiling
// cross-arch via `duck build benches/cmemory_bench.cpp --arm`, even though
// running benches under qemu yields mostly meaningless cycle counts.

#pragma once

#include "../src/syscall.hpp"
#include "../src/types.hpp"

namespace bench
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Linux perf_event_open ABI — minimal struct laid out per
// PERF_ATTR_SIZE_VER0 (64 bytes). The kernel zero-fills any newer
// fields it knows about; the benchmark needs none of them.

enum : u32 {
  PERF_TYPE_HARDWARE = 0,
};

enum : u64 {
  PERF_COUNT_HW_CPU_CYCLES = 0,
  PERF_COUNT_HW_INSTRUCTIONS = 1,
};

enum : u64 {
  PERF_FORMAT_TOTAL_TIME_ENABLED = 1ULL << 0,
  PERF_FORMAT_TOTAL_TIME_RUNNING = 1ULL << 1,
  PERF_FORMAT_GROUP = 1ULL << 3,
};

// flags bit positions inside attr.flags (the bitfield layout in
// linux/perf_event.h):
enum : u64 {
  ATTR_FLAG_DISABLED = 1ULL << 0,
  ATTR_FLAG_EXCLUDE_USER = 1ULL << 4,
  ATTR_FLAG_EXCLUDE_KERNEL = 1ULL << 5,
  ATTR_FLAG_EXCLUDE_HV = 1ULL << 6,
};

struct perf_event_attr_v0 {
  u32 type;
  u32 size;
  u64 config;
  u64 sample_period_or_freq;
  u64 sample_type;
  u64 read_format;
  u64 flags;     // disabled / exclude_* bitfield
  u32 wakeup_events;
  u32 bp_type;
  u64 config1;
};

static_assert(sizeof(perf_event_attr_v0) == 64, "PERF_ATTR_SIZE_VER0 must be 64 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ioctl numbers for perf_event fds. _IO('$', N) = (0x24 << 8) | N.

enum : u64 {
  PERF_EVENT_IOC_ENABLE = 0x2400,
  PERF_EVENT_IOC_DISABLE = 0x2401,
  PERF_EVENT_IOC_RESET = 0x2403,
  PERF_IOC_FLAG_GROUP = 1,
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// PERF_FORMAT_GROUP + TIME_ENABLED + TIME_RUNNING read layout. The two
// time fields appear after `nr` and before the per-event values; we
// use them to scale counters that were multiplexed by the kernel
// (cycles_actual = cycles_raw * time_enabled / time_running).

struct perf_group_read {
  u64 nr;
  u64 time_enabled;
  u64 time_running;
  u64 cycles;
  u64 instructions;
};

class perf_counter
{
  int leader_fd = -1;     // CPU_CYCLES
  int follow_fd = -1;     // INSTRUCTIONS

#if defined(__micron_arch_amd64)
  static int
  __perf_event_open(perf_event_attr_v0 *attr, i32 pid, i32 cpu, i32 group_fd, u64 flags) noexcept
  {
    return static_cast<int>(micron::syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags));
  }
#endif

public:
  perf_counter() noexcept = default;

  ~perf_counter() noexcept { close(); }

  perf_counter(const perf_counter &) = delete;
  perf_counter &operator=(const perf_counter &) = delete;

  // Returns true on success. False means HW counters are unavailable
  // (perf_event_paranoid too high, kernel.perf_event_disabled, no PMU,
  // running in a container with the syscall blocked, etc., or building
  // for a non-amd64 arch where this header is gated off). Caller should
  // fall back to a rdtsc-only path.
  bool
  open() noexcept
  {
#if defined(__micron_arch_amd64)
    perf_event_attr_v0 cyc{};
    cyc.type = PERF_TYPE_HARDWARE;
    cyc.size = sizeof(cyc);
    cyc.config = PERF_COUNT_HW_CPU_CYCLES;
    cyc.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    cyc.flags = ATTR_FLAG_DISABLED | ATTR_FLAG_EXCLUDE_KERNEL | ATTR_FLAG_EXCLUDE_HV;
    leader_fd = __perf_event_open(&cyc, 0 /* this thread */, -1 /* any cpu */, -1 /* group leader */, 0);
    if ( leader_fd < 0 ) return false;

    perf_event_attr_v0 inst{};
    inst.type = PERF_TYPE_HARDWARE;
    inst.size = sizeof(inst);
    inst.config = PERF_COUNT_HW_INSTRUCTIONS;
    inst.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    inst.flags = ATTR_FLAG_DISABLED | ATTR_FLAG_EXCLUDE_KERNEL | ATTR_FLAG_EXCLUDE_HV;
    follow_fd = __perf_event_open(&inst, 0, -1, leader_fd, 0);
    if ( follow_fd < 0 ) {
      close();
      return false;
    }

    return true;
#else
    return false;
#endif
  }

  void
  close() noexcept
  {
#if defined(__micron_arch_amd64)
    if ( follow_fd >= 0 ) {
      micron::syscall(SYS_close, follow_fd);
      follow_fd = -1;
    }
    if ( leader_fd >= 0 ) {
      micron::syscall(SYS_close, leader_fd);
      leader_fd = -1;
    }
#endif
  }

  bool
  ok() const noexcept
  {
    return leader_fd >= 0 && follow_fd >= 0;
  }

  void
  reset() noexcept
  {
#if defined(__micron_arch_amd64)
    micron::syscall(SYS_ioctl, leader_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
#endif
  }

  void
  enable() noexcept
  {
#if defined(__micron_arch_amd64)
    micron::syscall(SYS_ioctl, leader_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
#endif
  }

  void
  disable() noexcept
  {
#if defined(__micron_arch_amd64)
    micron::syscall(SYS_ioctl, leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
#endif
  }

  // Reads the group counters. Returns { cycles, instructions }. Open
  // order is leader (cycles) first.
  struct readout {
    u64 cycles;
    u64 instructions;
  };

  readout
  read() noexcept
  {
#if defined(__micron_arch_amd64)
    perf_group_read buf{};
    long n = micron::syscall(SYS_read, leader_fd, &buf, sizeof(buf));
    if ( n < static_cast<long>(sizeof(buf)) ) return { 0, 0 };
    // Scale up if the kernel multiplexed the counters across our group.
    // time_running == 0 means the events never ran; treat as unavailable.
    if ( buf.time_running == 0 ) return { 0, 0 };
    if ( buf.time_running == buf.time_enabled ) {
      return { buf.cycles, buf.instructions };
    }
    // Approximate scaling: out = raw * (time_enabled / time_running).
    // Use 128-bit-style multiply to avoid overflow on long-running benches.
    auto scale = [](u64 v, u64 enabled, u64 running) -> u64 {
      __uint128_t big = static_cast<__uint128_t>(v) * static_cast<__uint128_t>(enabled);
      return static_cast<u64>(big / running);
    };
    return { scale(buf.cycles, buf.time_enabled, buf.time_running),
             scale(buf.instructions, buf.time_enabled, buf.time_running) };
#else
    return { 0, 0 };
#endif
  }
};

};     // namespace bench
