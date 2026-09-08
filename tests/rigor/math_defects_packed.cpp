// math_defects_packed.cpp
// The packed transcendental defects in src/math/simd/: log's unreachable sqrt(1/2) re-centring and
// its missing domain guards, exp's clamp standing in for the overflow and NaN cases, the i32
// quadrant collapse and the missing Payne-Hanek stage in the packed pi/2 reducers, the two guards
// the packed CORDIC entry points dropped, armv7 packed sqrt of zero/inf/denormals, and the 128-bit
// fneg/copysign that existed on NEON and not on x86.
//
// References are the correctly-rounded double or float, baked as bit patterns. Inputs are laundered
// through a volatile so -Ofast cannot fold them, and non-finite results are read as bits rather than
// through x != x, which -ffast-math deletes.

#include "../../src/math/mk.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bit-level helpers

static f64
mk_d(u64 b)
{
  volatile u64 v = b;
  const u64 t = v;
  f64 r;
  __builtin_memcpy(&r, &t, sizeof r);
  return r;
}

static f32
mk_f(u32 b)
{
  volatile u32 v = b;
  const u32 t = v;
  f32 r;
  __builtin_memcpy(&r, &t, sizeof r);
  return r;
}

static u64
bits_d(f64 x)
{
  u64 b;
  __builtin_memcpy(&b, &x, sizeof b);
  return b;
}

static u32
bits_f(f32 x)
{
  u32 b;
  __builtin_memcpy(&b, &x, sizeof b);
  return b;
}

static bool
is_nan_d(f64 x)
{
  const u64 b = bits_d(x);
  return (b & 0x7FF0000000000000ull) == 0x7FF0000000000000ull && (b & 0x000FFFFFFFFFFFFFull) != 0;
}

static bool
is_nan_f(f32 x)
{
  const u32 b = bits_f(x);
  return (b & 0x7F800000u) == 0x7F800000u && (b & 0x007FFFFFu) != 0;
}

// order-preserving map, so a subtraction counts representable steps
static u64
mono_d(f64 x)
{
  const u64 b = bits_d(x);
  return (b & 0x8000000000000000ull) ? (0x8000000000000000ull - (b & 0x7FFFFFFFFFFFFFFFull)) : (b + 0x8000000000000000ull);
}

static u64
ulp_d(f64 a, f64 b)
{
  const u64 x = mono_d(a);
  const u64 y = mono_d(b);
  return x > y ? x - y : y - x;
}

static u32
mono_f(f32 x)
{
  const u32 b = bits_f(x);
  return (b & 0x80000000u) ? (0x80000000u - (b & 0x7FFFFFFFu)) : (b + 0x80000000u);
}

static u32
ulp_f(f32 a, f32 b)
{
  const u32 x = mono_f(a);
  const u32 y = mono_f(b);
  return x > y ? x - y : y - x;
}

static constexpr u64 D_PINF = 0x7FF0000000000000ull;
static constexpr u64 D_NINF = 0xFFF0000000000000ull;
static constexpr u64 D_MZERO = 0x8000000000000000ull;
static constexpr u64 D_ONE = 0x3FF0000000000000ull;
static constexpr u64 D_QNAN = 0x7FF8000000000001ull;
static constexpr u32 F_PINF = 0x7F800000u;
static constexpr u32 F_NINF = 0xFF800000u;
static constexpr u32 F_ONE = 0x3F800000u;
static constexpr u32 F_QNAN = 0x7FC00001u;

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

static void
st4(simd::d256 v, f64 (&out)[4])
{
  alignas(32) f64 tmp[4];
  _mm256_store_pd(reinterpret_cast<double *>(tmp), v);
  for ( int i = 0; i < 4; ++i ) out[i] = tmp[i];
}

static void
st8(simd::f256 v, f32 (&out)[8])
{
  alignas(32) f32 tmp[8];
  _mm256_store_ps(reinterpret_cast<float *>(tmp), v);
  for ( int i = 0; i < 8; ++i ) out[i] = tmp[i];
}

