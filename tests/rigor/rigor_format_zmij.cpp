// rigor_format_zmij.cpp -- Żmij/Ryu differential, public round-trip, and precision fuzz.

#include "../../src/string/conversions/chars.hpp"
#include "../../src/string/conversions/floating_point.hpp"

#include "../support/format_rigor.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;
namespace zj = micron::__impl::__zmij;
namespace ry = micron::__impl::__ryu;
namespace fp = micron::__impl::__fpconv;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static usize __random64 = 12000;
constexpr static usize __random32 = 8000;
constexpr static usize __precision_random = 5000;
constexpr static usize __wide_precision_random = 100;
#else
constexpr static usize __random64 = 300000;
constexpr static usize __random32 = 150000;
constexpr static usize __precision_random = 80000;
constexpr static usize __wide_precision_random = 1000;
#endif

static f64
f64_from_bits(u64 bits) noexcept
{
  return micron::math::ieee::from_bits<f64>(bits);
}

static f32
f32_from_bits(u32 bits) noexcept
{
  return micron::math::ieee::from_bits<f32>(bits);
}

static bool
__same(const char *a, usize an, const char *b, usize bn) noexcept
{
  if ( an != bn ) return false;
  for ( usize i = 0; i < an; ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

static void
__shortest64(u64 bits)
{
  const f64 value = f64_from_bits(bits);
  char a[32];
  char b[32];
  const usize an = zj::d2s_buffered(value, a);
  const usize bn = ry::d2s_buffered(value, b);
  if ( !__same(a, an, b, bn) ) {
    micron::io::print("zmij/ryu d2s mismatch bits=", bits, "\n");
    require_true(false);
  }

  char pub[32];
  const usize pn = fp::d2s_buffered(value, pub);
  require_true(__same(a, an, pub, pn));

  const u32 exp = static_cast<u32>((bits >> 52) & 0x7ffu);
  const u64 sig = bits & ((1ull << 52) - 1);
  if ( exp != 0x7ffu && (exp != 0 || sig != 0) ) {
    f64 parsed = 0;
    require_true(micron::try_parse_double(a, an, parsed));
    require_true(micron::math::ieee::to_bits(parsed) == bits);
  }
}

static void
__shortest32(u32 bits)
{
  const f32 value = f32_from_bits(bits);
  char a[20];
  char b[20];
  const usize an = zj::f2s_buffered(value, a);
  const usize bn = ry::__f32::f2s_buffered(value, b);
  if ( !__same(a, an, b, bn) ) {
    micron::io::print("zmij/ryu f2s mismatch bits=", static_cast<u64>(bits), "\n");
    require_true(false);
  }

  char pub[20];
  const usize pn = fp::f2s_buffered(value, pub);
  require_true(__same(a, an, pub, pn));

  const u32 exp = bits >> 23 & 0xffu;
  const u32 sig = bits & ((1u << 23) - 1);
  if ( exp != 0xffu && (exp != 0 || sig != 0) ) {
    f32 parsed = 0;
    require_true(micron::try_parse_float(a, an, parsed));
    require_true(micron::math::ieee::to_bits(parsed) == bits);
  }
}

static void
__precision(u64 bits, u32 precision)
{
  const f64 value = f64_from_bits(bits);
  char a[1100];
  char b[1100];

  usize an = zj::d2f_buffered(value, a, sizeof(a), precision);
  usize bn = ry::d2f_buffered(value, b, sizeof(b), precision);
  if ( !__same(a, an, b, bn) ) {
    micron::io::print("zmij/ryu fixed mismatch bits=", bits, " precision=", static_cast<u64>(precision), "\n");
    require_true(false);
  }

  an = zj::d2e_buffered(value, a, sizeof(a), precision);
  bn = ry::d2e_buffered(value, b, sizeof(b), precision);
  if ( !__same(a, an, b, bn) ) {
    micron::io::print("zmij/ryu scientific mismatch bits=", bits, " precision=", static_cast<u64>(precision), "\n");
    require_true(false);
  }

  for ( u32 flags = 0; flags < 4; ++flags ) {
    an = zj::d2g_buffered(value, a, sizeof(a), precision, (flags & 1) != 0, (flags & 2) != 0);
    bn = ry::d2g_buffered(value, b, sizeof(b), precision, (flags & 1) != 0, (flags & 2) != 0);
    if ( !__same(a, an, b, bn) ) {
      micron::io::print("zmij/ryu general mismatch bits=", bits, " precision=", static_cast<u64>(precision),
                        " flags=", static_cast<u64>(flags), "\n");
      require_true(false);
    }
  }
}

static void
__batch64(const f64 (&values)[4], usize stride)
{
  char out[4 * 37 + 16];
  for ( char &c : out ) c = '#';
  const micron::chars4_result result = micron::to_chars4(out, stride, values);
  for ( usize lane = 0; lane < 4; ++lane ) {
    char scalar[32];
    const usize n = fp::d2s_buffered(values[lane], scalar);
    require_true(result[lane] == n);
    require_true(__same(out + lane * stride, result[lane], scalar, n));
    for ( usize i = result[lane]; i < stride; ++i ) require_true(out[lane * stride + i] == '#');
  }
  for ( usize i = stride * 4; i < sizeof(out); ++i ) require_true(out[i] == '#');
}

static void
__batch32(const f32 (&values)[4], usize stride)
{
  char out[4 * 23 + 16];
  for ( char &c : out ) c = '#';
  const micron::chars4_result result = micron::to_chars4(out, stride, values);
  for ( usize lane = 0; lane < 4; ++lane ) {
    char scalar[20];
    const usize n = fp::f2s_buffered(values[lane], scalar);
    require_true(result[lane] == n);
    require_true(__same(out + lane * stride, result[lane], scalar, n));
    for ( usize i = result[lane]; i < stride; ++i ) require_true(out[lane * stride + i] == '#');
  }
  for ( usize i = stride * 4; i < sizeof(out); ++i ) require_true(out[i] == '#');
}

int
main()
{
  sb::print("=== ZMIJ FLOAT FORMAT RIGOR ===");

  test_case("compressed powers reconstruct the expanded table exactly");
  {
    for ( u32 i = 0; i < 649; ++i ) {
      const zj::uint128 computed = zj::pow10_significand_table::compute(i);
      const zj::uint128 stored = zj::__data.pow10_significands[static_cast<i32>(i) - 307];
      require_true(computed.hi == stored.hi && computed.lo == stored.lo);
    }
  }
  end_test_case();

  test_case("upstream boundary and carry corpus");
  {
    constexpr u64 boundary64[]
        = { 0x0000000000000001ull, 0x000fffffffffffffull, 0x0010000000000000ull, 0x3fb999999999999aull, 0x3fd6666666666666ull,
            0x3fefffffffffffffull, 0x3ff0000000000000ull, 0x3ff0000000000001ull, 0x4023ffffffffffffull, 0x4024000000000000ull,
            0x431fffffffffffffull, 0x4320000000000000ull, 0x7fefffffffffffffull, 0x8000000000000001ull, 0x8010000000000000ull,
            0x1e1708d0f84d3de7ull, 0x2a690c59d89584c7ull, 0x2db34076d9def500ull };
    for ( u64 bits : boundary64 ) __shortest64(bits);
    constexpr u32 boundary32[] = { 0x00000001u, 0x007fffffu, 0x00800000u, 0x3dcccccdu, 0x3f7fffffu, 0x3f800000u,
                                   0x3f800001u, 0x4affffffu, 0x4b000000u, 0x7f7fffffu, 0x80000001u, 0x80800000u };
    for ( u32 bits : boundary32 ) __shortest32(bits);
  }
  end_test_case();

  test_case("every exponent with edge and random mantissas");
  {
    prng random(0x5a17c9e34d206bf1ull);
    for ( u32 exponent = 0; exponent < 2047; ++exponent ) {
      const u64 base = static_cast<u64>(exponent) << 52;
      __shortest64(base);
      __shortest64(base | 1);
      __shortest64(base | ((1ull << 52) - 1));
      __shortest64(base | (random.next() & ((1ull << 52) - 1)) | ((random.next() & 1) << 63));
    }
    for ( u32 exponent = 0; exponent < 255; ++exponent ) {
      const u32 base = exponent << 23;
      __shortest32(base);
      __shortest32(base | 1);
      __shortest32(base | ((1u << 23) - 1));
      __shortest32(base | (static_cast<u32>(random.next()) & ((1u << 23) - 1)) | (static_cast<u32>(random.next() & 1) << 31));
    }
  }
  end_test_case();

  test_case("fixed-seed shortest and round-trip fuzz");
  {
    prng random(0xd1ff3a7e9b5026c4ull);
    for ( usize i = 0; i < __random64; ++i ) __shortest64(random.next());
    for ( usize i = 0; i < __random32; ++i ) __shortest32(static_cast<u32>(random.next()));
  }
  end_test_case();

  test_case("four-lane shortest matches scalar for every exceptional mask");
  {
    constexpr u64 ordinary64[] = { 0x3ff8000000000000ull, 0xc005bf0a8b145769ull, 0x0000000000000001ull, 0x7fefffffffffffffull };
    constexpr u32 ordinary32[] = { 0x3fc00000u, 0xc02df854u, 0x00000001u, 0x7f7fffffu };
    for ( u32 mask = 0; mask < 16; ++mask ) {
      f64 values64[4];
      f32 values32[4];
      for ( usize lane = 0; lane < 4; ++lane ) {
        const u64 bits64 = (mask >> lane & 1u) != 0 ? 0x7ff0000000000000ull | (lane == 3 ? 1ull : 0ull) : ordinary64[lane];
        const u32 bits32 = (mask >> lane & 1u) != 0 ? 0x7f800000u | (lane == 3 ? 1u : 0u) : ordinary32[lane];
        values64[lane] = f64_from_bits(bits64);
        values32[lane] = f32_from_bits(bits32);
      }
      __batch64(values64, micron::f64_shortest_chars_capacity);
      __batch64(values64, 37);
      __batch32(values32, micron::f32_shortest_chars_capacity);
      __batch32(values32, 23);
    }
  }
  end_test_case();

  test_case("four-lane shortest fixed-seed fuzz");
  {
    prng random(0xe6b47c0a932d185full);
#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
    constexpr usize rounds = 2000;
#else
    constexpr usize rounds = 20000;
#endif
    for ( usize i = 0; i < rounds; ++i ) {
      f64 values64[4];
      f32 values32[4];
      for ( usize lane = 0; lane < 4; ++lane ) {
        values64[lane] = f64_from_bits(random.next());
        values32[lane] = f32_from_bits(static_cast<u32>(random.next()));
      }
      __batch64(values64, micron::f64_shortest_chars_capacity);
      __batch32(values32, micron::f32_shortest_chars_capacity);
    }
  }
  end_test_case();

  test_case("all decimal formats agree with the legacy engine");
  {
    prng random(0x69c82fa105d37be4ull);
    for ( usize i = 0; i < __precision_random; ++i ) {
      const u32 precision = static_cast<u32>(random.next() % 25);
      __precision(random.next(), precision);
    }
  }
  end_test_case();

  test_case("wide precision agrees through the exact bigint path");
  {
    constexpr u32 precisions[] = { 25, 50, 100, 300, 700 };
    prng random(0xa830de147f2c596bull);
    for ( usize i = 0; i < __wide_precision_random; ++i )
      __precision(random.next(), precisions[random.next() % (sizeof(precisions) / sizeof(precisions[0]))]);
  }
  end_test_case();

  test_case("exact-capacity and no-write-on-failure");
  {
    const f64 values[] = { -0.0, 0.1, 9.99, 1e-300, 1.7976931348623157e308 };
    for ( f64 value : values ) {
      for ( u32 precision = 0; precision <= 18; ++precision ) {
        char full[1100];
        const usize need = zj::d2f_buffered(value, full, sizeof(full), precision);
        char short_buf[1100];
        for ( usize i = 0; i < sizeof(short_buf); ++i ) short_buf[i] = '#';
        require_true(zj::d2f_buffered(value, short_buf, need == 0 ? 0 : need - 1, precision) == 0);
        for ( usize i = 0; i < sizeof(short_buf); ++i ) require_true(short_buf[i] == '#');
        require_true(zj::d2f_buffered(value, short_buf, need, precision) == need);
        require_true(__same(full, need, short_buf, need));
      }
    }
  }
  end_test_case();

  sb::print("=== ZMIJ FLOAT FORMAT RIGOR PASSED ===");
  return 1;
}
