// math_geom_kernels.cpp
// Rigorous snowball suite for the graphics-math kernels: normalize (linalg
// vec<F,N> + quants vector_N<T>, exact and policy::fast tiers), cross, inv4,
// look_at. Seeded fuzz against f64/scalar oracles; the SIMD fast paths must
// agree with the scalar contract on every arch.

#include "../../src/math/geometry/projection.hpp"
#include "../../src/math/linalg.hpp"
#include "../../src/math/quants/vecs.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math::linalg;
namespace lops = micron::math::linalg::ops;
namespace geo = micron::math::geometry;

// layout contract the 16-byte loads/stores rely on
static_assert(sizeof(math::vec<f32, 3>) == 16);
static_assert(sizeof(math::vec<f32, 4>) == 16);
static_assert(sizeof(micron::vector_3<f32>) == 16);
static_assert(sizeof(micron::vector_4<f32>) == 16);

// consteval pins: scalar bodies stay constant-evaluable and exact
static_assert(micron::vector_3<f64>{ 3.0, 0.0, 0.0 }.normalized().x == 1.0);
static_assert(micron::vector_2<f64>{ 0.0, 4.0 }.normalized().y == 1.0);
constexpr math::vec<f64, 3> __cx = lops::cross<f64>(math::vec<f64, 3>{ 1.0, 0.0, 0.0 }, math::vec<f64, 3>{ 0.0, 1.0, 0.0 });
static_assert(__cx.data[2] == 1.0 && __cx.data[0] == 0.0);
constexpr math::mat<f64, 4, 4> __cinv = lops::inv4<f64>(math::mat<f64, 4, 4>::identity());
static_assert(__cinv.data[0] == 1.0 && __cinv.data[1] == 0.0 && __cinv.data[15] == 1.0);

static u64 g_seed = 0x9E3779B97F4A7C15ULL;

static u64
rnd_next(void)
{
  g_seed = g_seed * 6364136223846793005ULL + 1442695040888963407ULL;
  return g_seed;
}

static f64
rnd_unit(void)
{
  return static_cast<f64>(rnd_next() >> 11) * 0x1.0p-53;
}

static f64
rnd_sym(f64 scale = 1.0)
{
  return (rnd_unit() * 2.0 - 1.0) * scale;
}

static bool
near_abs(f64 a, f64 b, f64 eps)
{
  f64 d = a - b;
  if ( d < 0 ) d = -d;
  return d <= eps;
}

