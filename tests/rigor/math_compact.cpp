// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/blas/mixed.hpp"
#include "../../src/vector/vector.hpp"

#include "../../src/memory/allocation/abcmalloc/stats.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1";
}

using namespace micron::math;
namespace mx = micron::math::blas::mixed;
static_assert(sizeof(float16) == 2 && sizeof(bfloat16) == 2);
static_assert(!micron::is_constructible_v<float16, f64> && !micron::is_constructible_v<bfloat16, f64>);
static_assert(!mx::integer_accumulator<i8> && !ieee754_floating<float16>);
static_assert(micron::is_trivially_copyable_v<float16> && micron::is_trivially_copyable_v<bfloat16>);
static_assert(float16(f32(1)).bits == 0x3c00 && bfloat16(f32(1)).bits == 0x3f80);
static_assert(float16::from_bits(1).to_float() == f32(0x1p-24));
static_assert(bfloat16::from_bits(1).to_float() == f32(0x1p-133));
static_assert(mx::quantize_i8(f32(2.5), f32(1), 1) == 3);

// Arithmetic oracle deliberately does not share the conversion bit manipulations.
f64
reference(u16 word, usize mantissa, i32 bias)
{
  const u32 mask = (u32(1) << mantissa) - 1;
  const u32 exponent = (word & 0x7fff) >> mantissa;
  f64 value = f64(word & mask) / f64(mask + 1);
  if ( exponent ) value += f64(1);
  const i32 power = (exponent ? i32(exponent) : 1) - bias;
  for ( i32 i = 0; i < power; ++i ) value *= f64(2);
  for ( i32 i = 0; i > power; --i ) value *= f64(.5);
  return word & 0x8000 ? -value : value;
}

template<class H, usize Mantissa, i32 Bias>
void
conversions()
{
  constexpr u16 infinity = ((u16(1) << (15 - Mantissa)) - 1) << Mantissa;
  bool decode = true, roundtrip = true, midpoints = true, special = true;
  for ( u32 word = 0; word <= 65535; ++word ) {
    const H h = H::from_bits(u16(word));
    const f32 value = h.to_float();
    if ( (word & 0x7fff) < infinity ) {
      decode &= value == f32(reference(word, Mantissa, Bias)) && h.to_double() == reference(word, Mantissa, Bias);
      roundtrip &= H(value).bits == word;
    } else if ( (word & 0x7fff) == infinity ) {
      special &= ieee::is_inf(value) && ieee::sign_of(value) == i32(word >> 15) && H(value).bits == word;
    } else {
      special &= ieee::is_nan(value) && ieee::sign_of(value) == i32(word >> 15) && (ieee::to_bits(value) & 0x400000) != 0
                 && H(value).bits == ((word & 0x8000) | infinity | (1u << (Mantissa - 1)));
    }
  }
  for ( u32 word = 0; word + 1 < infinity; ++word ) {
    // FTZ/DAZ must not erase the oracle's subnormal BF16 midpoints before conversion.
    const f32 middle = Mantissa == 7 && word < 128 ? ieee::from_bits<f32>((word << 16) + 0x8000)
                                                   : f32((reference(word, Mantissa, Bias) + reference(word + 1, Mantissa, Bias)) * f64(.5));
    const u32 raw = ieee::to_bits(middle);
    const u16 even = u16(word + (word & 1));
    midpoints &= H(middle).bits == even && H(ieee::from_bits<f32>(raw ^ 0x80000000u)).bits == (even | 0x8000);
    midpoints &= H(ieee::from_bits<f32>(raw - 1)).bits == word && H(ieee::from_bits<f32>(raw + 1)).bits == word + 1;
  }
  sb::require_true(decode && roundtrip && midpoints && special);
  sb::require_true(H(ieee::from_bits<f32>(0x80000000)).bits == 0x8000);
  sb::require_true(H(ieee::from_bits<f32>(0x7f800001)).bits == (infinity | (1u << (Mantissa - 1))));
  sb::require_true(H(ieee::from_bits<f32>(0xff800001)).bits == (0x8000 | infinity | (1u << (Mantissa - 1))));
}