static simd::d256
ld4(const u64 (&b)[4])
{
  return _mm256_setr_pd(mk_d(b[0]), mk_d(b[1]), mk_d(b[2]), mk_d(b[3]));
}

int
main()
{
  f64 o[4];

  test_case("packed log — the mantissa split is sqrt(2), not sqrt(1/2)");
  {
    // m is rebuilt with the implicit bit forced on, so it lands in [1,2) and can never fall below
    // 0.7071; comparing against that left the fdlibm series running at s = f/(2+f) up to 1/3, and
    // the k*ln2 + log_m sum cancelling for x just under 1
    const u64 in[4] = { 0x3FEFFFF583A53B8Eull, 0x3FFFD70A3D70A3D7ull, 0x3FF8000000000000ull, 0x3FE8000000000000ull };
    const u64 want[4] = { 0xBED4F8B8F880BB63ull, 0x3FE60532EF13C385ull, 0x3FD9F323ECBF984Cull, 0xBFD269621134DB92ull };
    st4(mk::log_ns::log<simd::d256>(ld4(in)), o);
    for ( int i = 0; i < 4; ++i ) require_true(ulp_d(o[i], mk_d(want[i])) <= 2);
  }
  end_test_case();

  test_case("packed log — NaN, negative, zero and inf");
  {
    const u64 in[4] = { 0ull, 0xC000000000000000ull, D_PINF, D_QNAN };
    st4(mk::log_ns::log<simd::d256>(ld4(in)), o);
    require_true(bits_d(o[0]) == D_NINF);
    require_true(is_nan_d(o[1]));
    require_true(bits_d(o[2]) == D_PINF);
    require_true(is_nan_d(o[3]));

    const u64 in2[4] = { D_MZERO, D_NINF, D_ONE, 0x3FF8000000000000ull };
    st4(mk::log_ns::log<simd::d256>(ld4(in2)), o);
    require_true(bits_d(o[0]) == D_NINF);
    require_true(is_nan_d(o[1]));
    require_true(bits_d(o[2]) == 0ull);
    require_true(ulp_d(o[3], mk_d(0x3FD9F323ECBF984Cull)) <= 2);
  }
  end_test_case();

  test_case("packed log — subnormals are renormalised");
  {
    // a build with DAZ set (crtfastmath under -Ofast) cannot present a subnormal to the kernel
    const f64 tiny = mk_d(0x0000100000000000ull);
    volatile f64 probe = tiny;
    if ( probe * 2.0 != 0.0 ) {
      const u64 in[4] = { 0x0000100000000000ull, 0x0000000000000001ull, D_ONE, D_ONE };
      st4(mk::log_ns::log<simd::d256>(ld4(in)), o);
      require_true(ulp_d(o[0], mk_d(0xC0864F886378B146ull)) <= 2);
      require_true(ulp_d(o[1], mk_d(0xC0874385446D71C3ull)) <= 2);
    }
  }
  end_test_case();

  test_case("packed exp — overflow, underflow and NaN are cases, not a clamp");
  {
    const u64 in[4] = { D_PINF, D_NINF, D_QNAN, 0x40862E42FEFA39EFull };
    st4(mk::exp_ns::exp<simd::d256>(ld4(in)), o);
    require_true(bits_d(o[0]) == D_PINF);
    require_true(bits_d(o[1]) == 0ull);
    require_true(is_nan_d(o[2]));
    // the largest argument with a finite exp; the old clamp at 709.78 answered 0.27% low
    require_true(ulp_d(o[3], mk_d(0x7FEFFFFFFFFFFF2Aull)) <= 1);

    const u64 big[4] = { 0x4086396000000000ull, 0x7E37E43C8800759Cull, 0xC087500000000000ull, 0xC08F400000000000ull };
    st4(mk::exp_ns::exp<simd::d256>(ld4(big)), o);
    require_true(bits_d(o[0]) == D_PINF);
    require_true(bits_d(o[1]) == D_PINF);
    require_true(bits_d(o[2]) == 0ull);
    require_true(bits_d(o[3]) == 0ull);
  }
  end_test_case();

  test_case("packed exp — f256 lane path");
  {
    f32 of[8];
    const simd::f256 v
        = _mm256_setr_ps(mk_f(F_PINF), mk_f(F_NINF), mk_f(F_QNAN), 0.0f, 89.0f, -104.0f, 0.0f, 0.0f);
    st8(mk::exp_ns::exp<simd::f256>(v), of);
    require_true(bits_f(of[0]) == F_PINF);
    require_true(bits_f(of[1]) == 0u);
    require_true(is_nan_f(of[2]));
    require_true(bits_f(of[3]) == F_ONE);
    require_true(bits_f(of[4]) == F_PINF);
    require_true(bits_f(of[5]) == 0u);
  }
  end_test_case();

  test_case("packed sin/cos — the quadrant counter is not 32 bits wide");
  {
    // convert_f64_to_i32 answers INT_MIN for |fN| >= 2^31, i.e. |x| >= 2^31 * pi/2 = 3.3733e9, and
    // INT_MIN & 1 == INT_MIN & 2 == 0, so every lane collapsed onto the q == 0 branch
    const u64 in[4] = { 0x41E954FC40000000ull, 0x42374876E8000000ull, 0x41F2A05F20000000ull, 0x41E9211B00000000ull };
    const u64 ws[4] = { 0xBFB3FEF6FDB26531ull, 0x3FEDB7DBC47ABB83ull, 0x3FD01EB16F3E6C9Cull, 0x3FD7B052177D698Full };
    const u64 wc[4] = { 0xBFEFE6F8CCD3D2C9ull, 0x3FD7BBF860C90A11ull, 0xBFEEF7E43F02F408ull, 0x3FEDBA2E8D557E0Aull };
    st4(mk::trig::sin<simd::d256>(ld4(in)), o);
    for ( int i = 0; i < 4; ++i ) require_true(ulp_d(o[i], mk_d(ws[i])) <= 8);
    st4(mk::trig::cos<simd::d256>(ld4(in)), o);
    for ( int i = 0; i < 4; ++i ) require_true(ulp_d(o[i], mk_d(wc[i])) <= 8);
  }
  end_test_case();

  test_case("packed sin — a bounded function stays bounded");
  {
    // three Cody-Waite words run out of pi/2 at 2^33; with no fallback the residual kept growing
    // and ksin's r + r^3*p returned values far outside [-1,1], +/-inf among them
    const u64 in[4] = { 0x7E37E43C8800759Cull, 0x43B1200C7644D500ull, 0x430C6BF526340000ull, 0x5FB317E5EF3AB327ull };
    const u64 want[4] = { 0xBFEA2C16B010E385ull, 0x3FEB25904652297Cull, 0x3FEB76F88136CEBAull, 0xBFDB3D1150B651A2ull };
    st4(mk::trig::sin<simd::d256>(ld4(in)), o);
    for ( int i = 0; i < 4; ++i ) require_true(ulp_d(o[i], mk_d(want[i])) <= 65536);

    u64 s = 0x9E3779B97F4A7C15ull;
    for ( int k = 0; k < 4096; ++k ) {
      u64 v[4];
      for ( int i = 0; i < 4; ++i ) {
        s = s * 6364136223846793005ull + 1442695040888963407ull;
        v[i] = ((1023ull + (s >> 40) % 600ull) << 52) | (s & 0x000FFFFFFFFFFFFFull);
      }
      st4(mk::trig::sin<simd::d256>(ld4(v)), o);
      for ( int i = 0; i < 4; ++i ) require_true(o[i] <= 1.0 && o[i] >= -1.0);
      st4(mk::trig::cos<simd::d256>(ld4(v)), o);
      for ( int i = 0; i < 4; ++i ) require_true(o[i] <= 1.0 && o[i] >= -1.0);
    }
  }
  end_test_case();

  test_case("packed reducer — one out-of-range lane does not spoil the others");
  {
    const u64 in[4] = { 0x7E37E43C8800759Cull, 0x41E954FC40000000ull, 0x41F2A05F20000000ull, 0x41E9211B00000000ull };
    const u64 want[4] = { 0xBFEA2C16B010E385ull, 0xBFB3FEF6FDB26531ull, 0x3FD01EB16F3E6C9Cull, 0x3FD7B052177D698Full };
    st4(mk::trig::sin<simd::d256>(ld4(in)), o);
    for ( int i = 0; i < 4; ++i ) require_true(ulp_d(o[i], mk_d(want[i])) <= 65536);
  }
  end_test_case();

  test_case("packed CORDIC — the non-finite and small-angle guards");
  {
    // the rotation kernel's sign decision is z < 0, false for a NaN residual, so 53 unconditional
    // rotations from (K, 0) converged on one fixed point for every non-finite input, and could not
    // land exactly on (1, 0) for a true zero either
    const u64 in[4] = { 0ull, D_MZERO, 0x3BC79CA10C924223ull, D_PINF };
    st4(mk::cordic::sin<simd::d256>(ld4(in)), o);
    require_true(bits_d(o[0]) == 0ull);
    require_true(bits_d(o[1]) == D_MZERO);
    require_true(bits_d(o[2]) == 0x3BC79CA10C924223ull);
    require_true(is_nan_d(o[3]));
    st4(mk::cordic::cos<simd::d256>(ld4(in)), o);
    for ( int i = 0; i < 3; ++i ) require_true(bits_d(o[i]) == D_ONE);
    require_true(is_nan_d(o[3]));
    st4(mk::cordic::tan<simd::d256>(ld4(in)), o);
    require_true(bits_d(o[0]) == 0ull);
    require_true(bits_d(o[2]) == 0x3BC79CA10C924223ull);
    require_true(is_nan_d(o[3]));

    const u64 big[4] = { 0x7E37E43C8800759Cull, D_NINF, D_QNAN, 0x41E954FC40000000ull };
    st4(mk::cordic::sin<simd::d256>(ld4(big)), o);
    require_true(ulp_d(o[0], mk_d(0xBFEA2C16B010E385ull)) <= 65536);
    require_true(is_nan_d(o[1]));
    require_true(is_nan_d(o[2]));
    require_true(ulp_d(o[3], mk_d(0xBFB3FEF6FDB26531ull)) <= 16);
  }
  end_test_case();

  test_case("mk::fneg / mk::copysign exist at 128 bits on x86, as they do on NEON");
  {
    const simd::f128 vf = _mm_setr_ps(1.0f, -2.0f, 3.0f, -4.0f);
    const simd::f128 sf = _mm_setr_ps(-1.0f, 1.0f, -1.0f, 1.0f);
    alignas(16) f32 rf[4];
    _mm_store_ps(reinterpret_cast<float *>(rf), mk::fneg(vf));
    require_true(bits_f(rf[0]) == 0xBF800000u && bits_f(rf[1]) == 0x40000000u);
    _mm_store_ps(reinterpret_cast<float *>(rf), mk::copysign(vf, sf));
    require_true(bits_f(rf[0]) == 0xBF800000u && bits_f(rf[1]) == 0x40000000u);

    const simd::d128 vd = _mm_setr_pd(1.0, -2.0);
    const simd::d128 sd = _mm_setr_pd(-1.0, 1.0);
    alignas(16) f64 rd[2];
    _mm_store_pd(reinterpret_cast<double *>(rd), mk::fneg(vd));
    require_true(bits_d(rd[0]) == 0xBFF0000000000000ull && bits_d(rd[1]) == 0x4000000000000000ull);
    _mm_store_pd(reinterpret_cast<double *>(rd), mk::copysign(vd, sd));
    require_true(bits_d(rd[0]) == 0xBFF0000000000000ull && bits_d(rd[1]) == 0x4000000000000000ull);
  }
  end_test_case();

  print("=== packed transcendental defects ok ===");
  return 1;
}

