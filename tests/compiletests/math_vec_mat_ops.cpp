// compile-validity gate: the componentwise vector operators and the mat<T,R,C>
// operator surface, on every arch/ISA/opt cell, f32 and f64, runtime and
// consteval. Every operand is bound CONST on purpose -- operator/(vector,vector)
// was once the only binary operator here without a trailing const, and only a
// const operand catches that. Not run.
#include "../../src/math/generic.hpp"
#include "../../src/math/linalg/ops.hpp"
#include "../../src/math/matrix/dynmat.hpp"
#include "../../src/math/matrix/mat.hpp"
#include "../../src/math/matrix/matrices.hpp"
#include "../../src/math/quants/vec.hpp"
#include "../../src/math/quants/vecs.hpp"

using micron::math::mat;
using micron::math::matmul;
using micron::math::matvec;
using micron::math::vec;
using ml_dynmat = micron::math::dynmat<f64>;

// consteval pins
constexpr mat<f64, 2, 2> ca(1.0, 2.0, 3.0, 4.0);
constexpr mat<f64, 2, 2> cb(5.0, 6.0, 7.0, 8.0);
constexpr mat<f64, 2, 2> cmm = matmul(ca, cb);
static_assert(cmm.data[0] == 19.0 && cmm.data[3] == 50.0);
constexpr mat<f64, 2, 2> cew = ca * cb;      // element-wise, by design
static_assert(cew.data[0] == 5.0 && cew.data[3] == 32.0);
static_assert((micron::vector_3<f64>{ 6.0, 8.0, 10.0 } / micron::vector_3<f64>{ 2.0, 4.0, 5.0 }).y == 2.0);
static_assert(micron::vector_2<f64>{ 9.0, 4.0 }.quotient(micron::vector_2<f64>{ 3.0, 2.0 }).x == 3.0);

// ffma must fuse at the argument's own width on every arch, and an integral argument must
// still reach the double fallback
static_assert(micron::is_same_v<decltype(micron::math::ffma(f32{ 1 }, f32{ 2 }, f32{ 3 })), f32>);
static_assert(micron::is_same_v<decltype(micron::math::ffma(f64{ 1 }, f64{ 2 }, f64{ 3 })), f64>);
static_assert(micron::is_same_v<decltype(micron::math::ffma(1, 2, 3)), double>);

// the base's introspection typedefs must be reachable
static_assert(micron::is_same_v<micron::int4x4_t::value_type, i32>);
static_assert(micron::is_same_v<micron::int4x4_t::const_reference, const i32 &>);

// braced init and the whole accessor/arithmetic surface must be constant-evaluable
constexpr mat<f64, 2, 2> cbi{ 1.0, 2.0, 3.0, 4.0 };
static_assert(cbi[1] == 2.0 && (cbi[1, 0]) == 3.0);
static_assert(cbi.row(1) == 3.0 && cbi.col(1) == 2.0);
static_assert(cbi.transpose().data[1] == 3.0 && cbi.mul(cbi).data[0] == 7.0);
static_assert(cbi.scale(2.0).data[3] == 8.0 && cbi.div_scalar(2.0).data[1] == 1.0);

// a bare scalar must not convert to a whole matrix
static_assert(!micron::is_convertible_v<f64, mat<f64, 2, 2>>);