template<class H>
f64
decoded(H h)
{
  if constexpr ( micron::is_same_v<H, float16> )
    return reference(h.bits, 10, 15);
  else if constexpr ( micron::is_same_v<H, bfloat16> )
    return reference(h.bits, 7, 127);
  else
    return f64(h);
}

template<class A, class B, class Acc>
void
floating_gemm(bool ta, bool tb)
{
  constexpr usize m = 3, n = 7, k = 37;
  const usize lda = (ta ? m : k) + 2, ldb = (tb ? k : n) + 3;
  micron::vector<A, micron::allocator_serial<>, false> a((ta ? k : m) * lda);
  micron::vector<B, micron::allocator_serial<>, false> b((tb ? n : k) * ldb);
  Acc c[m * (n + 1)]{};
  for ( usize i = 0; i < a.size(); ++i ) a[i] = A(f32(i32(i % 29) - 14) / f32(13));
  for ( usize i = 0; i < b.size(); ++i ) b[i] = B(f32(i32(i % 17) - 8) / f32(19));
  const auto before = abc::stats();
  bool good = true;
  for ( usize repeat = 0; repeat < 8; ++repeat ) {
    for ( auto &v : c ) v = Acc(3);
    good &= mx::gemm_row(ta, tb, m, n, k, Acc(.5), a.data(), lda, b.data(), ldb, Acc(-.25), c, n + 1) == mx::status::ok;
  }
  const auto after = abc::stats();
#if MICRON_ABC_STATS
  sb::require_true(before.enabled && before.alloc_requests == after.alloc_requests && before.dealloc_requests == after.dealloc_requests);
#else
  (void)before;
  (void)after;
#endif
  for ( usize i = 0; i < m; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      f64 sum{};
      for ( usize z = 0; z < k; ++z ) sum += decoded(a[ta ? z * lda + i : i * lda + z]) * decoded(b[tb ? j * ldb + z : z * ldb + j]);
      const f64 difference = f64(c[i * (n + 1) + j]) - (sum * f64(.5) - f64(.75));
      good &= (difference < 0 ? -difference : difference) < (sizeof(Acc) == 4 ? f64(3e-6) : f64(3e-13));
    }
    good &= c[i * (n + 1) + n] == Acc(3);
  }
  sb::require_true(good);
}

template<class Acc>
void
integer_gemm(bool ta, bool tb)
{
  constexpr usize m = 3, n = 5, k = 41;
  const usize lda = (ta ? m : k) + 3, ldb = (tb ? k : n) + 2;
  micron::vector<i8, micron::allocator_serial<>, false> a((ta ? k : m) * lda), b((tb ? n : k) * ldb);
  Acc c[m * (n + 1)]{};
  for ( usize i = 0; i < a.size(); ++i ) a[i] = i8(i32(i % 256) - 128);
  for ( usize i = 0; i < b.size(); ++i ) b[i] = i8(i32((i * 17) % 256) - 128);
  sb::require_true(mx::gemm_i8_row(ta, tb, m, n, k, a.data(), lda, -13, b.data(), ldb, 27, c, n + 1) == mx::status::ok);
  bool exact = true;
  for ( usize i = 0; i < m; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      i64 expected{};
      for ( usize z = 0; z < k; ++z )
        expected += (i64(a[ta ? z * lda + i : i * lda + z]) + 13) * (i64(b[tb ? j * ldb + z : z * ldb + j]) - 27);
      exact &= c[i * (n + 1) + j] == expected;
    }
    exact &= c[i * (n + 1) + n] == 0;
  }
  sb::require_true(exact);
}

