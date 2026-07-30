// rigor_format_ryu.cpp — exact-oracle suite for the Ryu shortest-form core
// (src/string/conversions/bits.hpp + floating_point.hpp).

#include "../../src/string/conversions/floating_point.hpp"

#include "../support/format_rigor.hpp"
#include "../support/ryu_oracle.hpp"

using namespace mtest::format_rigor;
using mtest::prng;
namespace ro = mtest::ryu_oracle;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)

constexpr static const usize N_SWEEP_MANT = 4;
constexpr static const usize N_RANDOM = 8000;
constexpr static const usize N_VP = 2000;
constexpr static const usize N_RANDOM32 = 4000;
constexpr static const usize CLOSEST_STRIDE = 16;
#else
constexpr static const usize N_SWEEP_MANT = 24;
constexpr static const usize N_RANDOM = 150000;
constexpr static const usize N_VP = 20000;
constexpr static const usize N_RANDOM32 = 60000;
constexpr static const usize CLOSEST_STRIDE = 8;
#endif

static f64
f64_from_bits(u64 b)
{
  f64 x;
  __builtin_memcpy(&x, &b, 8);
  return x;
}

static f32
f32_from_bits(u32 b)
{
  f32 x;
  __builtin_memcpy(&x, &b, 4);
  return x;
}

static f64
f64_opaque(u64 b)
{
  volatile u64 vb = b;
  u64 t = vb;
  return f64_from_bits(t);
}

static f32
f32_opaque(u32 b)
{
  volatile u32 vb = b;
  u32 t = vb;
  return f32_from_bits(t);
}

static void
check_f64(u64 bits, bool check_closest)
{
  ro::fbits v = ro::decompose64(bits);
  if ( v.special || v.zero ) return;
  auto s = micron::double_to_string<schar>(f64_from_bits(bits));
  ro::parsed p = ro::parse_text(s.cdata(), s.size());
  if ( !p.ok || p.special || p.neg != v.neg || !ro::decodes_to(p.m, p.e10, v) || !ro::is_shortest(p.m, p.e10, v)
       || (check_closest && !ro::is_closest(p.m, p.e10, v)) ) {
    micron::io::print("d2s FAIL bits=", bits, " text='", s, "'\n");
    require_true(false);
  }
}

static void
check_f32(u32 bits, bool check_closest)
{
  ro::fbits v = ro::decompose32(bits);
  if ( v.special || v.zero ) return;
  auto s = micron::float_to_string<schar>(f32_from_bits(bits));
  ro::parsed p = ro::parse_text(s.cdata(), s.size());
  if ( !p.ok || p.special || p.neg != v.neg || !ro::decodes_to(p.m, p.e10, v) || !ro::is_shortest(p.m, p.e10, v)
       || (check_closest && !ro::is_closest(p.m, p.e10, v)) ) {
    micron::io::print("f2s FAIL bits=", static_cast<u64>(bits), " text='", s, "'\n");
    require_true(false);
  }
}

