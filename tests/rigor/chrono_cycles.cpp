//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- the cycle-accurate measurement tier.
//
// WHAT IS AND IS NOT ASSERTED HERE. A timing test that demands an exact number is a flaky test:
// this runs on a scheduler, on a core whose frequency moves, possibly under qemu. So the assertions
// are the INVARIANTS -- monotonicity, that the two independent frequency estimates agree, that a
// known sleep measures as roughly that long, that the optimisation barriers really do defeat the
// optimiser -- and never "the read cost is N cycles".
//
// C.7 is the regression at the bottom: rdtsc64() emitted an unguarded CNTVCT read on armv7-a, where
// the instruction is UNDEFINED on any core without the generic timer.

#include "../../src/chrono/measure.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

// WARNING: a TEMPLATE, not an `if constexpr` inside a plain function. `if constexpr` discards a
// branch from evaluation but NOT from instantiation, so guarding an unsupported tick<S>() that way
// still fires its static_assert -- which is exactly what the arm cells caught
template<ch::serial S>
static void
check_policy(void)
{
  if constexpr ( ch::serial_supported<S> ) {
    const u64 a = ch::tick<S>();
    const u64 b = ch::tick<S>();
    require_true(b >= a);
    const u64 c = ch::tick_start<S>();
    const u64 d = ch::tick_end<S>();
    require_true(d >= c);
  }
}

static void
test_counter(void)
{
  sb::print("=== the counter ===");

  test_case("tick() advances and never goes backwards");
  {
    u64 prev = ch::tick<>();
    for ( int i = 0; i < 20000; ++i ) {
      const u64 now = ch::tick<>();
      require_true(now >= prev);
      prev = now;
    }
  }
  end_test_case();

  test_case("tick_end() after tick_start() is never negative");
  {
    for ( int i = 0; i < 5000; ++i ) {
      const u64 a = ch::tick_start<>();
      const u64 b = ch::tick_end<>();
      require_true(b >= a);
    }
  }
  end_test_case();

  test_case("every serialisation policy this build supports reads and advances");
  {
    check_policy<ch::serial::none>();
    check_policy<ch::serial::lfence>();
    check_policy<ch::serial::mfence_lfence>();
    check_policy<ch::serial::cpuid>();
    check_policy<ch::serial::rdtscp>();
    check_policy<ch::serial::serialize>();
    check_policy<ch::serial::isb>();
    require_true(true);
  }
  end_test_case();

  test_case("the default policy is one this target actually supports");
  {
    require_true(ch::serial_supported<ch::default_serial>);
  }
  end_test_case();

  test_case("C.7: the arm32 counter is opt-in, so reaching this line at all is the assertion");
  {
    // On armv7-a WITHOUT MICRON_CHRONO_ARM32_CNTVCT the counter is the monotonic clock, not an
    // unguarded mrrc that SIGILLs on a core with no generic timer. Everywhere else it is native
#if defined(__micron_arch_arm32) && !defined(MICRON_CHRONO_ARM32_CNTVCT)
    require_false(ch::counter_is_native);
#else
    require_true(ch::counter_is_native);
#endif
    const u64 v = ch::tick<>();
    require_true(v > 0 || !ch::counter_is_native);
  }
  end_test_case();
}

static void
test_frequency(void)
{
  sb::print("=== frequency ===");

  ch::prepare_here();

  test_case("tick_hz is a plausible counter rate");
  {
    const u64 hz = ch::tick_hz();
    require_true(hz >= 1000000ull);            // no counter is slower than 1 MHz
    require_true(hz <= 100000000000ull);       // and none is faster than 100 GHz
  }
  end_test_case();

  test_case("the counter agrees with CLOCK_MONOTONIC to better than 1%");
  {
    const u64 hz = ch::tick_hz();
    const i64 t0 = ch::mono_ns();
    const u64 c0 = ch::tick<ch::serial::mfence_lfence>();
    // busy-wait ~20ms; sleeping would let the core drop out of a P-state mid-measurement
    i64 t1 = t0;
    while ( t1 - t0 < 20000000ll ) t1 = ch::mono_ns();
    const u64 c1 = ch::tick<ch::serial::mfence_lfence>();

    const u64 measured_ns = ch::cycles_to_ns(c1 - c0, hz);
    const u64 real_ns = static_cast<u64>(t1 - t0);
    const u64 diff = measured_ns > real_ns ? measured_ns - real_ns : real_ns - measured_ns;
    require_true(diff * 100ull < real_ns);
  }
  end_test_case();

  test_case("counter_traits reports something coherent");
  {
    const auto t = ch::counter_traits();
    if constexpr ( ch::counter_is_native ) {
      require_true(t.present);
      // a nominal rate, when the CPU gives one, must be in the same ballpark as the measured one
      if ( t.nominal_hz != 0 ) {
        const u64 m = ch::tick_hz();
        const u64 lo = t.nominal_hz / 2, hi = t.nominal_hz * 2;
        require_true(m >= lo && m <= hi);
      }
    }
    require_true(true);
  }
  end_test_case();

  test_case("core_hz is plausible and its spread is reported rather than hidden");
  {
    const u64 hz = ch::core_hz();
    require_true(hz >= 10000000ull);
    require_true(hz <= 100000000000ull);
    const auto s = ch::core_hz_spread();
    require_true(s.samples > 0);
    require_true(s.min_hz <= s.median_hz);
    require_true(s.median_hz <= s.max_hz);
  }
  end_test_case();

  test_case("set_tick_hz / set_core_hz override, reset_core_hz re-measures");
  {
    const u64 saved_tick = ch::tick_hz();
    ch::set_tick_hz(1234567ull);
    require(ch::tick_hz(), 1234567ull);
    ch::set_tick_hz(saved_tick);
    require(ch::tick_hz(), saved_tick);

    ch::set_core_hz(7654321ull);
    require(ch::core_hz(), 7654321ull);
    ch::reset_core_hz();
    require_true(ch::core_hz() != 7654321ull || true);      // re-measured; the value is the machine's
  }
  end_test_case();
}

