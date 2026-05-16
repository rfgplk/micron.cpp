// math_cr.cpp
// Rigorous snowball test suite for math::cr — Ziv-refined correctly
// rounded transcendentals (log, exp, sin, cos, f32 and f64).
//
// What this verifies:
//   * The Ziv refinement infrastructure (ulp_of, round_test_*, drive_*,
//     round_dd_to_f32) is internally consistent.
//   * The dd64 kernel companions (log_dd_f64, exp_dd_f64, sin_dd_f64,
//     cos_dd_f64) round to the same f64 as the existing faithful kernel
//     within 1 ulp on typical inputs — i.e. enabling the cr policy
//     does not regress accuracy.  Where the dd64 kernel is *more*
//     accurate than the faithful one, ulp distance from libm's
//     correctly-rounded reference is ≤ 1.
//   * Special values (NaN, ±inf, 0, ±1) survive the cr path unchanged.
//   * The mk:: dispatch routes cr_tag to the cr:: implementation.
//
// What this deliberately does NOT verify:
//   * Exhaustive ulp distance over the full f64 domain — that's a 2^64
//     sweep beyond a unit test's scope.  We sample representative values
//     including near-zero-of-sin/cos cases that previously failed (the
//     3-word Cody–Waite reduction in bits/trig.hpp had a typo'd pio2_lo
//     that drifted ~117k ulps from libm; fixed alongside this change).

#include "../../src/math/cr.hpp"
#include "../../src/math/dd64.hpp"
#include "../../src/math/ieee.hpp"
#include "../../src/math/mk.hpp"
#include "../../src/math/policy.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

// reference-via-builtin (libm's correctly-rounded sin/cos/log/exp).
static f64
ref_log(f64 x)
{
  return __builtin_log(x);
}

static f64
ref_exp(f64 x)
{
  return __builtin_exp(x);
}

static f64
ref_sin(f64 x)
{
  return __builtin_sin(x);
}

static f64
ref_cos(f64 x)
{
  return __builtin_cos(x);
}

static f32
ref_logf(f32 x)
{
  return __builtin_logf(x);
}

static f32
ref_expf(f32 x)
{
  return __builtin_expf(x);
}

static f32
ref_sinf(f32 x)
{
  return __builtin_sinf(x);
}

static f32
ref_cosf(f32 x)
{
  return __builtin_cosf(x);
}

template<typename F>
static bool
within_ulp(F got, F want, i64 max_ulp)
{
  if ( ieee::is_nan(got) && ieee::is_nan(want) ) return true;
  return ieee::ulp_distance(got, want) <= max_ulp;
}

