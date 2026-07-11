

#define MICRON_FIBER_FPENV

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)

static u32
get_mxcsr()
{
  u32 v;
  asm volatile("stmxcsr %0" : "=m"(v));
  return v;
}

static void
set_mxcsr(u32 v)
{
  asm volatile("ldmxcsr %0" ::"m"(v));
}

static u16
get_fcw()
{
  u16 v;
  asm volatile("fnstcw %0" : "=m"(v));
  return v;
}

static void
set_fcw(u16 v)
{
  asm volatile("fldcw %0" ::"m"(v));
}

[[gnu::always_inline]] static inline u64
tsc()
{
  u32 lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  sb::test_case("fiber-local MXCSR rounding mode survives suspend/resume; host untouched");
  {
    const u32 host0 = (get_mxcsr() >> 13) & 3u;
    auto r = coro::spin<void>([]() {
      u32 m = get_mxcsr();
      set_mxcsr((m & ~0x6000u) | 0x2000u);
      coro::yield();
      sb::check(((get_mxcsr() >> 13) & 3u) == 1u);
      set_mxcsr(m);
    });
    (void)r.jump();
    sb::check(((get_mxcsr() >> 13) & 3u) == host0);
    r.finish();
    sb::check(((get_mxcsr() >> 13) & 3u) == host0);
  }
  sb::end_test_case();

  sb::test_case("fiber-local x87 control word survives suspend/resume; host untouched");
  {
    const u16 host0 = get_fcw();
    auto r = coro::spin<void>([]() {
      u16 w = get_fcw();
      set_fcw(static_cast<u16>((w & ~0x0C00u) | 0x0400u));
      coro::yield();
      sb::check(((get_fcw() >> 10) & 3u) == 1u);
      set_fcw(w);
    });
    (void)r.jump();
    sb::check(get_fcw() == host0);
    r.finish();
  }
  sb::end_test_case();

  sb::test_case("dirty sticky status bits don't serialize the switch (regression gate)");
  {
    volatile double num = 1.0, den = 3.0;
    volatile double sink = num / den;
    (void)sink;
    volatile u64 warm = 0;
    for ( u64 i = 0; i < 150000000ull; ++i ) warm += i;

    auto r = coro::spin<void>([]() {
      for ( ;; ) coro::yield();
    });
    for ( u64 i = 0; i < 20000; ++i ) (void)r.jump();
    const u64 N = 2000000;
    u64 t0 = tsc();
    for ( u64 i = 0; i < N; ++i ) (void)r.jump();
    u64 dt = tsc() - t0;
    double per = static_cast<double>(dt) / static_cast<double>(N);
    sb::print("fpenv jump+yield roundtrip: ", per, " tsc-cyc (fixed ~25, broken ~235)");
    sb::check(per < 150.0);
    r.dismiss();
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL FPENV TESTS PASSED ===");
  return 1;
}

#else

int
main()
{

  sb::print("=== FPENV TESTS SKIPPED (x86-only feature) ===");
  return 1;
}

#endif
