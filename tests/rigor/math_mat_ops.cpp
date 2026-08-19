// math_mat_ops.cpp -- mat<T,R,C> operator surface.
//
// Two things are pinned here that used to be wrong:
//   1. a * b (and a / b, a / s) resolved to the operator inherited from
//      int_matrix_base_avx and returned THE BASE TYPE, not mat -- so
//      `mat<f32,3,3> c = a * b;` would not compile. The static_asserts below
//      are the regression.
//   2. there was no matrix product of any kind on mat, and no mat*vec.
//      matmul()/matvec() are checked against linalg::ops::gemm/gemv and an
//      independent f64 scalar oracle.
//
// operator* on two mats remains ELEMENT-WISE by design; matmul() is the product.

#include "../../src/math/linalg/ops.hpp"
#include "../../src/math/matrix/dynmat.hpp"
#include "../../src/math/matrix/mat.hpp"
#include "../../src/math/matrix/matrices.hpp"
#include "../../src/math/quants/vec.hpp"
#include "../../src/type_traits.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace ml = micron::math;
namespace lo = micron::math::linalg::ops;

namespace
{

bool
near(f64 a, f64 b, f64 e)
{
  const f64 d = a - b;
  return (d < 0 ? -d : d) <= e;
}

u64 g_seed = 0x0DDC0FFEE0DDF00DULL;

u64
rnd_next(void)
{
  g_seed = g_seed * 6364136223846793005ULL + 1442695040888963407ULL;
  return g_seed;
}

f64
rnd_sym(void)
{
  return (static_cast<f64>(rnd_next() >> 11) * 0x1.0p-53) * 2.0 - 1.0;
}

f64
rnd_nz(void)
{
  const f64 v = (static_cast<f64>(rnd_next() >> 11) * 0x1.0p-53) * 2.0 + 1.0;
  return (rnd_next() & 1u) ? v : -v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// return-type pins: every one of these was the base class before
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

using m33 = ml::mat<f32, 3, 3>;
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() * micron::declval<const m33 &>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() / micron::declval<const m33 &>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() + micron::declval<const m33 &>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() - micron::declval<const m33 &>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() * micron::declval<f32>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() / micron::declval<f32>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<const m33 &>() + micron::declval<f32>()), m33>);
static_assert(micron::is_same_v<decltype(micron::declval<f32>() - micron::declval<const m33 &>()), m33>);
static_assert(micron::is_same_v<decltype(-micron::declval<const m33 &>()), m33>);
static_assert(
    micron::is_same_v<decltype(ml::matmul(micron::declval<const ml::mat<f32, 2, 3> &>(), micron::declval<const ml::mat<f32, 3, 4> &>())),
                      ml::mat<f32, 2, 4>>);
static_assert(
    micron::is_same_v<decltype(ml::matvec(micron::declval<const m33 &>(), micron::declval<const ml::vec<f32, 3> &>())), ml::vec<f32, 3>>);

// the base's introspection typedefs were all private -- the class opened `class {` and only
// reached `public:` after them, so int4x4_t::value_type did not compile
static_assert(micron::is_same_v<micron::int4x4_t::value_type, i32>);
static_assert(micron::is_same_v<micron::int4x4_t::size_type, usize>);
static_assert(micron::is_same_v<micron::int4x4_t::iterator, i32 *>);
static_assert(micron::is_same_v<micron::int4x4_t::const_reference, const i32 &>);

// braced init picks the initializer_list ctor over the variadic one; that ctor was not
// constexpr, so a braced matrix could never be a constant expression. nor could any accessor.
constexpr ml::mat<f64, 2, 2> __bi{ 1.0, 2.0, 3.0, 4.0 };
static_assert(__bi[1] == 2.0);
static_assert((__bi[1, 0]) == 3.0);
static_assert(__bi.row(1) == 3.0 && __bi.col(1) == 2.0);
static_assert(__bi.transpose().data[1] == 3.0);
static_assert(__bi.mul(__bi).data[0] == 7.0);
static_assert(__bi.scale(2.0).data[3] == 8.0);
static_assert((__bi + __bi).data[0] == 2.0 && (__bi - __bi).data[0] == 0.0);

// a bare scalar must no longer become a whole matrix
static_assert(!micron::is_convertible_v<f32, m33>);
static_assert(!micron::is_convertible_v<f32, micron::int_matrix_base_avx<f32, 3, 3>>);
// direct-init stays legal
static_assert(micron::is_constructible_v<m33, f32>);

