// test_avx.cpp
// Behavioral coverage for `micron::simd::avx::*` (256-bit ps / pd, plus
// 256-bit load / store / cast / movemask / convert / broadcast / horizontal
// / testz family).

#include "../../src/simd/aliases/avx.hpp"
#include "../snowball/snowball.hpp"

namespace ma = ::micron::simd::avx;

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

template <typename T>
[[gnu::always_inline]] inline bool
v_eq(T a, T b) noexcept
{
  alignas(32) unsigned char ba[sizeof(T)];
  alignas(32) unsigned char bb[sizeof(T)];
  __builtin_memcpy(ba, &a, sizeof(T));
  __builtin_memcpy(bb, &b, sizeof(T));
  for ( unsigned i = 0; i < sizeof(T); ++i )
    if ( ba[i] != bb[i] ) return false;
  return true;
}

static void
test_set_load_store()
{
  test_case("avx: set / splat / load / store");
  auto z = ma::zero_f32();
  alignas(32) float zb[8];
  ma::storeu_f32(zb, z);
  for ( int i = 0; i < 8; ++i ) require_true(zb[i] == 0.0f);

  auto v = ma::splat_f32(1.5f);
  ma::storeu_f32(zb, v);
  for ( int i = 0; i < 8; ++i ) require_true(zb[i] == 1.5f);

  alignas(32) double db[4] = { 1, 2, 3, 4 };
  alignas(32) double db2[4];
  auto vd = ma::load_f64(db);
  ma::store_f64(db2, vd);
  for ( int i = 0; i < 4; ++i ) require_true(db[i] == db2[i]);
  end_test_case();
}

static void
test_arith_fp()
{
  test_case("avx: fp arith / sqrt / min / max / hadd / addsub");
  auto a = ma::setr_f32(1, 2, 3, 4, 5, 6, 7, 8);
  auto b = ma::splat_f32(0.5f);
  alignas(32) float sb[8];
  ma::storeu_f32(sb, ma::add_f32(a, b));
  for ( int i = 0; i < 8; ++i ) require_true(sb[i] == float(i + 1) + 0.5f);

  ma::storeu_f32(sb, ma::mul_f32(a, ma::splat_f32(2.0f)));
  for ( int i = 0; i < 8; ++i ) require_true(sb[i] == float(i + 1) * 2.0f);

  ma::storeu_f32(sb, ma::sqrt_f32(ma::splat_f32(16.0f)));
  for ( int i = 0; i < 8; ++i ) require_true(sb[i] == 4.0f);

  // hadd: pairwise within 128-bit lanes.
  ma::storeu_f32(sb, ma::hadd_f32(a, b));
  // lane0 = a[0]+a[1] = 3, lane1 = a[2]+a[3] = 7, lane2 = b[0]+b[1] = 1.0,
  // lane3 = b[2]+b[3] = 1.0; then upper-half repeats for the upper 128 bits.
  require_true(sb[0] == 3.0f && sb[1] == 7.0f && sb[2] == 1.0f && sb[3] == 1.0f);

  // addsub: even lanes subtract, odd lanes add (intel docs).
  auto x = ma::splat_f32(10.0f);
  auto y = ma::splat_f32(3.0f);
  ma::storeu_f32(sb, ma::addsub_f32(x, y));
  require_true(sb[0] == 7.0f && sb[1] == 13.0f && sb[2] == 7.0f && sb[3] == 13.0f);
  end_test_case();
}

static void
test_bitwise_movemask()
{
  test_case("avx: bitwise / movemask");
  auto a = ma::splat_f32(-1.0f);
  auto b = ma::zero_f32();
  auto x = ma::xor_f32(a, b);
  require_true(v_eq(x, a));
  // movemask: top bits of each f32 = sign bit; -1.0f has sign bit set
  require_true(ma::movemask_f32(a) == 0xFF);
  require_true(ma::movemask_f32(b) == 0);

  auto fa = ma::setr_f32(1, -2, 3, -4, 5, -6, 7, -8);
  require_true(ma::movemask_f32(fa) == 0b10101010);
  end_test_case();
}

static void
test_convert_extract()
{
  test_case("avx: convert / extract");
  auto i = ma::cast_lo128_to_i256(_mm_setr_epi32(1, 2, 3, 4));
  // upper half is undefined when using cast (not zext). ignore upper.
  alignas(32) int ib[8] = {};
  ma::storeu_i256((__m256i_u *)ib, ma::zext_i256_from_lo128(_mm_setr_epi32(10, 20, 30, 40)));
  require_true(ib[0] == 10 && ib[3] == 40 && ib[4] == 0 && ib[7] == 0);
  (void)i;

  auto fi = ma::setr_f32(1.4f, 2.6f, -0.5f, 3.7f, 4.4f, 5.6f, 6.5f, 7.7f);
  auto trunc = ma::convert_trunc_f32_to_i32(fi);
  alignas(32) int tb[8];
  ma::storeu_i256((__m256i_u *)tb, trunc);
  require_true(tb[0] == 1 && tb[1] == 2 && tb[2] == 0 && tb[3] == 3);
  require_true(tb[4] == 4 && tb[5] == 5 && tb[6] == 6 && tb[7] == 7);

  auto cast128 = ma::cast_f32_to_lo128(ma::splat_f32(7.5f));
  alignas(16) float cb[4];
  _mm_storeu_ps(cb, cast128);
  require_true(cb[0] == 7.5f);
  end_test_case();
}

static void
test_broadcast()
{
  test_case("avx: broadcast / dup");
  float v = 9.25f;
  auto bv = ma::broadcast_f32_from_mem(&v);
  alignas(32) float sb[8];
  ma::storeu_f32(sb, bv);
  for ( int i = 0; i < 8; ++i ) require_true(sb[i] == 9.25f);

  auto a = ma::setr_f32(1, 2, 3, 4, 5, 6, 7, 8);
  auto hi = ma::movehdup_f32(a);
  auto lo = ma::moveldup_f32(a);
  ma::storeu_f32(sb, hi);
  require_true(sb[0] == 2 && sb[1] == 2 && sb[2] == 4 && sb[3] == 4);
  ma::storeu_f32(sb, lo);
  require_true(sb[0] == 1 && sb[1] == 1 && sb[2] == 3 && sb[3] == 3);
  end_test_case();
}

static void
test_test_family()
{
  test_case("avx: testz / testc / testnzc");
  auto z = ma::zero_i256();
  auto a = ma::splat_f32(-1.0f);
  require_true(ma::testz_i256(z, ma::cast_f32_to_i256(a)) == 1);
  auto zf = ma::zero_f32();
  require_true(ma::testz_f32(zf, zf) == 1);
  end_test_case();
}

int
main()
{
  print("=== TEST AVX ===");
  test_set_load_store();
  test_arith_fp();
  test_bitwise_movemask();
  test_convert_extract();
  test_broadcast();
  test_test_family();
  print("[TEST AVX OK]");
  return 0;
}
