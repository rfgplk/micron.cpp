// test_avx2.cpp
// Behavioral coverage for `micron::simd::avx2::*` (256-bit integer +
// variable shifts + cross-lane broadcasts + non-temporal load).

#include "../../src/simd/aliases/avx.hpp"
#include "../../src/simd/aliases/avx2.hpp"
#include "../snowball/snowball.hpp"

namespace ma2 = ::micron::simd::avx2;
namespace ma = ::micron::simd::avx;

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

template<typename T>
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
test_arith()
{
  test_case("avx2: int arith");
  auto a = ma::splat_i32(7);
  auto b = ma::splat_i32(3);
  alignas(32) int ib[8];
  ma::storeu_i256((__m256i_u *)ib, ma2::add_i32(a, b));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 10);

  ma::storeu_i256((__m256i_u *)ib, ma2::sub_i32(a, b));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 4);

  ma::storeu_i256((__m256i_u *)ib, ma2::mul_lo_i32(a, b));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 21);
  end_test_case();
}

static void
test_sat_avg_sad()
{
  test_case("avx2: saturating / avg / sad");
  auto sat_a = ma::splat_i8(120);
  auto sat_b = ma::splat_i8(50);
  alignas(32) signed char sb[32];
  ma::storeu_i256((__m256i_u *)sb, ma2::add_sat_i8(sat_a, sat_b));
  require_true(sb[0] == 127);

  auto avg = ma2::avg_u8(ma::splat_i8((char)0x40), ma::splat_i8((char)0x60));
  alignas(32) unsigned char ub[32];
  ma::storeu_i256((__m256i_u *)ub, avg);
  require_true(ub[0] == 0x50);

  auto sad = ma2::sad_u8(ma::splat_i8((char)0xA0), ma::splat_i8((char)0x10));
  alignas(32) unsigned long long sad_lanes[4];
  ma::storeu_i256((__m256i_u *)sad_lanes, sad);
  require_true(sad_lanes[0] == 0x90ull * 8);
  require_true(sad_lanes[2] == 0x90ull * 8);
  end_test_case();
}

static void
test_bitwise_cmp()
{
  test_case("avx2: bitwise / cmp");
  auto a = ma::splat_i32(int(0xCAFEBABEu));
  auto b = ma::splat_i32(int(0x55555555u));
  auto x = ma2::xor_i256(a, b);
  alignas(32) int ib[8];
  ma::storeu_i256((__m256i_u *)ib, x);
  require_true(ib[0] == int(0xCAFEBABEu ^ 0x55555555u));

  auto eq = ma2::eq_i32(a, a);
  ma::storeu_i256((__m256i_u *)ib, eq);
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == int(0xFFFFFFFFu));

  auto gt = ma2::gt_i32(a, ma::zero_i256());
  // a's lane interpretation: 0xCAFEBABE = signed -889275714, which is < 0
  ma::storeu_i256((__m256i_u *)ib, gt);
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 0);
  end_test_case();
}

static void
test_shifts()
{
  test_case("avx2: imm + per-lane shifts");
  auto a = ma::splat_i32(0x12345678);
  auto l = ma2::shl_i32(a, 4);
  auto r = ma2::shr_i32(a, 8);
  alignas(32) int ib[8];
  ma::storeu_i256((__m256i_u *)ib, l);
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 0x23456780);
  ma::storeu_i256((__m256i_u *)ib, r);
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 0x00123456);

  // per-lane shift counts.
  alignas(32) int counts[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  __m256i ncv = _mm256_loadu_si256((const __m256i *)counts);
  __m256i src = ma::splat_i32(1);
  ma::storeu_i256((__m256i_u *)ib, ma2::shl_per_i32(src, ncv));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == (1 << i));
  end_test_case();
}

static void
test_broadcast()
{
  test_case("avx2: broadcast i128 / lane");
  auto src = _mm_setr_epi32(1, 2, 3, 4);
  auto v = ma2::broadcast_i128_to_i256(src);
  alignas(32) int ib[8];
  ma::storeu_i256((__m256i_u *)ib, v);
  for ( int i = 0; i < 4; ++i ) require_true(ib[i] == ib[i + 4]);
  require_true(ib[0] == 1 && ib[3] == 4);

  auto bv32 = ma2::broadcast_i32_to_i256(_mm_setr_epi32(99, 0, 0, 0));
  ma::storeu_i256((__m256i_u *)ib, bv32);
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 99);
  end_test_case();
}

static void
test_minmax_abs()
{
  test_case("avx2: min / max / abs / sign");
  auto a = ma::splat_i32(7);
  auto b = ma::splat_i32(-3);
  alignas(32) int ib[8];
  ma::storeu_i256((__m256i_u *)ib, ma2::min_i32(a, b));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == -3);
  ma::storeu_i256((__m256i_u *)ib, ma2::max_i32(a, b));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 7);

  ma::storeu_i256((__m256i_u *)ib, ma2::abs_i32(b));
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == 3);
  ma::storeu_i256((__m256i_u *)ib, ma2::sign_i32(a, b));      // sign(b)*a = -7
  for ( int i = 0; i < 8; ++i ) require_true(ib[i] == -7);
  end_test_case();
}

static void
test_pack_unpack_movemask()
{
  test_case("avx2: pack / unpack / movemask");
  auto a = ma::splat_i16(0x40);
  auto b = ma::splat_i16(int(0x80));
  auto packed = ma2::pack_satu_i16(a, b);
  alignas(32) unsigned char ub[32];
  ma::storeu_i256((__m256i_u *)ub, packed);
  // packus_epi16 saturates each i16 to u8: 0x40 -> 0x40, 0x80 -> 0x80.
  // results interleave 128-bit lanes: 8 from a, 8 from b, 8 from a, 8 from b.
  require_true(ub[0] == 0x40 && ub[8] == 0x80 && ub[16] == 0x40 && ub[24] == 0x80);

  auto neg = ma::splat_i16(-1);
  require_true(ma2::movemask_i8(neg) == int(0xFFFFFFFFu));
  end_test_case();
}

int
main()
{
  print("=== TEST AVX2 ===");
  test_arith();
  test_sat_avg_sad();
  test_bitwise_cmp();
  test_shifts();
  test_broadcast();
  test_minmax_abs();
  test_pack_unpack_movemask();
  print("[TEST AVX2 OK]");
  return 1;
}