#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

static void
st4f(simd::f128 v, f32 (&out)[4])
{
  simd::neon::store_f32(reinterpret_cast<float *>(out), v);
}

static simd::f128
ld4f(const u32 (&b)[4])
{
  const f32 t[4] = { mk_f(b[0]), mk_f(b[1]), mk_f(b[2]), mk_f(b[3]) };
  return simd::neon::load_f32(reinterpret_cast<const float *>(t));
}

int
main()
{
  f32 o[4];

  test_case("packed sqrt f128 — zero, inf and denormals");
  {
    // pre-ARMv8 NEON has no f32 sqrt, so sqrt(x) was x * rsqrt(x); VRSQRTE answers +inf for zero
    // and every denormal and +0 for +inf, which makes that product NaN or +inf
    const u32 in[4] = { 0u, 0x40800000u, F_PINF, 0x000116C2u };
    st4f(mk::pow_ns::sqrt<simd::f128>(ld4f(in)), o);
    require_true(bits_f(o[0]) == 0u);
    require_true(bits_f(o[1]) == 0x40000000u);
    require_true(bits_f(o[2]) == F_PINF);
    volatile f32 probe = mk_f(0x000116C2u);
    if ( probe * 2.0f != 0.0f ) require_true(ulp_f(o[3], mk_f(0x1E3CE4E7u)) <= 1);
  }
  end_test_case();

  test_case("packed log f128 — sqrt(2) split, NaN, negative, zero");
  {
    const u32 in[4] = { 0x3F7FFFACu, 0x3FFEB852u, 0u, 0xC0000000u };
    st4f(mk::log_ns::log<simd::f128>(ld4f(in)), o);
    require_true(ulp_f(o[0], mk_f(0xB6A8001Cu)) <= 2);
    require_true(ulp_f(o[1], mk_f(0x3F302998u)) <= 2);
    require_true(bits_f(o[2]) == F_NINF);
    require_true(is_nan_f(o[3]));

    const u32 in2[4] = { F_PINF, F_QNAN, F_ONE, 0x80000000u };
    st4f(mk::log_ns::log<simd::f128>(ld4f(in2)), o);
    require_true(bits_f(o[0]) == F_PINF);
    require_true(is_nan_f(o[1]));
    require_true(bits_f(o[2]) == 0u);
    require_true(bits_f(o[3]) == F_NINF);
  }
  end_test_case();

  test_case("packed exp f128 — overflow, underflow and NaN");
  {
    const u32 in[4] = { F_PINF, F_NINF, F_QNAN, 0u };
    st4f(mk::exp_ns::exp<simd::f128>(ld4f(in)), o);
    require_true(bits_f(o[0]) == F_PINF);
    require_true(bits_f(o[1]) == 0u);
    require_true(is_nan_f(o[2]));
    require_true(bits_f(o[3]) == F_ONE);
  }
  end_test_case();

  test_case("packed sin f128 — bounded, and the CORDIC guards");
  {
    const u32 in[4] = { 0x60AD78ECu, 0x4F6E6B28u, 0x58635FA9u, 0x4F4AA7E2u };      // 1e20, 4e9, 1e15, 3.4e9
    st4f(mk::trig::sin<simd::f128>(ld4f(in)), o);
    for ( int i = 0; i < 4; ++i ) require_true(!is_nan_f(o[i]) && o[i] <= 1.0f && o[i] >= -1.0f);
    require_true(ulp_f(o[1], mk_f(0x3F3D41EAu)) <= 64);

    const u32 c[4] = { 0u, 0x1E3CE508u, F_PINF, F_QNAN };
    st4f(mk::cordic::sin<simd::f128>(ld4f(c)), o);
    require_true(bits_f(o[0]) == 0u);
    require_true(bits_f(o[1]) == 0x1E3CE508u);
    require_true(is_nan_f(o[2]));
    require_true(is_nan_f(o[3]));
    st4f(mk::cordic::cos<simd::f128>(ld4f(c)), o);
    require_true(bits_f(o[0]) == F_ONE);
    require_true(bits_f(o[1]) == F_ONE);
    require_true(is_nan_f(o[2]));
  }
  end_test_case();

  print("=== packed transcendental defects ok ===");
  return 1;
}

#else

int
main()
{
  print("=== packed transcendental defects — skipped (no packed backend) ===");
  return 1;
}

#endif
