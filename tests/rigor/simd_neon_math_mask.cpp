//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/simd/math.hpp"
#include "../../src/simd/types.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using namespace micron;

static inline void
store_f32(f32 *dst, simd::f128 v) noexcept
{
  vst1q_f32(dst, v);
}

static inline simd::f128
load_f32(const f32 *src) noexcept
{
  return vld1q_f32(src);
}

int
main()
{
  print("=== NEON MATH MASK/ROUND TESTS ===");

  test_case("mask_reduce_ps — masked-out lane does not win");
  {

    f32 in[4] = { -5.f, 100.f, 3.f, 7.f };
    simd::f128 v = load_f32(in);
    require(simd::mask_reduce_max_ps(static_cast<u32>(0b1101), v), 7.f);
    require(simd::mask_reduce_min_ps(static_cast<u32>(0b1101), v), -5.f);
    require(simd::mask_reduce_max_ps(static_cast<u32>(0b1111), v), 100.f);
    require(simd::mask_reduce_max_ps(static_cast<u32>(0b0010), v), 100.f);

    float none = simd::mask_reduce_max_ps(static_cast<u32>(0), v);
    require_true(none < 0.f && none == none);
  }
  end_test_case();

  test_case("mask_reduce_i32 — masked-out lane does not win");
  {
    int32_t in[4] = { -5, 100, 3, 7 };
    simd::i128 v = vld1q_s32(in);
    require(simd::mask_reduce_max_i32(static_cast<u32>(0b1101), v), 7);
    require(simd::mask_reduce_min_i32(static_cast<u32>(0b1101), v), -5);
  }
  end_test_case();

  test_case("mask_reduce_i8 — 16-lane mask, lane>=8 reachable");
  {
    int8_t in[16] = { 50, 1, 2, 3, 4, 5, 6, 7, 120, 1, 2, 3, 4, 5, 6, 7 };
    simd::i128 v = vreinterpretq_s32_s8(vld1q_s8(in));

    require(static_cast<int>(static_cast<int8_t>(simd::mask_reduce_max_i8(static_cast<u32>(0xFEFFu), v))), 50);

    require(static_cast<int>(static_cast<int8_t>(simd::mask_reduce_max_i8(static_cast<u32>(1u << 8), v))), 120);

    int8_t inm[16] = { -50, 0, 0, 0, 0, 0, 0, 0, -120, 0, 0, 0, 0, 0, 0, 0 };
    simd::i128 vm = vreinterpretq_s32_s8(vld1q_s8(inm));
    require(static_cast<int>(static_cast<int8_t>(simd::mask_reduce_min_i8(static_cast<u32>(0xFEFFu), vm))), -50);
  }
  end_test_case();

  test_case("mask_reduce_u8 — 16-lane mask, lane>=8 reachable");
  {
    uint8_t in[16] = { 50, 1, 2, 3, 4, 5, 6, 7, 240, 1, 2, 3, 4, 5, 6, 7 };
    simd::i128 v = vreinterpretq_s32_u8(vld1q_u8(in));
    require(static_cast<int>(simd::mask_reduce_max_u8(static_cast<u32>(0xFEFFu), v)), 50);
    require(static_cast<int>(simd::mask_reduce_max_u8(static_cast<u32>(1u << 8), v)), 240);
    require(static_cast<int>(simd::mask_reduce_min_u8(static_cast<u32>(0xFEFFu), v)), 1);
  }
  end_test_case();

  test_case("mask_reduce_i16 — single active lane");
  {
    int16_t in[8] = { 50, 1, 2, 3, 4, 5, 6, 7 };
    simd::i128 v = vreinterpretq_s32_s16(vld1q_s16(in));
    require(static_cast<int>(simd::mask_reduce_max_i16(static_cast<u32>(0b00000001u), v)), 50);
  }
  end_test_case();

  test_case("min_i64 / max_i64");
  {
    int64_t a[2] = { -3, 9 };
    int64_t b[2] = { 5, 2 };
    simd::i128 va = vreinterpretq_s32_s64(vld1q_s64(a));
    simd::i128 vb = vreinterpretq_s32_s64(vld1q_s64(b));
    int64_t rmn[2], rmx[2];
    vst1q_s64(rmn, vreinterpretq_s64_s32(simd::min_i64(va, vb)));
    vst1q_s64(rmx, vreinterpretq_s64_s32(simd::max_i64(va, vb)));
    require_true(rmn[0] == -3 && rmn[1] == 2);
    require_true(rmx[0] == 5 && rmx[1] == 9);
  }
  end_test_case();

  test_case("round_ss case-0 is ties-to-even");
  {
    f32 ties[4] = { 2.5f, 3.5f, 0.5f, -2.5f };
    f32 expect[4] = { 2.f, 4.f, 0.f, -2.f };
    for ( int i = 0; i < 4; ++i ) {
      simd::f128 b = vdupq_n_f32(ties[i]);
      simd::f128 a = vdupq_n_f32(999.f);
      f32 out[4];
      store_f32(out, simd::round_ss(a, b, 0));
      require(out[0], expect[i]);
    }
    simd::f128 vties = load_f32(ties);
    simd::f128 vr = simd::round(vties, 0);
    f32 vout[4];
    store_f32(vout, vr);
    require_true(vout[0] == 2.f && vout[1] == 4.f && vout[2] == 0.f && vout[3] == -2.f);
  }
  end_test_case();

  test_case("transcendental delegation (cbrt / hypot)");
  {
    f32 cin[4] = { 1.f, 4.f, 9.f, 16.f };
    simd::f128 cv = load_f32(cin);
    f32 out[4];
    store_f32(out, simd::cbrt(cv));
    require_true(out[0] > 0.99f && out[0] < 1.01f);

    simd::f128 hb = vdupq_n_f32(4.f);
    simd::f128 ha = vdupq_n_f32(3.f);
    store_f32(out, simd::hypot(ha, hb));
    require_true(out[0] > 4.99f && out[0] < 5.01f);
  }
  end_test_case();

  print("=== NEON MATH MASK/ROUND TESTS PASSED ===");
  return 1;
}
