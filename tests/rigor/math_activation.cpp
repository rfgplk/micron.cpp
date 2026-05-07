// math_activation.cpp
// Snowball tests for math::activation — relu/leaky_relu/elu/gelu/silu/mish/...

#include "../../src/math/activation.hpp"
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
  print("=== ACTIVATION TESTS ===");

  test_case("relu / leaky_relu");
  {
    require_true(near(activation::relu<f64>(0.0), 0.0));
    require_true(near(activation::relu<f64>(2.0), 2.0));
    require_true(near(activation::relu<f64>(-2.0), 0.0));

    require_true(near(activation::leaky_relu<f64>(2.0), 2.0));
    require_true(near(activation::leaky_relu<f64>(-2.0, 0.1), -0.2));
  }
  end_test_case();

  test_case("elu");
  {
    require_true(near(activation::elu<f64>(2.0, 1.0), 2.0));
    require_true(near(activation::elu<f64>(0.0, 1.0), 0.0));
    require_true(near(activation::elu<f64>(-2.0, 1.0), -0.8646647167633873, 1e-9));
  }
  end_test_case();

  test_case("gelu / gelu_approx");
  {
    require_true(near(activation::gelu<f64>(0.0), 0.0));
    // gelu(1) ≈ 0.8413447460685429 (via erf — faithful ~5 ulp)
    require_true(near(activation::gelu<f64>(1.0), 0.8413447460685429, 1e-6));
    // approximation should be close but not bit-equal
    require_true(near(activation::gelu_approx<f64>(1.0), 0.8413447460685429, 1e-3));
  }
  end_test_case();

  test_case("silu / mish / softsign / hardswish");
  {
    // silu(0) = 0
    require_true(near(activation::silu<f64>(0.0), 0.0));
    // silu(2) = 2 · σ(2) ≈ 2 · 0.880797 = 1.761594
    require_true(near(activation::silu<f64>(2.0), 1.7615941559557649, 1e-7));

    require_true(near(activation::mish<f64>(0.0), 0.0));

    require_true(near(activation::softsign<f64>(2.0), 2.0 / 3.0, 1e-12));
    require_true(near(activation::softsign<f64>(-3.0), -3.0 / 4.0, 1e-12));

    require_true(near(activation::hardswish<f64>(-3.0), 0.0));
    require_true(near(activation::hardswish<f64>(3.0), 3.0));
    require_true(near(activation::hardswish<f64>(0.0), 0.0));
  }
  end_test_case();

  test_case("bulk + container");
  {
    f64 x[4] = { -1, 0, 1, 2 };
    activation::relu<f64>(x, 4);
    require_true(near(x[0], 0.0));
    require_true(near(x[1], 0.0));
    require_true(near(x[2], 1.0));
    require_true(near(x[3], 2.0));

    micron::vector<f64> v(3, 0.0);
    v[0] = -1; v[1] = 0; v[2] = 1;
    activation::silu(v);
    require_true(near(v[0], -0.2689414213699951, 1e-7));
    require_true(near(v[1], 0.0));
    require_true(near(v[2], 0.7310585786300049, 1e-7));
  }
  end_test_case();

  print("=== activation ok ===");
  return 1;
}