template<typename T>
static T
touch_quant_vectors(void)
{
  const micron::vector_2<T> a2{ T(1), T(2) }, b2{ T(3), T(4) };
  const micron::vector_3<T> a3{ T(1), T(2), T(3) }, b3{ T(3), T(4), T(5) };
  const micron::vector_4<T> a4{ T(1), T(2), T(3), T(4) }, b4{ T(3), T(4), T(5), T(6) };
  const micron::vector_8<T> a8{ T(1), T(2), T(3), T(4), T(5), T(6), T(7), T(8) };
  const micron::vector_8<T> b8{ T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2) };
  const micron::vector_16<T> a16{ T(1), T(2), T(3), T(4), T(5), T(6), T(7), T(8), T(9), T(10), T(11), T(12), T(13), T(14), T(15), T(16) };
  const micron::vector_16<T> b16{ T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2), T(2) };

  T acc = (a2 / b2).x + (a2 * b2).y + a2.quotient(b2).x + a2.product(b2).y;
  acc += (a3 / b3).z + (a3 * b3).x + a3.quotient(b3).y + a3.product(b3).z;
  acc += (a4 / b4).w + (a4 * b4).z + a4.quotient(b4).x + a4.product(b4).w;
  acc += (a8 / b8).d + (a8 * b8).a + a8.quotient(b8).b + a8.product(b8).c;
  acc += (a16 / b16).l + (a16 * b16).e + a16.quotient(b16).f + a16.product(b16).g;

  micron::vector_2<T> m2 = a2;
  micron::vector_3<T> m3 = a3;
  micron::vector_4<T> m4 = a4;
  micron::vector_8<T> m8 = a8;
  micron::vector_16<T> m16 = a16;
  m2 *= b2;
  m2 /= b2;
  m3 *= b3;
  m3 /= b3;
  m4 *= b4;
  m4 /= b4;
  m8 *= b8;
  m8 /= b8;
  m16 *= b16;
  m16 /= b16;
  acc += m2.x + m3.y + m4.z + m8.d + m16.l;
  acc += a2.mul_add(b2, a2).x + a2.mul_add(T(2), a2).y;
  return acc;
}

template<typename T>
static T
touch_mvec(void)
{
  const vec<T, 3> a{ T(1), T(2), T(3) }, b{ T(4), T(5), T(6) };
  vec<T, 3> m = a;
  m *= b;
  m /= b;
  T acc = (a * b).data[0] + (a / b).data[1] + (T(2) / b).data[2] + m.data[0];
  acc += (a * T(2)).data[0] + (T(2) * a).data[1] + (a / T(2)).data[2];
  return acc;
}

template<typename T>
static T
touch_mat(void)
{
  const mat<T, 3, 3> A = mat<T, 3, 3>::identity();
  mat<T, 3, 3> Bm = mat<T, 3, 3>::identity();
  Bm.data[1] = T(2);
  const mat<T, 3, 3> B = Bm;
  const T s = T(2);

  // every one of these must yield mat<T,3,3>, not the base class
  const mat<T, 3, 3> e0 = A * B;
  const mat<T, 3, 3> e1 = A / B;
  const mat<T, 3, 3> e2 = A + B;
  const mat<T, 3, 3> e3 = A - B;
  const mat<T, 3, 3> e4 = A * s;
  const mat<T, 3, 3> e5 = s * A;
  const mat<T, 3, 3> e6 = A / s;
  const mat<T, 3, 3> e7 = A + s;
  const mat<T, 3, 3> e8 = s + A;
  const mat<T, 3, 3> e9 = A - s;
  const mat<T, 3, 3> ea = s - A;
  const mat<T, 3, 3> eb = -A;

  T acc = e0.data[0] + e1.data[0] + e2.data[0] + e3.data[0] + e4.data[0] + e5.data[0];
  acc += e6.data[0] + e7.data[0] + e8.data[0] + e9.data[0] + ea.data[0] + eb.data[0];
  acc += (A == B) ? T(1) : T(0);
  acc += (A != B) ? T(1) : T(0);

  // shape-generic product + mat*vec, cross-checked against the linalg SIMD paths
  const mat<T, 2, 3> P{};
  const mat<T, 3, 4> Q{};
  const mat<T, 2, 4> R = matmul(P, Q);
  acc += R.data[0];
  acc += matmul(A, B).data[4];
  acc += micron::math::linalg::ops::gemm(A, B).data[4];

  const vec<T, 3> v{ T(1), T(2), T(3) };
  acc += matvec(A, v).data[1];
  acc += micron::math::linalg::ops::gemv(A, v).data[1];
  return acc;
}

static f64
touch_dynmat(void)
{
  ml_dynmat a(3, 4);
  a.at(2, 3) = 7.0;
  ml_dynmat b(micron::move(a));
  ml_dynmat c(2, 2);
  c = micron::move(b);
  return c.at(2, 3) + static_cast<f64>(a.rows + b.cols + c.ld);
}

int
main(void)
{
  f64 d = static_cast<f64>(touch_quant_vectors<f32>());
  d += touch_quant_vectors<f64>();
  d += static_cast<f64>(touch_mvec<f32>());
  d += touch_mvec<f64>();
  d += touch_dynmat();
  d += static_cast<f64>(touch_mat<f32>());
  d += touch_mat<f64>();
  return static_cast<int>(d) & 0x7f;
}