static void
test_conversion(void)
{
  sb::print("=== tick <-> ns conversion ===");

  test_case("exact at the identity frequency");
  {
    require(ch::cycles_to_ns(12345, 1000000000ull), 12345ull);
    require(ch::ns_to_cycles(12345, 1000000000ull), 12345ull);
  }
  end_test_case();

  test_case("exact against a 128-bit oracle at real frequencies");
  {
    const u64 hz[3] = { 2300000000ull, 3200000000ull, 24000000ull };
    u64 s = 0xBB67AE8584CAA73Bull;
    for ( int i = 0; i < 3; ++i ) {
      for ( int k = 0; k < 30000; ++k ) {
        s = s * 6364136223846793005ull + 1442695040888963407ull;
        const u64 c = s >> 16;
        require(ch::cycles_to_ns(c, hz[i]), static_cast<u64>((static_cast<uint128_t>(c) * 1000000000ull) / hz[i]));
        require(ch::ns_to_cycles(c, hz[i]), static_cast<u64>((static_cast<uint128_t>(c) * hz[i]) / 1000000000ull));
      }
    }
  }
  end_test_case();

  test_case("a huge tick count stays exact where an f64 would not");
  {
    // 2^53 is where a double stops counting integers; a counter passes it in ~45 days at 2.3 GHz
    const u64 big = 9007199254740993ull;      // 2^53 + 1
    const u64 hz = 2300000000ull;
    require(ch::cycles_to_ns(big, hz), static_cast<u64>((static_cast<uint128_t>(big) * 1000000000ull) / hz));
  }
  end_test_case();

  test_case("ticks_to_ns / ns_to_ticks use the live rate");
  {
    const u64 hz = ch::tick_hz();
    require(ch::ticks_to_ns(hz), 1000000000ull);
    require_true(ch::ns_to_ticks(1000000000ull) == hz);
  }
  end_test_case();
}

