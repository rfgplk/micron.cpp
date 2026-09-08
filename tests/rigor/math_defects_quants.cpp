// math_defects_quants.cpp -- Snowball gates for the vector/tensor container defects in
// MATH_AUDIT (tensors.hpp:233 / :69 / :490 and v_types/vec3.hpp:242 / :374).
//
// Every case asserts the property that SHOULD hold, so the file FAILS on a tree carrying
// the defect.  Sections (b) and (c) are COMPILE gates: on the defective tree the
// brace-initialised tensor and the non-square bmm do not build at all, so a red cell there
// is the finding.  Sections (a), (d) and (e) run.
//
// Section (a) needs an element type wider than eight bytes to be observable: the surplus
// stores of the i += 8 loops land inside the class's own 64-byte tail padding for f32/f64,
// and only leave the object for a 16-byte one -- f128 on amd64 and aarch64.  armv7-a maps
// f128 to double, so there the case is a control.

#include "../../src/math/quants/tensors.hpp"
#include "../../src/math/quants/vecs.hpp"
#include "../../src/std.hpp"
#include "../../src/strings.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;

static constexpr u64 k_canary = 0xA5A5A5A5A5A5A5A5ull;
static constexpr u32 k_seed = 0x9E3779B9u;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (a) the four scalar compound-assign operators must not write past __size

template<u32 D, u32 H, u32 W> struct guarded {
  micron::tensor3ld<D, H, W> t;
  u64 wall[8];
};