// constexpr pins
constexpr ml::mat<f64, 2, 2> __ca(1.0, 2.0, 3.0, 4.0);
constexpr ml::mat<f64, 2, 2> __cb(5.0, 6.0, 7.0, 8.0);
constexpr ml::mat<f64, 2, 2> __cp = ml::matmul(__ca, __cb);
static_assert(__cp.data[0] == 19.0 && __cp.data[1] == 22.0 && __cp.data[2] == 43.0 && __cp.data[3] == 50.0);
constexpr ml::mat<f64, 2, 2> __ce = __ca * __cb;      // still element-wise
static_assert(__ce.data[0] == 5.0 && __ce.data[3] == 32.0);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// oracles
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T, usize R, usize K, usize C>
bool
matmul_vs_oracle(int iters, f64 tol)
{
  bool ok = true;
  for ( int t = 0; t < iters && ok; ++t ) {
    ml::mat<T, R, K> A{};
    ml::mat<T, K, C> B{};
    for ( usize i = 0; i < R * K; ++i ) A.data[i] = static_cast<T>(rnd_sym());
    for ( usize i = 0; i < K * C; ++i ) B.data[i] = static_cast<T>(rnd_sym());

    const ml::mat<T, R, C> got = ml::matmul(A, B);
    const ml::mat<T, R, C> ref = lo::gemm(A, B);
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) {
        f64 acc = 0.0;
        for ( usize k = 0; k < K; ++k ) acc += static_cast<f64>(A.data[i * K + k]) * static_cast<f64>(B.data[k * C + j]);
        ok = ok && near(static_cast<f64>(got.data[i * C + j]), acc, tol);
        ok = ok && near(static_cast<f64>(got.data[i * C + j]), static_cast<f64>(ref.data[i * C + j]), tol);
      }
  }
  return ok;
}

template<typename T, usize R, usize C>
bool
elementwise_vs_oracle(int iters, f64 tol)
{
  bool ok = true;
  for ( int t = 0; t < iters && ok; ++t ) {
    ml::mat<T, R, C> MA{}, MB{};
    for ( usize i = 0; i < R * C; ++i ) {
      MA.data[i] = static_cast<T>(rnd_nz());
      MB.data[i] = static_cast<T>(rnd_nz());
    }
    const ml::mat<T, R, C> A = MA, B = MB;
    const T s = static_cast<T>(rnd_nz());

    const ml::mat<T, R, C> pm = A * B, pd = A / B, pa = A + B, ps = A - B;
    const ml::mat<T, R, C> sm = A * s, sd = A / s, sa = A + s, ss = A - s;
    const ml::mat<T, R, C> ls = s - A, la = s + A, ng = -A;

    for ( usize i = 0; i < R * C; ++i ) {
      const f64 av = static_cast<f64>(A.data[i]), bv = static_cast<f64>(B.data[i]), sv = static_cast<f64>(s);
      ok = ok && near(static_cast<f64>(pm.data[i]), av * bv, tol);
      ok = ok && near(static_cast<f64>(pd.data[i]), av / bv, tol);
      ok = ok && near(static_cast<f64>(pa.data[i]), av + bv, tol);
      ok = ok && near(static_cast<f64>(ps.data[i]), av - bv, tol);
      ok = ok && near(static_cast<f64>(sm.data[i]), av * sv, tol);
      ok = ok && near(static_cast<f64>(sd.data[i]), av / sv, tol);
      ok = ok && near(static_cast<f64>(sa.data[i]), av + sv, tol);
      ok = ok && near(static_cast<f64>(ss.data[i]), av - sv, tol);
      ok = ok && near(static_cast<f64>(ls.data[i]), sv - av, tol);
      ok = ok && near(static_cast<f64>(la.data[i]), sv + av, tol);
      ok = ok && near(static_cast<f64>(ng.data[i]), -av, tol);
    }
  }
  return ok;
}

template<typename T, usize R, usize C>
bool
matvec_vs_gemv(int iters, f64 tol)
{
  bool ok = true;
  for ( int t = 0; t < iters && ok; ++t ) {
    ml::mat<T, R, C> M{};
    ml::vec<T, C> v{};
    for ( usize i = 0; i < R * C; ++i ) M.data[i] = static_cast<T>(rnd_sym());
    for ( usize i = 0; i < C; ++i ) v.data[i] = static_cast<T>(rnd_sym());

    const ml::vec<T, R> got = ml::matvec(M, v);
    const ml::vec<T, R> ref = lo::gemv(M, v);
    for ( usize i = 0; i < R; ++i ) {
      f64 acc = 0.0;
      for ( usize j = 0; j < C; ++j ) acc += static_cast<f64>(M.data[i * C + j]) * static_cast<f64>(v.data[j]);
      ok = ok && near(static_cast<f64>(got.data[i]), acc, tol);
      ok = ok && near(static_cast<f64>(got.data[i]), static_cast<f64>(ref.data[i]), tol);
    }
  }
  return ok;
}

};      // namespace