void
boundaries()
{
  constexpr usize k = mx::max_i8_terms<i32>;
  micron::vector<i8, micron::allocator_serial<>, false> a(k + 1, i8(-128));
  i32 result = 123;
  sb::require_true(mx::gemm_i8_row(false, true, 1, 1, k, a.data(), k, 127, a.data(), k, 127, &result, 1) == mx::status::ok);
  sb::require_true(i64(result) == i64(k) * 65025);
  result = 123;
  sb::require_true(mx::gemm_i8_row(false, true, 1, 1, k + 1, a.data(), k + 1, 127, a.data(), k + 1, 127, &result, 1)
                       == mx::status::accumulator_overflow
                   && result == 123);
  i64 wide{};
  sb::require_true(mx::gemm_i8_row(false, true, 1, 1, k + 1, a.data(), k + 1, 127, a.data(), k + 1, 127, &wide, 1) == mx::status::ok
                   && wide == i64(k + 1) * 65025);
  sb::require_true(mx::gemm_i8_row(false, true, 1, 1, 1, a.data(), 1, -129, a.data(), 1, 0, &result, 1) == mx::status::bad_zero_point
                   && result == 123);
  f32 out = ieee::qnan_v<f32>();
  sb::require_true(mx::gemm_row(false, false, 1, 1, 0, f32(1), static_cast<const f32 *>(nullptr), 0, static_cast<const float16 *>(nullptr),
                                0, f32(0), &out, 1)
                       == mx::status::ok
                   && out == 0);
  sb::require_true(float16(f32(65519)).bits == 0x7bff && float16(f32(65520)).bits == 0x7c00);
  sb::require_true(bfloat16(ieee::from_bits<f32>(0x7f7f7fff)).bits == 0x7f7f && bfloat16(ieee::from_bits<f32>(0x7f7f8000)).bits == 0x7f80);
  out = f32(17);
  sb::require_true(mx::gemm_row(false, false, 2, 1, 0, f32(1), static_cast<const f32 *>(nullptr), 0, static_cast<const float16 *>(nullptr),
                                0, f32(0), &out, usize(-1))
                       == mx::status::bad_dimension
                   && out == f32(17));
  sb::require_true(mx::gemm_row(false, false, 1, 1, 9, f32(0), static_cast<const f32 *>(nullptr), 0, static_cast<const float16 *>(nullptr),
                                0, f32(2), &out, 1)
                       == mx::status::ok
                   && out == f32(34));
  bfloat16 minimum = bfloat16::from_bits(1), one(f32(1));
  f64 widened{};
  sb::require_true(mx::gemm_row(false, false, 1, 1, 1, f64(1), &minimum, 1, &one, 1, f64(0), &widened, 1) == mx::status::ok
                   && widened == f64(0x1p-133));
  volatile u32 tiny32 = 1;
  volatile u64 tiny64 = 1;
  const f32 tiny_value32 = ieee::from_bits<f32>(tiny32 * 2), s32 = ieee::from_bits<f32>(tiny32);
  const f64 tiny_value64 = ieee::from_bits<f64>(tiny64 * 2), s64 = ieee::from_bits<f64>(tiny64);
  sb::require_true(mx::quantize_i8(tiny_value32, s32) == 2 && mx::quantize_i8(tiny_value64, s64) == 2);
  for ( i32 zero : { -128, -3, 0, 1, 127 } ) {
    sb::require_true(mx::quantize_i8(ieee::qnan_v<f32>(), f32(1), zero) == zero);
    sb::require_true(mx::quantize_i8(ieee::inf_v<f32>(), f32(1), zero) == 127);
    sb::require_true(mx::quantize_i8(ieee::inf_v<f32>(1), f32(1), zero) == -128);
  }
  sb::require_true(mx::quantize_i8(f64(2.5), f64(1)) == 2 && mx::quantize_i8(f64(-3.5), f64(1)) == -4);
}

int
main()
{
  sb::test_case("portable compact representations, RNE boundaries and mixed accumulators");
  conversions<float16, 10, 15>();
  conversions<bfloat16, 7, 127>();
  for ( bool ta : { false, true } )
    for ( bool tb : { false, true } ) {
      floating_gemm<float16, bfloat16, f32>(ta, tb);
      floating_gemm<float16, bfloat16, f64>(ta, tb);
      floating_gemm<f32, float16, f32>(ta, tb);
      floating_gemm<bfloat16, f32, f64>(ta, tb);
      integer_gemm<i32>(ta, tb);
      integer_gemm<i64>(ta, tb);
    }
  boundaries();
  sb::end_test_case();
  return 1;
}