template<u32 D, u32 H, u32 W>
static bool
compound_assign_stays_inside()
{
  guarded<D, H, W> g;
  for ( u32 i = 0; i < 8; ++i ) g.wall[i] = k_canary;
  for ( u32 i = 0; i < g.t.size(); ++i ) g.t[i] = static_cast<f128>(i);

  g.t += static_cast<f128>(1);
  g.t *= static_cast<f128>(2);
  g.t -= static_cast<f128>(2);
  g.t /= static_cast<f128>(2);

  for ( u32 i = 0; i < 8; ++i )
    if ( g.wall[i] != k_canary ) return false;
  for ( u32 i = 0; i < g.t.size(); ++i )
    if ( static_cast<f64>(g.t[i]) != static_cast<f64>(i) ) return false;
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (c) the parameter type already pins the contraction; a non-square left operand is legal

template<typename L, typename R, u32 W2>
concept bmm_callable = requires(const L &a, const R &b) { a.template bmm<W2>(b); };

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (e) angle/cos_angle must survive magnitudes squared_norm() itself can hold

static volatile f32 __opaque_f32 = 1.0f;
static volatile f64 __opaque_f64 = 1.0;

static f32
opaque(f32 x)
{
  return x * __opaque_f32;
}

static f64
opaque(f64 x)
{
  return x * __opaque_f64;
}

static bool
near_f32(f32 a, f32 b, f32 eps)
{
  const f32 d = a - b;
  return (d < 0 ? -d : d) <= eps;
}

static bool
near_f64(f64 a, f64 b, f64 eps)
{
  const f64 d = a - b;
  return (d < 0 ? -d : d) <= eps;
}

int
main()
{
  test_case("tensor scalar compound-assign writes nothing past __size");
  {
    // __size mod 8 in {1,2,3,4} is where round_up(__size,8) leaves the object
    require_true(compound_assign_stays_inside<1, 1, 1>());
    require_true(compound_assign_stays_inside<1, 1, 2>());
    require_true(compound_assign_stays_inside<1, 1, 3>());
    require_true(compound_assign_stays_inside<1, 3, 3>());
    require_true(compound_assign_stays_inside<1, 1, 12>());
    require_true(compound_assign_stays_inside<1, 4, 5>());
    // controls: shapes that were always inside
    require_true(compound_assign_stays_inside<1, 2, 4>());
    require_true(compound_assign_stays_inside<2, 2, 4>());

    micron::tensor3f<1, 3, 3> f;
    for ( u32 i = 0; i < f.size(); ++i ) f[i] = f32(i);
    f += 1.0f;
    for ( u32 i = 0; i < f.size(); ++i ) require_true(f[i] == f32(i) + 1.0f);
  }
  end_test_case();

  test_case("tensor brace-initialisation selects a constructor that compiles");
  {
    micron::tensor3f<1, 2, 2> t{ 1.0f, 2.0f, 3.0f, 4.0f };
    require_true(t[0] == 1.0f && t[1] == 2.0f && t[2] == 3.0f && t[3] == 4.0f);

    micron::tensor3d<1, 2, 3> d{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    for ( u32 i = 0; i < d.size(); ++i ) require_true(d[i] == f64(i + 1));

    // the variadic form is unchanged
    micron::tensor3f<1, 2, 2> p(1.0f, 2.0f, 3.0f, 4.0f);
    require_true(p[3] == 4.0f);
  }
  end_test_case();

  test_case("bmm accepts a non-square left operand");
  {
    require_true((bmm_callable<micron::tensor3f<2, 2, 3>, micron::tensor3f<2, 3, 4>, 4>));
    require_true((bmm_callable<micron::tensor3f<1, 4, 2>, micron::tensor3f<1, 2, 5>, 5>));
    // square control
    require_true((bmm_callable<micron::tensor3f<1, 3, 3>, micron::tensor3f<1, 3, 2>, 2>));

    micron::tensor3f<2, 2, 3> a;
    micron::tensor3f<2, 3, 4> b;
    u32 s = k_seed;
    for ( u32 i = 0; i < a.size(); ++i ) {
      s ^= s << 13;
      s ^= s >> 17;
      s ^= s << 5;
      a[i] = f32(i32(s % 17u) - 8);
    }
    for ( u32 i = 0; i < b.size(); ++i ) {
      s ^= s << 13;
      s ^= s >> 17;
      s ^= s << 5;
      b[i] = f32(i32(s % 17u) - 8);
    }
    auto r = a.bmm<4>(b);
    for ( u32 d = 0; d < 2; ++d )
      for ( u32 h = 0; h < 2; ++h )
        for ( u32 w = 0; w < 4; ++w ) {
          f32 acc = 0.0f;
          for ( u32 k = 0; k < 3; ++k ) acc += a[d * 6 + h * 3 + k] * b[d * 12 + k * 4 + w];
          require_true(r[d * 8 + h * 4 + w] == acc);
        }
  }
  end_test_case();

  test_case("linf_norm resolves and reduces to the largest magnitude");
  {
    // the defect this pins is a NAME LOOKUP failure that only shows outside amd64+GCC;
    // on the host it is a value control
    require_true(micron::vec2{ -3.0f, 2.0f }.linf_norm() == 3.0f);
    require_true(micron::vec3{ -3.0f, 7.0f, -2.0f }.linf_norm() == 7.0f);
    require_true(micron::vec4{ 1.0f, -9.0f, 2.0f, 3.0f }.linf_norm() == 9.0f);
    require_true(micron::vec8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -11.0f).linf_norm() == 11.0f);
    require_true(micron::dvec3{ -3.0, 7.0, -2.0 }.linf_norm() == 7.0);
    static_assert(micron::vector_3<f64>{ -3.0, 7.0, -2.0 }.linf_norm() == 7.0);
  }
  end_test_case();

  test_case("angle/cos_angle keep their working range at the magnitude scale");
  {
    const f32 big = opaque(1e10f);
    const micron::vec3 a{ big, 0.0f, 0.0f };
    const micron::vec3 b{ big, big, 0.0f };
    require_true(near_f32(a.angle(a), 0.0f, 1e-3f));
    require_true(near_f32(a.cos_angle(a), 1.0f, 1e-6f));
    require_true(near_f32(a.angle(b), 0.7853982f, 1e-5f));

    const micron::vec2 a2{ big, 0.0f }, b2{ big, big };
    require_true(near_f32(a2.angle(b2), 0.7853982f, 1e-5f));
    const micron::vec4 a4{ big, 0.0f, 0.0f, 0.0f }, b4{ big, big, 0.0f, 0.0f };
    require_true(near_f32(a4.angle(b4), 0.7853982f, 1e-5f));

    const f64 huge = opaque(1e100);
    const micron::dvec3 c{ huge, 0.0, 0.0 };
    const micron::dvec3 d{ huge, huge, 0.0 };
    require_true(near_f64(c.angle(d), 0.78539816339744831, 1e-12));
    require_true(near_f64(c.cos_angle(c), 1.0, 1e-14));

    // mixed magnitudes: |a||b| is fine, the fourth power is not
    const micron::vec3 lo{ opaque(1e-3f), 0.0f, 0.0f };
    const micron::vec3 hi{ 0.0f, opaque(1e12f), 0.0f };
    require_true(near_f32(lo.angle(hi), 1.5707964f, 1e-5f));

    // controls: unit scale is untouched, and so is the documented degenerate answer
    const micron::vec3 ux{ opaque(1.0f), 0.0f, 0.0f };
    const micron::vec3 uy{ 0.0f, opaque(1.0f), 0.0f };
    require_true(near_f32(ux.angle(uy), 1.5707964f, 1e-6f));
    require_true(near_f32(ux.cos_angle(ux), 1.0f, 1e-6f));
    require_true(micron::vec3{ opaque(1e-6f), 0.0f, 0.0f }.angle(uy) == 0.0f);
  }
  end_test_case();

  print("=== QUANTS DEFECT GATES PASSED ===");
  return 1;
}
