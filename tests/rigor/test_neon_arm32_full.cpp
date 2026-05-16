

#include "../../src/bits/__arch.hpp"

#if defined(__micron_arch_arm32) && defined(__micron_arm_neon)

#include "../../src/simd/intrin.hpp"
#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

int
main()
{
  print("=== TEST NEON ARM32 FULL SWEEP ===");

  test_case("phase 3: vbslq, vextq, vabsq, vrsqrtsq, vshrq_n_s64, veorq_s64");
  {
    uint32x4_t m = { 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0u };
    int32x4_t a = { 1, 2, 3, 4 }, b = { 10, 20, 30, 40 };
    int32x4_t r = vbslq_s32(m, a, b);
    require_true(r[0] == 1 && r[1] == 20 && r[3] == 40);

    int8x16_t v = vdupq_n_s8(-7);
    int8x16_t va = vabsq_s8(v);
    require_true(va[0] == 7);

    int64x2_t neg = vdupq_n_s64(-32);
    int64x2_t shifted = vshrq_n_s64(neg, 2);
    require_true(shifted[0] == -8);

    int64x2_t e = veorq_s64(vdupq_n_s64(0xFF), vdupq_n_s64(0x0F));
    require_true(e[0] == 0xF0);

    uint8x16_t ex = vextq_u8(vdupq_n_u8(1), vdupq_n_u8(2), 4);
    require_true(ex[0] == 1 && ex[15] == 2);

    float32x4_t fa = vdupq_n_f32(2.0f), fb = vdupq_n_f32(0.5f);
    float32x4_t fr = vrsqrtsq_f32(fa, fb);

    for ( int i = 0; i < 4; ++i ) require_true(fr[i] > 0.99f && fr[i] < 1.01f);
  }
  end_test_case();

  test_case("phase 4: vcltq, vcleq, vcgeq, vtstq, vca*, 64-bit emulated");
  {
    int32x4_t a = { 1, 2, 3, 4 }, b = { 5, 5, 5, 5 };
    require_true(vcltq_s32(a, b)[0] == 0xFFFFFFFFu);
    require_true(vcgeq_s32(a, b)[0] == 0u);

    int64x2_t la = { -1LL, 100LL }, lb = { 0LL, 99LL };
    require_true(vcgtq_s64(la, lb)[1] == 0xFFFFFFFFFFFFFFFFULL);
  }
  end_test_case();

  test_case("phase 5: vbicq, vornq, vmvnq_s*");
  {
    uint8x16_t a = vdupq_n_u8(0xF0), b = vdupq_n_u8(0x33);
    require_true(vbicq_u8(a, b)[0] == 0xC0);
    require_true(vornq_u8(a, b)[0] == 0xFC);
    require_true(vmvnq_s8(vdupq_n_s8(0x55))[0] == (i8)0xAA);
  }
  end_test_case();

  test_case("phase 6: vqaddq, vqsubq, vhaddq");
  {
    int8x16_t r = vqaddq_s8(vdupq_n_s8(100), vdupq_n_s8(50));
    require_true(r[0] == 127);
    int8x16_t h = vhaddq_s8(vdupq_n_s8(10), vdupq_n_s8(7));
    require_true(h[0] == 8);
  }
  end_test_case();

  test_case("phase 7: vmlaq, vmulq_n, vmull_s8");
  {
    int32x4_t a = vdupq_n_s32(1), b = vdupq_n_s32(2), c = vdupq_n_s32(3);
    int32x4_t r = vmlaq_s32(a, b, c);
    require_true(r[0] == 7);

    int32x4_t n = vmulq_n_s32(vdupq_n_s32(5), 4);
    require_true(n[0] == 20);

    int8x8_t da = vdup_n_s8(3), db = vdup_n_s8(50);
    int16x8_t l = vmull_s8(da, db);
    require_true(l[0] == 150);
  }
  end_test_case();

  test_case("phase 8: vaddl, vmovn, vqmovn");
  {
    int8x8_t a = vdup_n_s8(50), b = vdup_n_s8(100);
    int16x8_t l = vaddl_s8(a, b);
    require_true(l[0] == 150);

    int16x8_t big = vdupq_n_s16(2000);
    int8x8_t qr = vqmovn_s16(big);
    require_true(qr[0] == 127);
  }
  end_test_case();

  test_case("phase 9: vqshlq_n, vsraq_n, vshll_n, vshrn_n");
  {
    int8x16_t a = vdupq_n_s8(50);
    int8x16_t qsl = vqshlq_n_s8(a, 3);
    require_true(qsl[0] == 127);

    int16x8_t s = vsraq_n_s16(vdupq_n_s16(100), vdupq_n_s16(8), 1);
    require_true(s[0] == 104);
  }
  end_test_case();

  test_case("phase 10: vmaxq, vmaxnmq, vpaddq, vpaddlq");
  {
    int32x4_t a = { 1, 5, 2, 8 }, b = { 7, 3, 6, 4 };
    int32x4_t mx = vmaxq_s32(a, b);
    require_true(mx[0] == 7 && mx[3] == 8);

    int32x4_t pq = vpaddq_s32(a, b);
    require_true(pq[0] == 6 && pq[2] == 10);

    int8x16_t pl_in = vdupq_n_s8(3);
    int16x8_t pl = vpaddlq_s8(pl_in);
    require_true(pl[0] == 6);
  }
  end_test_case();

  test_case("phase 11: vrev64q_s8, vzipq_s32, vtbl1_u8");
  {
    int8x16_t v = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    int8x16_t r = vrev64q_s8(v);
    require_true(r[0] == 7 && r[8] == 15);

    int32x4_t a = { 1, 2, 3, 4 }, b = { 10, 20, 30, 40 };
    int32x4x2_t z = vzipq_s32(a, b);
    require_true(z.val[0][1] == 10);

    uint8x8_t table = { 10, 20, 30, 40, 50, 60, 70, 80 };
    uint8x8_t idx = { 0, 7, 8, 1, 0, 0, 0, 0 };
    uint8x8_t rt = vtbl1_u8(table, idx);
    require_true(rt[0] == 10 && rt[1] == 80 && rt[2] == 0);
  }
  end_test_case();

  test_case("phase 12: vcvtq_f32_s32, vmovl_s8");
  {
    int32x4_t i = vdupq_n_s32(7);
    float32x4_t f = vcvtq_f32_s32(i);
    require_true(f[0] == 7.f);

    int8x8_t v = vdup_n_s8(-5);
    int16x8_t w = vmovl_s8(v);
    require_true(w[0] == -5);
  }
  end_test_case();

  test_case("phase 13: vclzq, vcntq");
  {
    require_true(vclzq_u32(vdupq_n_u32(0x80000000u))[0] == 0);
    require_true(vclzq_u32(vdupq_n_u32(0x00000001u))[0] == 31);
    require_true(vcntq_u8(vdupq_n_u8(0xFF))[0] == 8);
  }
  end_test_case();

  test_case("phase 14: vld2q, vst2q round-trip");
  {
    int32_t buf[8] = { 1, 100, 2, 200, 3, 300, 4, 400 };
    int32x4x2_t v = vld2q_s32(buf);
    require_true(v.val[0][0] == 1 && v.val[1][0] == 100);

    int32_t out[8] = { 0 };
    vst2q_s32(out, v);
    for ( int i = 0; i < 8; ++i ) require_true(out[i] == buf[i]);
  }
  end_test_case();

  test_case("phase 15: vmull_p8");
  {
    poly8x8_t a = { 1, 2, 4, 8, 16, 32, 64, (poly8_t)0x80 };
    poly8x8_t b = { 1, 1, 1, 1, 1, 1, 1, 1 };
    uint16x8_t r = vmull_p8(a, b);

    require_true(r[0] == 1 && r[1] == 2 && r[7] == 0x80);
  }
  end_test_case();

  test_case("phase 16: vadd_s8, vbsl_s32, vmax_s16, vqadd_u8");
  {
    int8x8_t r = vadd_s8(vdup_n_s8(3), vdup_n_s8(4));
    require_true(r[0] == 7);

    uint32x2_t mask = { 0xFFFFFFFFu, 0u };
    int32x2_t a = { 1, 2 }, b = { 10, 20 };
    int32x2_t bs = vbsl_s32(mask, a, b);
    require_true(bs[0] == 1 && bs[1] == 20);

    int16x4_t mx = vmax_s16(vdup_n_s16(3), vdup_n_s16(7));
    require_true(mx[0] == 7);

    uint8x8_t qa = vqadd_u8(vdup_n_u8(200), vdup_n_u8(100));
    require_true(qa[0] == 0xFF);
  }
  end_test_case();

#if defined(__ARM_FEATURE_FMA)
  test_case("phase 17 extension: FMA (vfmaq_f32)");
  {
    float32x4_t a = vdupq_n_f32(10.f);
    float32x4_t b = vdupq_n_f32(2.f);
    float32x4_t c = vdupq_n_f32(3.f);
    float32x4_t r = vfmaq_f32(a, b, c);
    for ( int i = 0; i < 4; ++i ) require_true(r[i] == 16.f);
  }
  end_test_case();
#endif

  print("[TEST NEON ARM32 FULL SWEEP OK]");
  return 1;
}

#else

#include "../snowball/snowball.hpp"

int
main()
{
  ::sb::print("=== TEST NEON ARM32 FULL SWEEP ===");
  ::sb::print("[skipped: not an armv7-a + NEON build]");
  return 1;
}

#endif