// NB: a plain v != v folds to false under -Ofast; math::isnan classifies bits
static bool
is_nan_f(f64 v)
{
  return micron::math::isnan(v);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// IEEE references for the __vsimd exact tier.
//
// these MUST stay noinline: gnu::optimize is discarded the moment a body is
// inlined into an -Ofast caller, which is exactly how the exact tier itself
// came to answer a reciprocal estimate (gcc's RECIP_MASK_DEFAULT rewrote
// divps into rcpps + newton). noinline is what keeps the attribute live.
[[gnu::noinline, gnu::optimize("no-fast-math", "no-reciprocal-math")]] static float
ieee_inv_sqrt_f(float n2)
{
  const float s = __builtin_sqrtf(n2);
  return float(1.0 / f64(s));
}

[[gnu::noinline, gnu::optimize("no-fast-math", "no-reciprocal-math")]] static f64
ieee_inv_sqrt_d(f64 n2)
{
  return 1.0 / __builtin_sqrt(n2);
}

[[gnu::noinline, gnu::optimize("no-fast-math", "no-reciprocal-math")]] static float
ieee_recip_f(float d)
{
  return 1.0f / d;
}

static u32
ulp_gap_f(float a, float b)
{
  u32 ia, ib;
  __builtin_memcpy(&ia, &a, 4);
  __builtin_memcpy(&ib, &b, 4);
  return ia > ib ? ia - ib : ib - ia;
}

int
main(void)
{
  test_case("normalize vec<f32,3/4> -- fuzz vs f64 oracle");
  {
    for ( int it = 0; it < 20000; ++it ) {
      // magnitudes swept across ~1e-3 .. 1e3
      const f64 mag = (it % 3 == 0) ? 1e-3 : ((it % 3 == 1) ? 1.0 : 1e3);
      const f64 a = rnd_sym(mag), b = rnd_sym(mag), c = rnd_sym(mag), d = rnd_sym(mag);
      const math::vec<f32, 3> v3{ f32(a), f32(b), f32(c) };
      const math::vec<f32, 4> v4{ f32(a), f32(b), f32(c), f32(d) };

      const f64 n3 = math::fsqrt(f64(v3.data[0]) * f64(v3.data[0]) + f64(v3.data[1]) * f64(v3.data[1])
                                 + f64(v3.data[2]) * f64(v3.data[2]));
      const auto r3 = lops::normalize<f32, 3>(v3);
      if ( n3 == 0.0 ) {
        require_true(r3.data[0] == v3.data[0] && r3.data[1] == v3.data[1] && r3.data[2] == v3.data[2]);
      } else {
        for ( int k = 0; k < 3; ++k ) require_true(near_abs(f64(r3.data[k]), f64(v3.data[k]) / n3, 1e-6));
        const f64 rn = f64(r3.data[0]) * f64(r3.data[0]) + f64(r3.data[1]) * f64(r3.data[1]) + f64(r3.data[2]) * f64(r3.data[2]);
        require_true(near_abs(rn, 1.0, 1e-5));
      }

      const f64 n4 = math::fsqrt(f64(v4.data[0]) * f64(v4.data[0]) + f64(v4.data[1]) * f64(v4.data[1])
                                 + f64(v4.data[2]) * f64(v4.data[2]) + f64(v4.data[3]) * f64(v4.data[3]));
      const auto r4 = lops::normalize<f32, 4>(v4);
      if ( n4 != 0.0 ) {
        for ( int k = 0; k < 4; ++k ) require_true(near_abs(f64(r4.data[k]), f64(v4.data[k]) / n4, 1e-6));
      }
    }
    // exact zero passes through unchanged
    const math::vec<f32, 3> z3{ 0.0f, 0.0f, 0.0f };
    const auto rz = lops::normalize<f32, 3>(z3);
    require_true(rz.data[0] == 0.0f && rz.data[1] == 0.0f && rz.data[2] == 0.0f);
    // NaN propagates
    const math::vec<f32, 3> nv{ __builtin_nanf(""), 1.0f, 2.0f };
    const auto rn = lops::normalize<f32, 3>(nv);
    require_true(is_nan_f(f64(rn.data[0])));
  }

  test_case("normalize vec<f64,3/4> -- guard-free exact tier");
  {
    for ( int it = 0; it < 20000; ++it ) {
      const math::vec<f64, 3> v{ rnd_sym(10.0), rnd_sym(10.0), rnd_sym(10.0) };
      const auto r = lops::normalize<f64, 3>(v);
      const f64 n = math::fsqrt(lops::dot(v, v));
      if ( n == 0.0 ) continue;
      for ( int k = 0; k < 3; ++k ) require_true(near_abs(r.data[k], v.data[k] / n, 1e-15));
      const f64 rn = lops::dot(r, r);
      require_true(near_abs(rn, 1.0, 1e-12));
    }
  }

  test_case("normalize policy::fast -- ~2^-22 tier");
  {
    for ( int it = 0; it < 20000; ++it ) {
      const f64 a = rnd_sym(100.0), b = rnd_sym(100.0), c = rnd_sym(100.0);
      const math::vec<f32, 3> v{ f32(a), f32(b), f32(c) };
      const f64 n = math::fsqrt(f64(v.data[0]) * f64(v.data[0]) + f64(v.data[1]) * f64(v.data[1])
                                + f64(v.data[2]) * f64(v.data[2]));
      if ( n == 0.0 ) continue;
      const auto r = lops::normalize(v, math::policy::fast);
      for ( int k = 0; k < 3; ++k ) require_true(near_abs(f64(r.data[k]), f64(v.data[k]) / n, 1e-4));
      const f64 rn = f64(r.data[0]) * f64(r.data[0]) + f64(r.data[1]) * f64(r.data[1]) + f64(r.data[2]) * f64(r.data[2]);
      require_true(near_abs(rn, 1.0, 5e-5));
      // f64 fast == exact tier (falls through)
      const math::vec<f64, 3> vd{ a, b, c };
      const auto rd = lops::normalize(vd, math::policy::fast);
      const auto re = lops::normalize<f64, 3>(vd);
      require_true(rd.data[0] == re.data[0] && rd.data[1] == re.data[1] && rd.data[2] == re.data[2]);
    }
  }

  test_case("vector_3/4<f32> normalized/normalize -- value + eps boundary + in-place");
  {
    for ( int it = 0; it < 20000; ++it ) {
      const f64 a = rnd_sym(50.0), b = rnd_sym(50.0), c = rnd_sym(50.0), d = rnd_sym(50.0);
      micron::vector_3<f32> v{ f32(a), f32(b), f32(c) };
      const f64 n2d = f64(v.x) * f64(v.x) + f64(v.y) * f64(v.y) + f64(v.z) * f64(v.z);
      const auto r = v.normalized();
      if ( n2d <= 1e-6 ) {
        require_true(r.x == 0.0f && r.y == 0.0f && r.z == 0.0f);
      } else {
        const f64 n = math::fsqrt(n2d);
        require_true(near_abs(f64(r.x), f64(v.x) / n, 1e-6) && near_abs(f64(r.y), f64(v.y) / n, 1e-6)
                     && near_abs(f64(r.z), f64(v.z) / n, 1e-6));
        // in-place agrees with the value form exactly
        micron::vector_3<f32> w = v;
        w.normalize();
        require_true(w.x == r.x && w.y == r.y && w.z == r.z);
        // fast tier within its looser bound
        const auto rf = v.normalized(math::policy::fast);
        require_true(near_abs(f64(rf.x), f64(v.x) / n, 1e-4) && near_abs(f64(rf.z), f64(v.z) / n, 1e-4));
      }

      micron::vector_4<f32> v4{ f32(a), f32(b), f32(c), f32(d) };
      const f64 m2 = f64(v4.x) * f64(v4.x) + f64(v4.y) * f64(v4.y) + f64(v4.z) * f64(v4.z) + f64(v4.w) * f64(v4.w);
      const auto r4 = v4.normalized();
      if ( m2 <= 1e-6 ) {
        require_true(r4.x == 0.0f && r4.w == 0.0f);
      } else {
        const f64 n = math::fsqrt(m2);
        require_true(near_abs(f64(r4.x), f64(v4.x) / n, 1e-6) && near_abs(f64(r4.w), f64(v4.w) / n, 1e-6));
        micron::vector_4<f32> w4 = v4;
        w4.normalize();
        require_true(w4.x == r4.x && w4.w == r4.w);
      }
    }
    // eps boundary: just below stays zero/unchanged, just above normalizes
    micron::vector_3<f32> lo{ 0.0009f, 0.0f, 0.0f };
    require_true(lo.normalized().x == 0.0f);
    micron::vector_3<f32> lo2{ 0.0009f, 0.0f, 0.0f };
    lo2.normalize();
    require_true(lo2.x == 0.0009f);
    micron::vector_3<f32> hi{ 0.0011f, 0.0f, 0.0f };
    require_true(near_abs(f64(hi.normalized().x), 1.0, 1e-6));
    // f64 vector_3 stays full precision
    micron::vector_3<f64> vd{ 3.0, 4.0, 12.0 };
    require_true(near_abs(vd.normalized().z, 12.0 / 13.0, 1e-15));
    // vector_2 exact + fast
    micron::vector_2<f32> v2{ 3.0f, 4.0f };
    require_true(near_abs(f64(v2.normalized().y), 0.8, 1e-6));
    require_true(near_abs(f64(v2.normalized(math::policy::fast).y), 0.8, 1e-4));
  }

  test_case("cross -- fuzz vs f64 oracle + basis");
  {
    for ( int it = 0; it < 20000; ++it ) {
      const f64 ax = rnd_sym(), ay = rnd_sym(), az = rnd_sym();
      const f64 bx = rnd_sym(), by = rnd_sym(), bz = rnd_sym();
      const math::vec<f32, 3> a{ f32(ax), f32(ay), f32(az) };
      const math::vec<f32, 3> b{ f32(bx), f32(by), f32(bz) };
      const auto c = lops::cross<f32>(a, b);
      const f64 ox = f64(a.data[1]) * f64(b.data[2]) - f64(a.data[2]) * f64(b.data[1]);
      const f64 oy = f64(a.data[2]) * f64(b.data[0]) - f64(a.data[0]) * f64(b.data[2]);
      const f64 oz = f64(a.data[0]) * f64(b.data[1]) - f64(a.data[1]) * f64(b.data[0]);
      require_true(near_abs(f64(c.data[0]), ox, 1e-6) && near_abs(f64(c.data[1]), oy, 1e-6)
                   && near_abs(f64(c.data[2]), oz, 1e-6));

      // member form agrees with the linalg form
      micron::vector_3<f32> ma{ f32(ax), f32(ay), f32(az) };
      micron::vector_3<f32> mb{ f32(bx), f32(by), f32(bz) };
      const auto mc_ = ma.cross(mb);
      require_true(near_abs(f64(mc_.x), ox, 1e-6) && near_abs(f64(mc_.y), oy, 1e-6) && near_abs(f64(mc_.z), oz, 1e-6));

      const math::vec<f64, 3> da{ ax, ay, az };
      const math::vec<f64, 3> db{ bx, by, bz };
      const auto dc = lops::cross<f64>(da, db);
      require_true(near_abs(dc.data[0], ay * bz - az * by, 1e-14));
    }
    const math::vec<f32, 3> e1{ 1.0f, 0.0f, 0.0f };
    const math::vec<f32, 3> e2{ 0.0f, 1.0f, 0.0f };
    const auto e3 = lops::cross<f32>(e1, e2);
    require_true(e3.data[0] == 0.0f && e3.data[1] == 0.0f && e3.data[2] == 1.0f);
  }

  test_case("inv4 -- conditioned fuzz, residual ||M*inv(M) - I||");
  {
    for ( int it = 0; it < 4000; ++it ) {
      math::mat<f64, 4, 4> md{};
      f64 det = 0.0;
      do {
        for ( usize k = 0; k < 16; ++k ) md.data[k] = rnd_sym();
        det = lops::det4<f64>(md);
      } while ( det < 0.1 && det > -0.1 );

      const auto mid = lops::inv4<f64>(md);
      const auto pd = lops::gemm<f64, 4, 4, 4>(md, mid);
      for ( usize r = 0; r < 4; ++r )
        for ( usize c = 0; c < 4; ++c ) require_true(near_abs(pd.data[r * 4 + c], (r == c) ? 1.0 : 0.0, 1e-9));

      math::mat<f32, 4, 4> mf{};
      for ( usize k = 0; k < 16; ++k ) mf.data[k] = f32(md.data[k]);
      const auto mif = lops::inv4<f32>(mf);
      const auto pf = lops::gemm<f32, 4, 4, 4>(mf, mif);
      for ( usize r = 0; r < 4; ++r )
        for ( usize c = 0; c < 4; ++c ) require_true(near_abs(f64(pf.data[r * 4 + c]), (r == c) ? 1.0 : 0.0, 1e-3));
      // f32 inverse tracks the f64 inverse
      for ( usize k = 0; k < 16; ++k ) require_true(near_abs(f64(mif.data[k]), mid.data[k], 1e-2 + 1e-3 * (mid.data[k] < 0 ? -mid.data[k] : mid.data[k])));
    }
  }

  test_case("look_at -- orthonormal rows + eye mapping, f32 vs f64");
  {
    for ( int it = 0; it < 4000; ++it ) {
      const f64 ex = rnd_sym(3.0), ey = rnd_sym(3.0), ez = rnd_sym(3.0);
      const f64 tx = rnd_sym(3.0), ty = rnd_sym(3.0), tz = rnd_sym(3.0);
      if ( near_abs(ex, tx, 1e-3) && near_abs(ey, ty, 1e-3) && near_abs(ez, tz, 1e-3) ) continue;
      const math::vec<f64, 3> eye_d{ ex, ey, ez };
      const math::vec<f64, 3> tgt_d{ tx, ty, tz };
      const math::vec<f64, 3> up_d{ rnd_sym(0.2), 1.0, rnd_sym(0.2) };

      const auto Ld = geo::look_at<geo::handedness::right, f64>(eye_d, tgt_d, up_d);
      // rotation rows orthonormal
      for ( usize r = 0; r < 3; ++r )
        for ( usize s = r; s < 3; ++s ) {
          f64 acc = 0.0;
          for ( usize k = 0; k < 3; ++k ) acc += Ld.M.data[r * 4 + k] * Ld.M.data[s * 4 + k];
          require_true(near_abs(acc, (r == s) ? 1.0 : 0.0, 1e-9));
        }
      // eye maps to the origin
      const math::vec<f64, 4> he{ ex, ey, ez, 1.0 };
      const auto me = lops::gemv<f64, 4, 4>(Ld.M, he);
      require_true(near_abs(me.data[0], 0.0, 1e-9) && near_abs(me.data[1], 0.0, 1e-9) && near_abs(me.data[2], 0.0, 1e-9)
                   && near_abs(me.data[3], 1.0, 1e-12));

      const math::vec<f32, 3> eye_f{ f32(ex), f32(ey), f32(ez) };
      const math::vec<f32, 3> tgt_f{ f32(tx), f32(ty), f32(tz) };
      const math::vec<f32, 3> up_f{ f32(up_d.data[0]), 1.0f, f32(up_d.data[2]) };
      const auto Lf = geo::look_at<geo::handedness::right, f32>(eye_f, tgt_f, up_f);
      for ( usize k = 0; k < 16; ++k ) require_true(near_abs(f64(Lf.M.data[k]), Ld.M.data[k], 2e-4));
      const auto Ll = geo::look_at<geo::handedness::left, f32>(eye_f, tgt_f, up_f);
      // LH forward row is the negated RH forward row
      require_true(near_abs(f64(Ll.M.data[8]), -f64(Lf.M.data[8]), 2e-4));
      // fast tier tracks the f64 reference within its looser bound
      const auto Lq = geo::look_at<geo::handedness::right, f32>(eye_f, tgt_f, up_f, math::policy::fast);
      for ( usize k = 0; k < 16; ++k ) require_true(near_abs(f64(Lq.M.data[k]), Ld.M.data[k], 1e-3));
      // f64 fast falls through to the exact tier (-Ofast may contract the two
      // inline contexts differently, so ulp-level equality is not pinnable)
      const auto Ldq = geo::look_at<geo::handedness::right, f64>(eye_d, tgt_d, up_d, math::policy::fast);
      for ( usize k = 0; k < 16; ++k ) require_true(near_abs(Ldq.M.data[k], Ld.M.data[k], 1e-12));
    }
    // degenerate: eye == target answers with fn == 0 guard (no NaN in rotation rows)
    const math::vec<f32, 3> p{ 1.0f, 2.0f, 3.0f };
    const auto Lz = geo::look_at<geo::handedness::right, f32>(p, p, math::vec<f32, 3>{ 0.0f, 1.0f, 0.0f });
    require_true(!is_nan_f(f64(Lz.M.data[0])));
  }

  test_case("__vsimd exact tier -- bit-exact IEEE divide, not a refined estimate");
  {
    // every kernel in __vec_simd.hpp is always_inline, so the file's own
    // no-fast-math pragma dies at the inline boundary and the caller's option
    // set governs the expansion. the near_abs tolerances above (1e-6) cannot
    // see the 1-2 ulp error an rcpps + newton rewrite leaves behind, so pin
    // the exact tier bit-for-bit here.
    for ( int it = 0; it < 200000; ++it ) {
      // positive normals swept over ~1e-9 .. 1e9
      const f64 mag = 1e-9 * f64(1ull << (it % 30));
      const float n2 = float(rnd_unit() * mag) + 1e-30f;
      if ( !(n2 > 0.0f) || micron::math::isinf(f64(n2)) ) continue;

      const float ref = ieee_inv_sqrt_f(n2);
      require_true(math::__vsimd::__inv_sqrt_exact_s(n2) == ref);

      const f64 d2 = rnd_unit() * mag + 1e-300;
      require_true(math::__vsimd::__inv_sqrt_exact_s(d2) == ieee_inv_sqrt_d(d2));

#if defined(__micron_gfx_simd)
      alignas(16) float in[4] = { n2, n2, n2, n2 };
      alignas(16) float out[4];
      const simd::f128 v = math::__vsimd::__load(in);
      math::__vsimd::__store(out, math::__vsimd::__inv_sqrt_exact(v));
      require_true(out[0] == ref && out[1] == ref && out[2] == ref && out[3] == ref);

      // __recip_signed is the inv4 1/det path -- exact by contract
      const float r1 = ieee_recip_f(n2);
      math::__vsimd::__store(out, math::__vsimd::__recip_signed(math::__vsimd::__setr(1.0f, -1.0f, -1.0f, 1.0f), v));
      require_true(out[0] == r1 && out[1] == -r1 && out[2] == -r1 && out[3] == r1);

      // the fast tier stays the cheap estimate: correct to ~2^-22, never
      // promoted to the exact form by whatever protects the exact one
      math::__vsimd::__store(out, math::__vsimd::__inv_sqrt_fast(v));
      for ( int k = 0; k < 4; ++k ) {
        const f64 rel = (f64(out[k]) - f64(ref)) / f64(ref);
        require_true((rel < 0 ? -rel : rel) <= 1e-6);
      }
      require_true(ulp_gap_f(math::__vsimd::__inv_sqrt_fast_s(n2), ref) < (1u << 8));
#endif
    }
  }

  print("all geom kernel cases passed\n");
  return 1;
}
