//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/math/matrix/mat.hpp"
#include "../src/std.hpp"

#if defined(__AVX2__) && defined(__FMA__)
#include "../src/simd/aliases.hpp"
#include "../src/simd/arch/types_amd64.hpp"
#endif

namespace
{

namespace ml = micron::math;

using mat4f = ml::mat<f32, 4, 4>;
using vec4f = ml::vec<f32, 4>;

constexpr u64 N = 256;
constexpr u64 REPS = 20000;
constexpr u32 K_MEAS = 7;

alignas(64) mat4f g_a[N];
alignas(64) mat4f g_b[N];
alignas(64) mat4f g_c[N];
alignas(64) vec4f g_v[N];
alignas(64) vec4f g_vout[N];

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

[[gnu::always_inline]] inline f32
lcg_unit(u64 &s) noexcept
{
  return static_cast<f32>(static_cast<f64>(lcg_next(s) >> 11) * 0x1.0p-53 - 0.5);
}

[[gnu::cold]] void
fill()
{
  u64 s = 0x1234567ULL;
  for ( u64 i = 0; i < N; ++i ) {
    for ( u64 j = 0; j < 16; ++j ) {
      g_a[i].data[j] = lcg_unit(s);
      g_b[i].data[j] = lcg_unit(s);
    }
    for ( u64 j = 0; j < 4; ++j ) g_v[i].data[j] = lcg_unit(s);
  }
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline u64
cycles_now() noexcept
{
  unsigned aux;
  return __builtin_ia32_rdtscp(&aux);
}

[[gnu::always_inline]] inline void
ref_gemm4(const mat4f &a, const mat4f &b, mat4f &out) noexcept
{
  for ( int i = 0; i < 4; ++i )
    for ( int j = 0; j < 4; ++j ) {
      f32 acc = 0;
      for ( int p = 0; p < 4; ++p ) acc = __builtin_fmaf(a.data[i * 4 + p], b.data[p * 4 + j], acc);
      out.data[i * 4 + j] = acc;
    }
}

[[gnu::always_inline]] inline void
ref_gemv4(const mat4f &m, const vec4f &v, vec4f &out) noexcept
{
  for ( int i = 0; i < 4; ++i ) {
    f32 acc = 0;
    for ( int j = 0; j < 4; ++j ) acc = __builtin_fmaf(m.data[i * 4 + j], v.data[j], acc);
    out.data[i] = acc;
  }
}

#if defined(__AVX2__) && defined(__FMA__)
[[gnu::always_inline]] inline void
sse_gemm4(const mat4f &a, const mat4f &b, mat4f &out) noexcept
{
  using namespace micron::simd;
  const float *ap = reinterpret_cast<const float *>(a.data);
  const float *bp = reinterpret_cast<const float *>(b.data);
  float *op = reinterpret_cast<float *>(out.data);
  const __m128 b0 = sse::loadu_f32(bp + 0);
  const __m128 b1 = sse::loadu_f32(bp + 4);
  const __m128 b2 = sse::loadu_f32(bp + 8);
  const __m128 b3 = sse::loadu_f32(bp + 12);
  for ( int i = 0; i < 4; ++i ) {
    const __m128 ar = sse::loadu_f32(ap + i * 4);
    __m128 r = sse::mul_f32(sse::shuffle_f32<0x00>(ar, ar), b0);
    r = fma::fma_f32(sse::shuffle_f32<0x55>(ar, ar), b1, r);
    r = fma::fma_f32(sse::shuffle_f32<0xAA>(ar, ar), b2, r);
    r = fma::fma_f32(sse::shuffle_f32<0xFF>(ar, ar), b3, r);
    sse::storeu_f32(op + i * 4, r);
  }
}

[[gnu::always_inline]] inline void
sse_gemv4(const mat4f &m, const vec4f &v, vec4f &out) noexcept
{
  using namespace micron::simd;
  const float *mp = reinterpret_cast<const float *>(m.data);
  const __m128 vv = sse::loadu_f32(reinterpret_cast<const float *>(v.data));
  const __m128 p0 = sse::mul_f32(sse::loadu_f32(mp + 0), vv);
  const __m128 p1 = sse::mul_f32(sse::loadu_f32(mp + 4), vv);
  const __m128 p2 = sse::mul_f32(sse::loadu_f32(mp + 8), vv);
  const __m128 p3 = sse::mul_f32(sse::loadu_f32(mp + 12), vv);
  const __m128 s01 = sse::hadd_f32(p0, p1);
  const __m128 s23 = sse::hadd_f32(p2, p3);
  const __m128 rr = sse::hadd_f32(s01, s23);
  sse::storeu_f32(reinterpret_cast<float *>(out.data), rr);
}
#endif

f64
median(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

enum class op { gemm_ref, gemm_sse, gemv_ref, gemv_sse };

template<op Op>
f64
measure() noexcept
{
  f64 samples[K_MEAS];
  for ( u32 k = 0; k < K_MEAS; ++k ) {
    asm volatile("" ::: "memory");
    const u64 t0 = cycles_now();
    for ( u64 r = 0; r < REPS; ++r )
      for ( u64 i = 0; i < N; ++i ) {
        if constexpr ( Op == op::gemm_ref ) {
          ref_gemm4(g_a[i], g_b[i], g_c[i]);
          clobber(&g_c[i]);
        } else if constexpr ( Op == op::gemv_ref ) {
          ref_gemv4(g_a[i], g_v[i], g_vout[i]);
          clobber(&g_vout[i]);
        }
#if defined(__AVX2__) && defined(__FMA__)
        else if constexpr ( Op == op::gemm_sse ) {
          sse_gemm4(g_a[i], g_b[i], g_c[i]);
          clobber(&g_c[i]);
        } else if constexpr ( Op == op::gemv_sse ) {
          sse_gemv4(g_a[i], g_v[i], g_vout[i]);
          clobber(&g_vout[i]);
        }
#endif
      }
    const u64 t1 = cycles_now();
    samples[k] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS * N);
  }
  return median(samples, K_MEAS);
}

[[gnu::cold]] void
report(const char *name, f64 cyc_per_op) noexcept
{
  const u64 x100 = static_cast<u64>(cyc_per_op * 100.0 + 0.5);
  micron::io::println(name, x100 / 100, ".", (x100 % 100) / 10, x100 % 10, " cyc/op");
}

f64
verify() noexcept
{
  f64 worst = 0;
#if defined(__AVX2__) && defined(__FMA__)
  for ( u64 i = 0; i < N; ++i ) {
    mat4f cr, cs;
    ref_gemm4(g_a[i], g_b[i], cr);
    sse_gemm4(g_a[i], g_b[i], cs);
    for ( int j = 0; j < 16; ++j ) {
      f64 d = static_cast<f64>(cr.data[j]) - static_cast<f64>(cs.data[j]);
      if ( d < 0 ) d = -d;
      if ( d > worst ) worst = d;
    }
    vec4f vr, vs;
    ref_gemv4(g_a[i], g_v[i], vr);
    sse_gemv4(g_a[i], g_v[i], vs);
    for ( int j = 0; j < 4; ++j ) {
      f64 d = static_cast<f64>(vr.data[j]) - static_cast<f64>(vs.data[j]);
      if ( d < 0 ) d = -d;
      if ( d > worst ) worst = d;
    }
  }
#endif
  return worst;
}

}      // namespace

int
main()
{
  fill();
  for ( u64 i = 0; i < N; ++i ) {
    ref_gemm4(g_a[i], g_b[i], g_c[i]);
    clobber(&g_c[i]);
  }

  const u64 vx = static_cast<u64>(verify() * 1e9 + 0.5);
  micron::io::println("correctness: max |ref - sse| = ", vx, " e-9  (small => SSE matches scalar)");

  micron::io::println("");
  micron::io::println("=== mat4*mat4 f32 ===");
  const f64 gm_ref = measure<op::gemm_ref>();
  report("  scalar (col-access)  : ", gm_ref);
#if defined(__AVX2__) && defined(__FMA__)
  const f64 gm_sse = measure<op::gemm_sse>();
  report("  hand SSE LinearComb  : ", gm_sse);
  micron::io::println("  speedup x100 = ", static_cast<u64>((gm_ref / gm_sse) * 100.0 + 0.5));
#endif

  micron::io::println("");
  micron::io::println("=== mat4*vec4 f32 ===");
  const f64 gv_ref = measure<op::gemv_ref>();
  report("  scalar               : ", gv_ref);
#if defined(__AVX2__) && defined(__FMA__)
  const f64 gv_sse = measure<op::gemv_sse>();
  report("  hand SSE (mul+hadd)  : ", gv_sse);
  micron::io::println("  speedup x100 = ", static_cast<u64>((gv_ref / gv_sse) * 100.0 + 0.5));
#endif
  return 0;
}