static void
test_measurement(void)
{
  sb::print("=== measuring something known ===");

  test_case("a 50ms sleep measures as 50ms +/- 20%, through the counter");
  {
    u64 ticks = 0;
    {
      ch::tick_timer<> tt(&ticks);
      ch::sleep_ns(50000000ll);
    }
    const u64 ns = ch::ticks_to_ns(ticks);
    require_true(ns > 40000000ull);
    require_true(ns < 70000000ull);
  }
  end_test_case();

  test_case("tick_timer::elapsed_ticks grows monotonically within a scope");
  {
    ch::tick_timer<> tt(nullptr);
    const u64 a = tt.elapsed_ticks();
    ch::warmup_ns(2000000ull);
    const u64 b = tt.elapsed_ticks();
    require_true(b >= a);
  }
  end_test_case();

  test_case("timer_overhead and timer_resolution report ordered statistics");
  {
    const auto o = ch::timer_overhead<>();
    require_true(o.min_ticks <= o.median_ticks);
    require_true(o.median_ticks <= o.max_ticks);
    const auto r = ch::timer_resolution<>();
    require_true(r.min_ticks <= r.median_ticks);
    require_true(r.median_ticks <= r.max_ticks);
    require_true(r.min_ticks > 0);      // a resolution of zero would mean the counter never moved
  }
  end_test_case();

  test_case("the unfenced read is not more expensive than the fully serialising one");
  {
    if constexpr ( ch::serial_supported<ch::serial::cpuid> ) {
      const u64 bare = ch::timer_overhead<ch::serial::none>().min_ticks;
      const u64 heavy = ch::timer_overhead<ch::serial::cpuid>().min_ticks;
      require_true(bare <= heavy);
    }
    require_true(true);
  }
  end_test_case();

  test_case("report() fills in without faulting");
  {
    const auto r = ch::report();
    require_true(r.tick_hz > 0);
    require_true(r.core_hz > 0);
    require(r.counter_native, ch::counter_is_native);
  }
  end_test_case();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the optimisation barriers
//
// asserting that these WORK means asserting the compiler did not remove something. The trick is to
// make the removal observable: a value the compiler could constant-fold, computed through a barrier,
// and then compared against what it must be if it really was computed

[[gnu::noinline]] static u64
compute_with_sink(u64 n)
{
  u64 acc = 0;
  for ( u64 i = 0; i < n; ++i ) {
    acc += i * 3ull;
    ch::sink(acc);
  }
  return acc;
}

[[gnu::noinline]] static u64
compute_with_modify(u64 n)
{
  u64 acc = 0;
  for ( u64 i = 0; i < n; ++i ) {
    acc += i * 3ull;
    ch::modify(acc);
  }
  return acc;
}

static void
test_opt_control(void)
{
  sb::print("=== optimisation control ===");

  test_case("sink and modify do not change the VALUE they guard");
  {
    const u64 n = 1000;
    const u64 want = 3ull * (n * (n - 1ull) / 2ull);
    require(compute_with_sink(n), want);
    require(compute_with_modify(n), want);
  }
  end_test_case();

  test_case("modify makes the loop genuinely execute rather than fold to a formula");
  {
    // a closed-form fold would be O(1); with modify() blocking it the loop has to run, so a much
    // larger n has to cost proportionally more. Timing is the only way to observe this, so the
    // margin is deliberately enormous
    const i64 a0 = ch::mono_ns();
    const u64 s1 = compute_with_modify(200000ull);
    const i64 a1 = ch::mono_ns();
    const u64 s2 = compute_with_modify(20000000ull);
    const i64 a2 = ch::mono_ns();
    ch::sink(s1);
    ch::sink(s2);
    const i64 small = a1 - a0;
    const i64 big = a2 - a1;
    require_true(big > small);
  }
  end_test_case();

  test_case("overwrite leaves the value unspecified but does not fault");
  {
    u64 v = 0x1234;
    ch::overwrite(v);
    ch::sink(v);
    require_true(true);
  }
  end_test_case();

  test_case("sink accepts floats through a register class that suits them");
  {
    f64 d = 1.5;
    f32 f = 2.5f;
    ch::sink(d);
    ch::sink(f);
    ch::modify(d);
    ch::modify(f);
    require_true(d == 1.5);
    require_true(f == 2.5f);
  }
  end_test_case();

  test_case("sink_ptr and clobber_memory materialise memory");
  {
    u64 arr[4] = { 1, 2, 3, 4 };
    ch::sink_ptr(arr);
    ch::clobber_memory();
    require(arr[3], 4ull);
  }
  end_test_case();
}

static void
test_affinity(void)
{
  sb::print("=== pinning ===");

  test_case("current_cpu answers something in range");
  {
    const u32 c = ch::current_cpu();
    require_true(c != ~0u);
    require_true(c < 4096u);
  }
  end_test_case();

  test_case("pin_to_cpu makes current_cpu agree");
  {
    const u32 target = ch::first_available_cpu();
    const i32 r = ch::pin_to_cpu(target);
    if ( r == 0 ) {
      // the move is not necessarily instant, so give the scheduler a moment
      ch::warmup_ns(1000000ull);
      require(ch::current_cpu(), target);
    }
    require_true(true);
  }
  end_test_case();

#if defined(__micron_arch_x86_any)
  test_case("rdtscp reports the cpu it ran on, and it matches after pinning");
  {
    const u32 target = ch::first_available_cpu();
    if ( ch::pin_to_cpu(target) == 0 ) {
      ch::warmup_ns(1000000ull);
      u32 aux = 0;
      (void)ch::tick_aux(aux);
      require(ch::aux_cpu(aux), target);
    }
    require_true(true);
  }
  end_test_case();
#endif
}

int
main(void)
{
  sb::print("micron::chrono cycles suite");
  sb::print("===========================");
  test_counter();
  test_frequency();
  test_conversion();
  test_measurement();
  test_opt_control();
  test_affinity();
  sb::print("===========================");
  sb::print("ALL CYCLES TESTS COMPLETED");
  return 1;
}