int
main()
{
  print("=== MATH::CR (Ziv-refined CR) TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // ulp_of — primitive used by the rounding test.
  test_case("cr::ulp_of — basic invariants");
  {
    require_true(cr::ulp_of<f64>(1.0) == 0x1.0p-52);
    require_true(cr::ulp_of<f64>(2.0) == 0x1.0p-51);
    require_true(cr::ulp_of<f64>(0.5) == 0x1.0p-53);
    require_true(cr::ulp_of<f32>(1.0f) == 0x1.0p-23f);
    require_true(cr::ulp_of<f64>(0.0) == ieee::from_bits<f64>(1ULL));
    require_true(cr::ulp_of<f64>(-3.5) == cr::ulp_of<f64>(3.5));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // round_test — accept far-from-boundary, reject close.
  test_case("cr::round_test_f64 — far from boundary accepts");
  {
    dd64 y_safe{ 1.0, 0x1.0p-100 };
    require_true(cr::round_test_f64(y_safe, 0x1.0p-100));
    // hi away from zero, lo right at the tipping point with abs_err
    // pushing across — must reject.
    dd64 y_edge{ 1.0, 0.5 * 0x1.0p-52 - 0x1.0p-90 };
    require_false(cr::round_test_f64(y_edge, 0x1.0p-60));
  }
  end_test_case();

  test_case("cr::round_test_f32 — accepts comfortably for tight dd64");
  {
    // f32 ulp at 1.0 is 2^-23.  abs_err 2^-50 is far below half ulp.
    require_true(cr::round_test_f32(1.0, 0x1.0p-50));
    // abs_err half-an-f32-ulp pushes right onto the boundary — reject.
    require_false(cr::round_test_f32(1.0 + 0.5 * 0x1.0p-23, 0x1.0p-30));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // round_dd_to_f64 — basic identity.
  test_case("cr::round_dd_to_f64 — sum reduction");
  {
    require(cr::round_dd_to_f64(dd64{ 1.0, 0.0 }), 1.0);
    require(cr::round_dd_to_f64(dd64{ 1.0, 0x1.0p-60 }), 1.0);
    // a (hi, lo) that's half-an-ulp above hi rounds up.
    f64 r = cr::round_dd_to_f64(dd64{ 1.0, 0x1.0p-52 });
    require(r, 1.0 + 0x1.0p-52);
  }
  end_test_case();

  // round_dd_to_f32 — round-to-odd cast.
  test_case("cr::round_dd_to_f32 — round-to-odd cast");
  {
    require(cr::round_dd_to_f32(dd64{ 1.0, 0.0 }), 1.0f);
    // (hi, +tiny) must not round below hi.
    f32 r1 = cr::round_dd_to_f32(dd64{ 1.0, 0x1.0p-30 });
    require_true(r1 >= 1.0f);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // log — CR vs. faithful: must match within 1 ulp on representative
  // inputs across the dynamic range.
  test_case("cr::log f64 vs faithful — within 1 ulp across the range");
  {
    static const f64 xs[] = {
      0x1.0p-1000, 0x1.0p-100, 0.001, 0.01,   0.1,   0.25,    0.5,
      0.999,       1.0,        1.001, 1.5,    2.0,   2.71828, 3.14159,
      7.0,         10.0,       100.0, 1.5e10, 1e100, 1e200,   0x1.fffffffffffffp+1023,
    };
    for ( f64 x : xs ) {
      f64 cr_v = cr::log<f64>(x);
      f64 fa_v = mk::log_ns::log<f64>(x);
      require_true(within_ulp<f64>(cr_v, fa_v, 1));
    }
  }
  end_test_case();

  // log — CR vs. libm reference: should be tight on safe inputs
  // (smooth domain, no near-special-point cancellation).
  test_case("cr::log f64 vs libm — within 1 ulp on smooth domain");
  {
    static const f64 xs[] = {
      0.5, 1.5, 2.0, 2.71828, 7.0, 10.0, 100.0, 1.5e10, 1e100,
    };
    for ( f64 x : xs ) {
      f64 got = cr::log<f64>(x);
      f64 want = ref_log(x);
      require_true(within_ulp<f64>(got, want, 1));
    }
  }
  end_test_case();

  // log — f32: dd64 stage runs in f64 then round-to-odd cast.
  test_case("cr::log f32 — within 1 ulp of libm");
  {
    static const f32 xs[] = {
      0x1.0p-30f, 0.5f, 1.0f, 1.5f, 2.0f, 2.71828f, 10.0f, 1e10f,
    };
    for ( f32 x : xs ) {
      f32 got = cr::log<f32>(x);
      f32 want = ref_logf(x);
      require_true(within_ulp<f32>(got, want, 1));
    }
  }
  end_test_case();

  // log — special values.
  test_case("cr::log — special values");
  {
    require(cr::log<f64>(1.0), 0.0);
    require_true(ieee::is_inf(cr::log<f64>(0.0)));
    require_true(ieee::is_nan(cr::log<f64>(-1.0)));
    require_true(ieee::is_nan(cr::log<f64>(ieee::qnan_v<f64>())));
    require_true(ieee::is_inf(cr::log<f64>(ieee::inf_v<f64>(0))));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // exp — CR vs. faithful, then vs. libm.
  test_case("cr::exp f64 vs faithful — within 1 ulp across the range");
  {
    static const f64 xs[] = {
      -700.0, -100.0, -10.0, -1.0, -0.5, -0x1.0p-30, 0.0, 0x1.0p-30, 0.5, 1.0, 2.0, 10.0, 100.0, 700.0,
    };
    for ( f64 x : xs ) {
      f64 cr_v = cr::exp<f64>(x);
      f64 fa_v = mk::exp_ns::exp<f64>(x);
      require_true(within_ulp<f64>(cr_v, fa_v, 1));
    }
  }
  end_test_case();

  test_case("cr::exp f64 vs libm — within 1 ulp");
  {
    static const f64 xs[] = {
      -10.0, -1.0, -0.5, 0.0, 0.5, 1.0, 2.0, 10.0, 100.0,
    };
    for ( f64 x : xs ) {
      f64 got = cr::exp<f64>(x);
      f64 want = ref_exp(x);
      require_true(within_ulp<f64>(got, want, 1));
    }
  }
  end_test_case();

  test_case("cr::exp f32 — within 1 ulp of libm");
  {
    static const f32 xs[] = {
      -80.0f, -10.0f, -1.0f, 0.0f, 0.5f, 1.0f, 2.0f, 10.0f, 80.0f,
    };
    for ( f32 x : xs ) {
      f32 got = cr::exp<f32>(x);
      f32 want = ref_expf(x);
      require_true(within_ulp<f32>(got, want, 1));
    }
  }
  end_test_case();

  test_case("cr::exp — special values");
  {
    require(cr::exp<f64>(0.0), 1.0);
    require(cr::exp<f64>(-1000.0), 0.0);
    require_true(ieee::is_inf(cr::exp<f64>(1000.0)));
    require_true(ieee::is_nan(cr::exp<f64>(ieee::qnan_v<f64>())));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // sin / cos — CR vs. faithful (CR must not regress).  We exclude
  // points near zeros of sin/cos since the underlying 3-word
  // Cody–Waite reduction loses bits there in BOTH cr and faithful
  // paths — that's a separate kernel limitation (see header comment).
  test_case("cr::sin f64 vs faithful — within 1 ulp on safe inputs");
  {
    static const f64 xs[] = {
      -10.0,
      -1.0,
      -0.5,
      -0x1.0p-30,
      0.0,
      0x1.0p-30,
      0.5,
      1.0,
      1.0471975511965976,      // π/3
      0.7853981633974483,      // π/4
      10.0,
      100.0,
    };
    for ( f64 x : xs ) {
      f64 cr_v = cr::sin<f64>(x);
      f64 fa_v = mk::trig::sin<f64>(x);
      require_true(within_ulp<f64>(cr_v, fa_v, 1));
    }
  }
  end_test_case();

  test_case("cr::sin f64 vs libm — within 1 ulp on smooth inputs");
  {
    static const f64 xs[] = {
      -1.0,
      -0.5,
      0.0,
      0.5,
      1.0,
      0.7853981633974483,      // π/4
      1.0471975511965976,      // π/3
    };
    for ( f64 x : xs ) {
      f64 got = cr::sin<f64>(x);
      f64 want = ref_sin(x);
      require_true(within_ulp<f64>(got, want, 1));
    }
  }
  end_test_case();

  // Near-zero-of-sin inputs.  Before the pio2_lo fix in bits/trig.hpp,
  // these were ~117k ulps off; they now match libm to ≤1 ulp.
  test_case("cr::sin f64 — near zeros of sin (post pio2_lo fix)");
  {
    static const f64 xs[] = {
      3.14159265,      // ≈ π − 3.59e-9
      -3.14159265,
      6.283185307,      // ≈ 2π
      9.42477796,       // ≈ 3π
    };
    for ( f64 x : xs ) {
      f64 got = cr::sin<f64>(x);
      f64 want = ref_sin(x);
      require_true(within_ulp<f64>(got, want, 1));
    }
  }
  end_test_case();

  test_case("cr::cos f64 vs faithful — within 1 ulp on safe inputs");
  {
    static const f64 xs[] = {
      -10.0, -1.0, -0.5, 0.0, 0.5, 1.0, 0.7853981633974483, 1.0471975511965976, 10.0, 100.0,
    };
    for ( f64 x : xs ) {
      f64 cr_v = cr::cos<f64>(x);
      f64 fa_v = mk::trig::cos<f64>(x);
      require_true(within_ulp<f64>(cr_v, fa_v, 1));
    }
  }
  end_test_case();

  test_case("cr::cos f64 vs libm — within 1 ulp on smooth inputs");
  {
    static const f64 xs[] = {
      -1.0, -0.5, 0.0, 0.5, 1.0, 0.7853981633974483, 1.0471975511965976,
    };
    for ( f64 x : xs ) {
      f64 got = cr::cos<f64>(x);
      f64 want = ref_cos(x);
      require_true(within_ulp<f64>(got, want, 1));
    }
  }
  end_test_case();

  // Near-zero-of-cos inputs (cos has zeros at π/2 + k·π).  Same fix.
  test_case("cr::cos f64 — near zeros of cos (post pio2_lo fix)");
  {
    static const f64 xs[] = {
      1.5707963,      // ≈ π/2
      -1.5707963,
      4.7123889,      // ≈ 3π/2
    };
    for ( f64 x : xs ) {
      f64 got = cr::cos<f64>(x);
      f64 want = ref_cos(x);
      require_true(within_ulp<f64>(got, want, 1));
    }
  }
  end_test_case();

  test_case("cr::sin / cos f32 — within 1 ulp of libm on smooth");
  {
    static const f32 xs[] = { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 0.7853981f };
    for ( f32 x : xs ) {
      require_true(within_ulp<f32>(cr::sin<f32>(x), ref_sinf(x), 1));
      require_true(within_ulp<f32>(cr::cos<f32>(x), ref_cosf(x), 1));
    }
  }
  end_test_case();

  test_case("cr::sin / cos — special values");
  {
    require(cr::sin<f64>(0.0), 0.0);
    require(cr::cos<f64>(0.0), 1.0);
    require_true(ieee::is_nan(cr::sin<f64>(ieee::qnan_v<f64>())));
    require_true(ieee::is_nan(cr::sin<f64>(ieee::inf_v<f64>(0))));
    require_true(ieee::is_nan(cr::cos<f64>(ieee::qnan_v<f64>())));
    require_true(ieee::is_nan(cr::cos<f64>(ieee::inf_v<f64>(0))));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // policy::cr dispatch — the mk:: surface must route cr_tag inputs
  // to cr::*, not to the faithful kernel.  We confirm by exact equality
  // (cr::log<f64>(x) is a constexpr inline; the dispatch site simply
  // forwards to it when P=cr_tag).
  test_case("policy::cr — dispatch routes to cr::* via mk::");
  {
    require(mk::log_ns::log<f64>(2.0, policy::cr), cr::log<f64>(2.0));
    require(mk::exp_ns::exp<f64>(1.0, policy::cr), cr::exp<f64>(1.0));
    require(mk::trig::sin<f64>(1.0, policy::cr), cr::sin<f64>(1.0));
    require(mk::trig::cos<f64>(1.0, policy::cr), cr::cos<f64>(1.0));
    // The default (faithful) policy is unaffected.
    require_true(within_ulp<f64>(mk::log_ns::log<f64>(2.0), ref_log(2.0), 1));
  }
  end_test_case();

  print("=== MATH::CR TESTS DONE ===");
  return 1;
}
