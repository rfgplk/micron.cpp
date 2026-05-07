// math_alignment.cpp — pin alignof / sizeof of math types via static_assert.
// Compile-time only; runtime body is a single print so snowball reports
// the file as passing.

#include "../../src/math/matrix/mat.hpp"
#include "../../src/math/matrix/matrices.hpp"
#include "../../src/math/quants/quat.hpp"
#include "../../src/math/quants/vec.hpp"
#include "../../src/math/quants/vecs.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// vector_2 / vector_3 / vector_4 / vector_8 / vector_16 alignment.

static_assert(alignof(vector_2<f32>) == 16);
static_assert(alignof(vector_3<f32>) == 16);
static_assert(alignof(vector_4<f32>) == 16);
static_assert(alignof(vector_8<f32>) == 32);
static_assert(alignof(vector_16<f32>) == 64);
static_assert(alignof(vector_2<f64>) == 16);
static_assert(alignof(vector_3<f64>) == 32);
static_assert(alignof(vector_4<f64>) == 32);
static_assert(alignof(vector_8<f64>) == 64);
static_assert(alignof(vector_16<f64>) == 128);

// sizeof — only vector_2<f32>, vector_3<f32>, vector_3<f64> are
// padded by the alignas requirement.  All others stay tight.
static_assert(sizeof(vector_2<f32>) == 16);     // ABI: 8 → 16
static_assert(sizeof(vector_2<f64>) == 16);     // unchanged
static_assert(sizeof(vector_3<f32>) == 16);     // ABI: 12 → 16
static_assert(sizeof(vector_3<f64>) == 32);     // ABI: 24 → 32
static_assert(sizeof(vector_4<f32>) == 16);
static_assert(sizeof(vector_4<f64>) == 32);
static_assert(sizeof(vector_8<f32>) == 32);
static_assert(sizeof(vector_8<f64>) == 64);
static_assert(sizeof(vector_16<f32>) == 64);
static_assert(sizeof(vector_16<f64>) == 128);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// math::quat<F> — alignment lifted via vec_align_v<F, 4>.
// `quat` (no template) is also a `vector_4<f32>` alias in namespace
// micron — qualify explicitly to disambiguate.

static_assert(alignof(micron::math::quat<f32>) == 16);
static_assert(alignof(micron::math::quat<f64>) == 32);
static_assert(sizeof(micron::math::quat<f32>) == 16);
static_assert(sizeof(micron::math::quat<f64>) == 32);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// math::vec<T, N> — alignment via vec_align_v.  Integer T uses the
// alignment of `double` (defensive over-alignment).

static_assert(alignof(vec<f32, 2>) == 16);
static_assert(alignof(vec<f32, 4>) == 16);
static_assert(alignof(vec<f32, 8>) == 32);
static_assert(alignof(vec<f64, 2>) == 16);
static_assert(alignof(vec<f64, 3>) == 32);
static_assert(alignof(vec<f64, 4>) == 32);
static_assert(alignof(vec<f64, 8>) == 64);
static_assert(alignof(vec<i32, 4>) == 32);     // promoted to double-size

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// math::mat<T, R, C> — alignment via mat_align_v, capped at 64.

static_assert(alignof(mat<f32, 4, 4>) == 64);
static_assert(alignof(mat<f64, 3, 3>) == 64);
static_assert(alignof(mat<f64, 4, 4>) == 64);
static_assert(alignof(mat<f64, 8, 8>) == 64);
static_assert(alignof(mat<f32, 2, 2>) == 16);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// int_matrix_base — class-level alignas(64) makes alignment survive
// when stored as a member, stack temporary, or container element.
// This is the change the matrix/bits.hpp edit verifies.

static_assert(alignof(int_matrix_base<i32, 4, 4>) == 64);
static_assert(alignof(int_matrix_base<i32, 8, 8>) == 64);
static_assert(alignof(int_matrix_base<u32, 16, 16>) == 64);

// Sanity: a struct that contains an int_matrix_base also rounds up.
struct __stuff {
  int_matrix_base<i32, 4, 4> m;
};
static_assert(alignof(__stuff) == 64);

int
main()
{
  print("=== ALIGNMENT TESTS ===");
  test_case("static_asserts above all hold");
  {
    require_true(true);     // compile-time evidence; nothing to verify at runtime
  }
  end_test_case();
  print("=== alignment ok ===");
  return 1;
}
