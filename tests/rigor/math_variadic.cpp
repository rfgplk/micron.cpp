// math_variadic.cpp
// Smoke test for the variadic in-place fold overloads added across the
// freestanding math kernel:
//   - activation::{relu,gelu,silu,...}(C&, ...)  via MICRON_ACTIVATION_BULK
//   - activation::{leaky_relu,elu}(alpha, C&, ...)
//   - softmax / log_softmax(C&, ...)
//   - blas::level1::scal(alpha, C&, ...)
//   - rng::dists::shuffle(rng, C&, ...)

#include "../../src/math/activation.hpp"
#include "../../src/math/blas/level1.hpp"
#include "../../src/math/log.hpp"
#include "../../src/math/rng.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-7)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== VARIADIC IN-PLACE FOLD TESTS ===");

  test_case("activation::relu — 3 containers folded");
  {
    micron::vector<f64> a(3, 0.0); a[0] = -1; a[1] = 0; a[2] = 1;
    micron::vector<f64> b(3, 0.0); b[0] = -2; b[1] = 2; b[2] = -3;
    micron::vector<f64> c(3, 0.0); c[0] =  4; c[1] = -5; c[2] = 6;
    activation::relu(a, b, c);
    require_true(near(a[0], 0.0));
    require_true(near(a[2], 1.0));
    require_true(near(b[0], 0.0));
    require_true(near(b[1], 2.0));
    require_true(near(c[1], 0.0));
    require_true(near(c[2], 6.0));
  }
  end_test_case();

  test_case("activation::leaky_relu — alpha-led variadic across 2 containers");
  {
    micron::vector<f64> a(2, 0.0); a[0] = -2; a[1] = 3;
    micron::vector<f64> b(2, 0.0); b[0] =  4; b[1] = -5;
    activation::leaky_relu(0.5, a, b);
    require_true(near(a[0], -1.0));     // -2 * 0.5
    require_true(near(a[1],  3.0));
    require_true(near(b[0],  4.0));
    require_true(near(b[1], -2.5));     // -5 * 0.5
  }
  end_test_case();

  test_case("activation::elu — default-alpha variadic across 2 containers");
  {
    micron::vector<f64> a(2, 0.0); a[0] = 2; a[1] = -1;
    micron::vector<f64> b(2, 0.0); b[0] = 1; b[1] =  0;
    activation::elu(a, b);
    require_true(near(a[0], 2.0));
    require_true(near(a[1], -0.6321205588285577, 1e-9));
    require_true(near(b[0], 1.0));
    require_true(near(b[1], 0.0));
  }
  end_test_case();

  test_case("math::softmax — variadic across 2 vectors");
  {
    micron::vector<f64> a(3, 0.0); a[0] = 1; a[1] = 2; a[2] = 3;
    micron::vector<f64> b(3, 0.0); b[0] = 0; b[1] = 0; b[2] = 0;
    micron::math::softmax(a, b);
    f64 sa = a[0] + a[1] + a[2];
    f64 sb = b[0] + b[1] + b[2];
    require_true(near(sa, 1.0));
    require_true(near(sb, 1.0));
    require_true(near(b[0], 1.0 / 3.0));
  }
  end_test_case();

  test_case("blas::level1::scal — alpha-led variadic across 2 containers");
  {
    micron::vector<f64> a(3, 0.0); a[0] = 1; a[1] = 2; a[2] = 3;
    micron::vector<f64> b(3, 0.0); b[0] = 4; b[1] = 5; b[2] = 6;
    blas::level1::scal(2.0, a, b);
    require_true(near(a[0], 2.0));
    require_true(near(a[2], 6.0));
    require_true(near(b[0], 8.0));
    require_true(near(b[2], 12.0));
  }
  end_test_case();

  test_case("rng::dists::shuffle — variadic fold across 2 containers");
  {
    rng::splitmix64 g{ 42 };
    micron::vector<i32> a(8, 0);
    micron::vector<i32> b(8, 0);
    for ( i32 i = 0; i < 8; ++i ) { a[i] = i; b[i] = 100 + i; }
    rng::dists::shuffle(g, a, b);
    // both should still be a permutation (sum-preserving sanity check)
    i64 sa = 0, sb = 0;
    for ( i32 i = 0; i < 8; ++i ) { sa += a[i]; sb += b[i]; }
    require_true(sa == (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7));
    require_true(sb == (8 * 100 + 0 + 1 + 2 + 3 + 4 + 5 + 6 + 7));
  }
  end_test_case();

  print("=== variadic ok ===");
  return 1;
}
