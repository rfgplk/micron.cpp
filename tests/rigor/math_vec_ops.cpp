// math_vec_ops.cpp -- componentwise * / *= /= across the quants vector family
// (vector_2/3/4/8/16) and micron::math::vec<T,N>.
//
// Every operand is bound as a CONST lvalue on purpose: operator/(vector,vector)
// used to be the one binary operator in these classes without a trailing const,
// so `const v3 a, b; a / b;` failed to compile while `a * b` succeeded.

#include "../../src/math/generic.hpp"
#include "../../src/math/quants/vec.hpp"
#include "../../src/math/quants/vecs.hpp"

#include "../../src/type_traits.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace math = micron::math;

namespace
{

bool
near(f64 a, f64 b, f64 e)
{
  const f64 d = a - b;
  return (d < 0 ? -d : d) <= e;
}

u64 g_seed = 0xA5A5F00D1234BEEFULL;

u64
rnd_next(void)
{
  g_seed = g_seed * 6364136223846793005ULL + 1442695040888963407ULL;
  return g_seed;
}

// magnitude in [1, 3]; keeps the divide oracle conditioned and never hits zero
f64
rnd_nz(void)
{
  const f64 u = static_cast<f64>(rnd_next() >> 11) * 0x1.0p-53;
  const f64 v = u * 2.0 + 1.0;
  return (rnd_next() & 1u) ? v : -v;
}

#define F2(m) m(x) m(y)
#define F3(m) m(x) m(y) m(z)
#define F4(m) m(x) m(y) m(z) m(w)
#define F8(m) m(x) m(y) m(z) m(w) m(a) m(b) m(c) m(d)
#define F16(m) m(x) m(y) m(z) m(w) m(a) m(b) m(c) m(d) m(e) m(f) m(g) m(h) m(i) m(j) m(k) m(l)

#define SET_A(fl) ma.fl = static_cast<T>(rnd_nz());
#define SET_B(fl) mb.fl = static_cast<T>(rnd_nz());
#define CK_MUL(fl) ok = ok && near(static_cast<f64>(p.fl), static_cast<f64>(a.fl) * static_cast<f64>(b.fl), tol);
#define CK_DIV(fl) ok = ok && near(static_cast<f64>(q.fl), static_cast<f64>(a.fl) / static_cast<f64>(b.fl), tol);
#define CK_PRD(fl) ok = ok && near(static_cast<f64>(pr.fl), static_cast<f64>(p.fl), tol);
#define CK_QUO(fl) ok = ok && near(static_cast<f64>(qr.fl), static_cast<f64>(q.fl), tol);
#define CK_MEQ(fl) ok = ok && near(static_cast<f64>(m1.fl), static_cast<f64>(p.fl), tol);
#define CK_DEQ(fl) ok = ok && near(static_cast<f64>(m2.fl), static_cast<f64>(q.fl), tol);

#define VEC_SUITE(NAME, TYPE, FIELDS)                                                                                                      \
  template<typename T> bool NAME(int iters, f64 tol)                                                                                       \
  {                                                                                                                                        \
    bool ok = true;                                                                                                                        \
    for ( int t = 0; t < iters && ok; ++t ) {                                                                                              \
      micron::TYPE<T> ma{}, mb{};                                                                                                          \
      FIELDS(SET_A)                                                                                                                        \
      FIELDS(SET_B)                                                                                                                        \
      const micron::TYPE<T> a = ma, b = mb;                                                                                                \
      const micron::TYPE<T> p = a * b;                                                                                                     \
      const micron::TYPE<T> q = a / b;                                                                                                     \
      const micron::TYPE<T> pr = a.product(b);                                                                                             \
      const micron::TYPE<T> qr = a.quotient(b);                                                                                            \
      micron::TYPE<T> m1 = ma;                                                                                                             \
      micron::TYPE<T> m2 = ma;                                                                                                             \
      m1 *= b;                                                                                                                             \
      m2 /= b;                                                                                                                             \
      FIELDS(CK_MUL)                                                                                                                       \
      FIELDS(CK_DIV)                                                                                                                       \
      FIELDS(CK_PRD)                                                                                                                       \
      FIELDS(CK_QUO)                                                                                                                       \
      FIELDS(CK_MEQ)                                                                                                                       \
      FIELDS(CK_DEQ)                                                                                                                       \
    }                                                                                                                                      \
    return ok;                                                                                                                             \
  }

VEC_SUITE(run2, vector_2, F2)
VEC_SUITE(run3, vector_3, F3)
VEC_SUITE(run4, vector_4, F4)
VEC_SUITE(run8, vector_8, F8)
VEC_SUITE(run16, vector_16, F16)

// micron::math::vec<T,N> -- array-backed, free operators
template<typename T, usize N>
bool
run_mvec(int iters, f64 tol)
{
  bool ok = true;
  for ( int t = 0; t < iters && ok; ++t ) {
    micron::math::vec<T, N> ma{}, mb{};
    for ( usize i = 0; i < N; ++i ) {
      ma.data[i] = static_cast<T>(rnd_nz());
      mb.data[i] = static_cast<T>(rnd_nz());
    }
    const micron::math::vec<T, N> a = ma, b = mb;
    const micron::math::vec<T, N> p = a * b;
    const micron::math::vec<T, N> q = a / b;
    const micron::math::vec<T, N> sq = static_cast<T>(2) / b;
    micron::math::vec<T, N> m1 = ma, m2 = ma;
    m1 *= b;
    m2 /= b;
    for ( usize i = 0; i < N; ++i ) {
      const f64 av = static_cast<f64>(a.data[i]), bv = static_cast<f64>(b.data[i]);
      ok = ok && near(static_cast<f64>(p.data[i]), av * bv, tol);
      ok = ok && near(static_cast<f64>(q.data[i]), av / bv, tol);
      ok = ok && near(static_cast<f64>(sq.data[i]), 2.0 / bv, tol);
      ok = ok && near(static_cast<f64>(m1.data[i]), static_cast<f64>(p.data[i]), tol);
      ok = ok && near(static_cast<f64>(m2.data[i]), static_cast<f64>(q.data[i]), tol);
    }
  }
  return ok;
}

// math::ffma used to be declared only for double, so an f32 call widened, fused at double
// width and rounded back -- and where f32 is _Float32 the braced `{ ffma(...), ... }` return
// in vector_N::fma was outright ambiguous. it must now fuse at the argument's own width.
static_assert(micron::is_same_v<decltype(math::ffma(f32{ 1 }, f32{ 2 }, f32{ 3 })), f32>);
static_assert(micron::is_same_v<decltype(math::ffma(f64{ 1 }, f64{ 2 }, f64{ 3 })), f64>);
static_assert(micron::is_same_v<decltype(math::ffma(1.0L, 2.0L, 3.0L)), long double>);
// a non-floating argument still falls back to the double overload, exactly as before
static_assert(micron::is_same_v<decltype(math::ffma(1, 2, 3)), double>);

// compile-time pins: these are the exact expressions that failed to compile before
static_assert(micron::vector_3<f64>{ 6.0, 8.0, 10.0 }.quotient(micron::vector_3<f64>{ 2.0, 4.0, 5.0 }).x == 3.0);
static_assert((micron::vector_2<f64>{ 9.0, 4.0 } / micron::vector_2<f64>{ 3.0, 2.0 }).y == 2.0);
static_assert((micron::vector_4<f64>{ 8.0, 6.0, 4.0, 2.0 } / micron::vector_4<f64>{ 4.0, 3.0, 2.0, 1.0 }).w == 2.0);
constexpr micron::vector_16<f64> __c16a{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
constexpr micron::vector_16<f64> __c16b{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static_assert((__c16a / __c16b).l == 2.0);

};      // namespace

int
main()
{
  print("=== VECTOR COMPONENTWISE OPS ===");

  test_case("vector_2/3/4 componentwise * / *= /= against a scalar oracle, const operands");
  {
    require_true(run2<f32>(2000, 1e-5) && run2<f64>(2000, 1e-12));
    require_true(run3<f32>(2000, 1e-5) && run3<f64>(2000, 1e-12));
    require_true(run4<f32>(2000, 1e-5) && run4<f64>(2000, 1e-12));
  }
  end_test_case();

  test_case("vector_8/16 componentwise * / *= /= against a scalar oracle, const operands");
  {
    require_true(run8<f32>(1000, 1e-5) && run8<f64>(1000, 1e-12));
    require_true(run16<f32>(1000, 1e-5) && run16<f64>(1000, 1e-12));
  }
  end_test_case();

  test_case("math::vec<T,N> componentwise * /, scalar/vector, and /=");
  {
    require_true(run_mvec<f32, 3>(1000, 1e-5) && run_mvec<f64, 3>(1000, 1e-12));
    require_true(run_mvec<f32, 4>(1000, 1e-5) && run_mvec<f64, 4>(1000, 1e-12));
    require_true(run_mvec<f64, 8>(1000, 1e-12));
  }
  end_test_case();

  test_case("a / b == a * (1/b) componentwise -- algebraic invariant");
  {
    const micron::vector_3<f64> a{ 3.0, -7.0, 11.0 }, b{ 2.0, 4.0, -8.0 };
    const micron::vector_3<f64> inv{ 1.0 / b.x, 1.0 / b.y, 1.0 / b.z };
    const micron::vector_3<f64> q = a / b, m = a * inv;
    require_true(near(q.x, m.x, 1e-15) && near(q.y, m.y, 1e-15) && near(q.z, m.z, 1e-15));
  }
  end_test_case();

  test_case("ffma fuses at the argument's own width (f64 oracle for the f32 case)");
  {
    // an f32 product is exact in f64 (24 + 24 <= 53 bits), so rounding the f64 expression
    // once to f32 IS the correctly-rounded fused result -- a true oracle, not an epsilon.
    bool ok = true;
    int differs_from_naive = 0;
    for ( int t = 0; t < 20000 && ok; ++t ) {
      const f32 a = static_cast<f32>(rnd_nz()), b = static_cast<f32>(rnd_nz()), c = static_cast<f32>(rnd_nz());
      const f32 got = math::ffma(a, b, c);
      const f32 ref = static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b) + static_cast<f64>(c));
      ok = ok && (got == ref);
      // volatile forces the product to round to f32 first, which is what an UNFUSED
      // implementation does. without it duck's -Ofast contracts a * b + c back into an
      // fma and the comparison can never discriminate.
      volatile f32 prod = a * b;
      if ( ref != static_cast<f32>(prod) + c ) ++differs_from_naive;
    }
    require_true(ok);
    // the oracle only proves anything if it disagrees with the unfused a * b + c somewhere;
    // otherwise the loop above would pass against a non-fused implementation too
    require_true(differs_from_naive > 0);
    require_true(math::ffma(f64{ 2 }, f64{ 3 }, f64{ 4 }) == 10.0);
  }
  end_test_case();

  test_case("vector_2::mul_add -- was uninstantiable, called micron::math::fma with two vectors");
  {
    const micron::vector_2<f64> p{ 2.0, 3.0 }, v{ 5.0, 7.0 }, w{ 1.0, 1.0 };
    const micron::vector_2<f64> r = p.mul_add(v, w);
    const micron::vector_2<f64> rs = p.mul_add(4.0, w);
    require_true(near(r.x, 11.0, 1e-15) && near(r.y, 22.0, 1e-15));
    require_true(near(rs.x, 9.0, 1e-15) && near(rs.y, 13.0, 1e-15));
  }
  end_test_case();

  print("=== VECTOR COMPONENTWISE OPS PASSED ===");
  return 1;
}