int
main()
{
  print("=== MAT OPERATOR SURFACE ===");

  test_case("element-wise and scalar operators match a scalar oracle and return mat");
  {
    require_true(elementwise_vs_oracle<f32, 3, 3>(2000, 1e-5));
    require_true(elementwise_vs_oracle<f64, 3, 3>(2000, 1e-12));
    require_true(elementwise_vs_oracle<f64, 4, 4>(1000, 1e-12));
    require_true(elementwise_vs_oracle<f64, 2, 5>(1000, 1e-12));
  }
  end_test_case();

  test_case("matmul square -- agrees with linalg::ops::gemm and an f64 oracle");
  {
    require_true((matmul_vs_oracle<f32, 4, 4, 4>(2000, 1e-4)));
    require_true((matmul_vs_oracle<f64, 4, 4, 4>(2000, 1e-12)));
    require_true((matmul_vs_oracle<f64, 3, 3, 3>(2000, 1e-12)));
  }
  end_test_case();

  test_case("matmul non-square -- the shape-generic path gemm/mul() never covered");
  {
    require_true((matmul_vs_oracle<f64, 2, 3, 4>(2000, 1e-12)));
    require_true((matmul_vs_oracle<f64, 5, 2, 3>(2000, 1e-12)));
    require_true((matmul_vs_oracle<f32, 3, 5, 2>(1000, 1e-4)));
  }
  end_test_case();

  test_case("matmul: identity is neutral, and it is NOT the element-wise operator*");
  {
    ml::mat<f64, 4, 4> A{};
    for ( usize i = 0; i < 16; ++i ) A.data[i] = static_cast<f64>(rnd_sym());
    const ml::mat<f64, 4, 4> I = ml::mat<f64, 4, 4>::identity();
    const ml::mat<f64, 4, 4> l = ml::matmul(I, A), r = ml::matmul(A, I);
    bool ok = true;
    for ( usize i = 0; i < 16; ++i ) ok = ok && near(l.data[i], A.data[i], 1e-15) && near(r.data[i], A.data[i], 1e-15);
    require_true(ok);

    // element-wise A*I zeroes everything off the diagonal -- proof the two differ
    const ml::mat<f64, 4, 4> e = A * I;
    require_true(e.data[1] == 0.0 && e.data[4] == 0.0 && near(e.data[5], A.data[5], 1e-15));
  }
  end_test_case();

  test_case("matvec -- agrees with linalg::ops::gemv and an f64 oracle");
  {
    require_true((matvec_vs_gemv<f32, 4, 4>(2000, 1e-4)));
    require_true((matvec_vs_gemv<f64, 4, 4>(2000, 1e-12)));
    require_true((matvec_vs_gemv<f64, 3, 3>(2000, 1e-12)));
    require_true((matvec_vs_gemv<f64, 2, 6>(1000, 1e-12)));
  }
  end_test_case();

  test_case("matmul associativity (A*B)*C == A*(B*C) on f64");
  {
    ml::mat<f64, 3, 3> A{}, B{}, C{};
    for ( usize i = 0; i < 9; ++i ) {
      A.data[i] = rnd_sym();
      B.data[i] = rnd_sym();
      C.data[i] = rnd_sym();
    }
    const ml::mat<f64, 3, 3> l = ml::matmul(ml::matmul(A, B), C);
    const ml::mat<f64, 3, 3> r = ml::matmul(A, ml::matmul(B, C));
    bool ok = true;
    for ( usize i = 0; i < 9; ++i ) ok = ok && near(l.data[i], r.data[i], 1e-12);
    require_true(ok);
  }
  end_test_case();

  test_case("operator[](usize) means the same element through the base and through mat");
  {
    ml::mat<f64, 4, 4> m{};
    for ( usize i = 0; i < 16; ++i ) m.data[i] = static_cast<f64>(i);
    auto &base = static_cast<micron::int_matrix_base_avx<f64, 4, 4> &>(m);
    // this used to be data[i] through mat and data[r * C] through the base, so the same
    // spelling selected a different element depending on the static type you held
    bool ok = true;
    for ( usize i = 0; i < 16; ++i ) ok = ok && (m[i] == base[i]) && (base[i] == static_cast<f64>(i));
    require_true(ok);
    require_true((m[1, 1]) == 5.0);        // the (r, c) form is unchanged
    require_true(base.row(1) == 4.0);      // the row head is still reachable, by name
    require_true(base.col(3) == 3.0);
  }
  end_test_case();

  test_case("dynmat move clears the source shape and self-move is a no-op");
  {
    ml::dynmat<f64> a(3, 4);
    a.at(2, 3) = 7.0;
    ml::dynmat<f64> b(micron::move(a));
    // a defaulted move emptied buf but left rows/cols/ld, so at(i, j) hit a null data()
    require_true(a.rows == 0 && a.cols == 0 && a.ld == 0);
    require_true(b.rows == 3 && b.cols == 4 && b.ld == 4 && b.at(2, 3) == 7.0);

    ml::dynmat<f64> c(2, 2);
    c = micron::move(b);
    require_true(b.rows == 0 && b.cols == 0 && b.ld == 0);
    require_true(c.rows == 3 && c.cols == 4 && c.at(2, 3) == 7.0);

    // storage_type is Sf = false, which skips micron::vector's own self-assignment guard
    c = micron::move(c);
    require_true(c.rows == 3 && c.cols == 4 && c.ld == 4 && c.at(2, 3) == 7.0);
  }
  end_test_case();

  print("=== MAT OPERATOR SURFACE PASSED ===");
  return 1;
}