int
main()
{
  sb::print("=== FORMAT/RYU RIGOR SUITE ===");

  test_case("pow5_compute exact for all 326 forward indices");
  {
    for ( u32 i = 0; i < 326; ++i ) {
      u64 r[2];
      micron::__impl::__ryu::pow5_compute(i, r);
      if ( !ro::fwd_entry_exact(i, r[0], r[1]) ) {
        micron::io::print("fwd entry wrong at i=", static_cast<u64>(i), "\n");
        require_true(false);
      }
    }
  }
  end_test_case();

  test_case("pow5_compute_inv exact for all reachable inverse indices");
  {
    for ( u32 q = 0; q < 291; ++q ) {
      u64 r[2];
      micron::__impl::__ryu::pow5_compute_inv(q, r);
      if ( !ro::inv_entry_exact(q, r[0], r[1]) ) {
        micron::io::print("inv entry wrong at q=", static_cast<u64>(q), "\n");
        require_true(false);
      }
    }
  }
  end_test_case();

  test_case("regression pins (2026-07 table corruption)");
  {

    auto a = micron::double_to_string<schar>(f64_from_bits(0x2db34076d9def500ull));
    require_true(hstr_equal_cstr(a, "1.5121432313921845e-88"));

    auto b = micron::double_to_string<schar>(f64_from_bits(0x1e1708d0f84d3de7ull));
    require_true(hstr_equal_cstr(b, "1e-163"));

    check_f64(0x2a690c59d89584c7ull, true);

    for ( u64 x = 0; x < 256; ++x ) check_f64(0x2db34076d9def500ull + x, true);
  }
  end_test_case();

  test_case("d2s exponent sweep with edge + random mantissas");
  {
    prng rng(0x9E3779B97F4A7C15ull);
    for ( u32 E = 0; E <= 2046; ++E ) {
      u64 base = static_cast<u64>(E) << 52;
      check_f64(base | 0, true);
      check_f64(base | 1, true);
      check_f64(base | ((1ull << 52) - 1), true);
      for ( usize k = 0; k < N_SWEEP_MANT; ++k ) {
        u64 m = rng.next() & ((1ull << 52) - 1);
        u64 sign = (rng.next() & 1) << 63;
        check_f64(sign | base | m, (k & 3) == 0);
      }
    }
  }
  end_test_case();

  test_case("d2s uniform random");
  {
    prng rng(0xC0FFEE0DDF00Dull);
    for ( usize k = 0; k < N_RANDOM; ++k ) check_f64(rng.next(), (k % CLOSEST_STRIDE) == 0);
  }
  end_test_case();

  test_case("d2s odd-mantissa e2 in [-4,-1]");
  {
    prng rng(0xF4F4F4F4F4ull);
    for ( u32 E = 1073; E <= 1076; ++E ) {
      for ( usize k = 0; k < N_VP; ++k ) {
        u64 m = (rng.next() & ((1ull << 52) - 1)) | 1;
        check_f64((static_cast<u64>(E) << 52) | m, true);
      }
    }
  }
  end_test_case();

  test_case("d2s powers of ten");
  {

    f64 x = 1.0;
    for ( i32 k = 0; k <= 308; ++k ) {
      u64 b;
      __builtin_memcpy(&b, &x, 8);
      check_f64(b, true);
      x *= 10.0;
    }
    x = 1.0;
    for ( i32 k = 0; k >= -308; --k ) {
      u64 b;
      __builtin_memcpy(&b, &x, 8);
      check_f64(b, true);
      x /= 10.0;
    }
  }
  end_test_case();

  test_case("f2s exponent sweep with edge + random mantissas");
  {
    prng rng(0x5EEDF00D5EEDF00Dull);
    for ( u32 E = 0; E <= 254; ++E ) {
      u32 base = E << 23;
      check_f32(base | 0, true);
      check_f32(base | 1, true);
      check_f32(base | ((1u << 23) - 1), true);
      for ( usize k = 0; k < N_SWEEP_MANT; ++k ) {
        u32 m = static_cast<u32>(rng.next()) & ((1u << 23) - 1);
        u32 sign = static_cast<u32>(rng.next() & 1) << 31;
        check_f32(sign | base | m, true);
      }
    }
  }
  end_test_case();

  test_case("f2s uniform random");
  {
    prng rng(0xABCDEF0123456789ull);
    for ( usize k = 0; k < N_RANDOM32; ++k ) check_f32(static_cast<u32>(rng.next()), (k % CLOSEST_STRIDE) == 0);
  }
  end_test_case();

  test_case("special text forms");
  {
    require_true(hstr_equal_cstr(micron::double_to_string<schar>(f64_opaque(0x0000000000000000ull)), "0.0"));
    require_true(hstr_equal_cstr(micron::double_to_string<schar>(f64_opaque(0x8000000000000000ull)), "-0.0"));
    require_true(hstr_equal_cstr(micron::double_to_string<schar>(f64_from_bits(0x7ff8000000000000ull)), "NaN"));
    require_true(hstr_equal_cstr(micron::double_to_string<schar>(f64_from_bits(0x7ff0000000000000ull)), "Inf"));
    require_true(hstr_equal_cstr(micron::double_to_string<schar>(f64_from_bits(0xfff0000000000000ull)), "-Inf"));
    require_true(hstr_equal_cstr(micron::float_to_string<schar>(f32_opaque(0x00000000u)), "0E0"));
    require_true(hstr_equal_cstr(micron::float_to_string<schar>(f32_opaque(0x80000000u)), "-0E0"));
  }
  end_test_case();

  test_case("to_fixed / to_scientific / to_general smoke on fixed classes");
  {
    const f64 vals[] = { f64_from_bits(0x2db34076d9def500ull), f64_from_bits(0x1e1708d0f84d3de7ull), 1.5e15, 0.1, 123456.789 };
    for ( f64 x : vals ) {
      require_true(micron::to_fixed<schar>(x, 6).size() > 0);
      require_true(micron::to_scientific<schar>(x, 6).size() > 0);
      require_true(micron::to_general<schar>(x, 6).size() > 0);
    }
    require_true(hstr_equal_cstr(micron::to_fixed<schar>(123456.789, 3), "123456.789"));
  }
  end_test_case();

  sb::print("=== FORMAT/RYU RIGOR SUITE PASSED ===");
  return 1;
}
