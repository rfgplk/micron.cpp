//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Benchmark for micron::sec's two real data paths.
//
// Most of this module is one-shot syscalls -- installing a filter, creating a ruleset, unsharing a
// namespace. Timing those measures the KERNEL, not micron, so they are deliberately absent here.
// What is left is the work micron actually does per call:
//
//   [encode]   filter_builder construction. A realistic policy is several hundred instructions,
//              each emitted through a bounds-checked push(), and every argument predicate expands
//              into a 6-to-8 instruction 64-bit comparison ladder. This is the only part of a
//              seccomp policy whose cost is ours.
//   [faces]    the same policy built three ways -- imperative chain, type-level policy, functional
//              pipeline. They must cost the SAME: the pipeline returns the builder by reference
//              precisely so a chain does not memcpy an 8 KB object once per stage, and this cell
//              is what would show that regressing.
//   [context]  SELinux context parse and field extraction, which runs once per labelled file and
//              so is the hot path of any relabelling sweep.
//
// Build (every flag matters -- duck defaults to -fstack-protector-all, a canary on every function
// that does not cancel out of a ratio):
//   duck build benches/sec_bench.cpp --perf --fp --no-ssp --no-lto -i . -o bin/bench
//   taskset -c 2 ./bin/bench/sec_bench

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/sec.hpp"
#include "../src/std.hpp"

namespace
{

using ev = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace sl = micron::sec::selinux;
namespace s = micron::sec;

constexpr usize cap_n = 1024;
constexpr u16 deny_e = static_cast<u16>(micron::error::permissions);

using type_policy = s::seccomp_policy_n<cap_n, s::allow<g::baseline, g::io>, s::deny<g::network>>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct cell {
  const char *name;
  u64 ops;
  f64 cyc_per_op;
  f64 inst_per_op;
  f64 ipc;
};

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

template<typename Kernel>
[[gnu::noinline]] cell
measure(const char *name, u64 ops_per_rep, u64 reps, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS * reps; ++i ) kernel();

  f64 cpo[K_MEASUREMENTS];
  f64 ipo[K_MEASUREMENTS];
  f64 ipc[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    ev evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < reps; ++i ) kernel();
    evs.end();

    const auto cyc = static_cast<f64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<f64>(evs.get<bbench::hardware_instructions>().retrieve());
    const f64 total = static_cast<f64>(reps) * static_cast<f64>(ops_per_rep);
    cpo[m] = cyc / total;
    ipo[m] = ins / total;
    ipc[m] = cyc > 0 ? ins / cyc : 0.0;
  }
  return cell{ name, ops_per_rep, median_f64(cpo, K_MEASUREMENTS), median_f64(ipo, K_MEASUREMENTS),
               median_f64(ipc, K_MEASUREMENTS) };
}

void
print_cell(const cell &c)
{
  // one console() call: console() terminates each call with a newline, so padding has to be part
  // of the same string rather than a run of separate writes
  char pad[40];
  usize n = 0;
  while ( c.name[n] && n < 34 ) {
    pad[n] = c.name[n];
    ++n;
  }
  while ( n < 34 ) pad[n++] = ' ';
  pad[n] = '\0';
  micron::console(pad, " ops=", c.ops, "  cyc/op=", c.cyc_per_op, "  inst/op=", c.inst_per_op, "  IPC=", c.ipc);
}

// a realistic policy, built three ways.
//
// WARNING: every one of these is constexpr, and the type-level and functional forms take no
// runtime input at all -- so at -O3 the compiler evaluates them AT COMPILE TIME and the cell
// measures a constant load. Measured that way they read 3.25 inst/op against the imperative
// chain's 318, which says nothing about the code and everything about the fold. Sourcing the
// default action from a volatile is what forces all three to be built for real, so the three
// numbers are comparable.
volatile usize sink_count = 0;
volatile u32 sink_action = 0;
volatile u16 runtime_errno = static_cast<u16>(micron::error::permissions);
volatile i32 runtime_base = 0;

void
build_imperative(void) noexcept
{
  sc::filter_builder<cap_n> fb;
  fb.require_native_arch();
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::io::count; ++i ) fb.allow(g::io::calls[i]);
  for ( usize i = 0; i < g::network::count; ++i ) fb.deny_errno(g::network::calls[i], deny_e);
  fb.__seal(sc::act_errno(static_cast<u16>(runtime_errno)));
  sink_count = fb.count;
}

void
build_type_level(void) noexcept
{
  auto fb = type_policy::build(sc::act_errno(static_cast<u16>(runtime_errno)));
  sink_count = fb.count;
}

void
build_functional(void) noexcept
{
  auto fb = sc::policy<cap_n>() | sc::arch_native() | sc::allow_group<g::baseline, g::io>()
            | sc::deny_group<g::network>(static_cast<u16>(runtime_errno))
            | sc::deny_all(static_cast<u16>(runtime_errno)) | sc::build();
  sink_count = fb.count;
}

void
build_predicates(void) noexcept
{
  // the expensive shape: every rule is a 64-bit argument comparison, i.e. a full emit_pred ladder
  sc::filter_builder<512> fb;
  fb.require_native_arch();
  const i32 base = runtime_base;
  for ( i32 i = 0; i < 32; ++i ) {
    fb.action_if(base + i, sc::arg_lt(0, 0x1'0000'0000ull + static_cast<u64>(i)),
                 sc::act_errno(static_cast<u16>(runtime_errno)));
  }
  fb.__seal(sc::act_allow());
  sink_count = fb.count;
}

const char *ctx_strings[] = {
  "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023",
  "system_u:object_r:bin_t:s0",
  "system_u:system_r:init_t:s0",
  "system_u:object_r:etc_t:s0",
  "staff_u:staff_r:staff_t:s0-s0:c0.c1023",
};
constexpr usize ctx_n = sizeof(ctx_strings) / sizeof(ctx_strings[0]);

void
parse_contexts(void) noexcept
{
  usize acc = 0;
  for ( usize i = 0; i < ctx_n; ++i ) {
    const sl::context c = sl::context::parse(ctx_strings[i + static_cast<usize>(runtime_base)]);
    acc += c.size() + c.type().size + c.range().size;
  }
  sink_count = acc;
}

};      // namespace

int
main(void)
{
  micron::console("=== micron::sec bench ===\n");
  micron::console("landlock abi=", static_cast<i64>(s::landlock::abi_level()), "  selinux present=",
                  static_cast<u64>(sl::present()), "\n\n");

  micron::console("[encode] one policy of ", static_cast<u64>(type_policy::build(0).count), " instructions\n");
  print_cell(measure("build: imperative chain", 1, 20000, build_imperative));
  print_cell(measure("build: type-level policy", 1, 20000, build_type_level));
  print_cell(measure("build: functional pipeline", 1, 20000, build_functional));

  micron::console("\n[encode] 32 argument-predicate rules (emit_pred ladders)\n");
  print_cell(measure("build: 32x arg_lt predicates", 32, 20000, build_predicates));

  micron::console("\n[context] selinux parse + field extraction\n");
  print_cell(measure("context::parse x5", ctx_n, 200000, parse_contexts));

  micron::console("\n(", sink_count, " ", sink_action, ")\n");
  return 0;
}
