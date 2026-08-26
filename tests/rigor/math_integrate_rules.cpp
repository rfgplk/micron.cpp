// math_integrate_rules.cpp — coefficient invariants and adaptive status contracts

#include "../../src/math/integrate/integrate.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;
using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 tolerance) noexcept
{
  return math::fabs(a - b) <= tolerance * (math::fabs(b) + 1.0);
}

static f64
integer_power(f64 x, usize exponent) noexcept
{
  f64 result = 1.0;
  for ( usize i = 0; i < exponent; ++i ) result *= x;
  return result;
}

template<usize Order>
static void
check_newton_cotes() noexcept
{
  for ( usize degree = 0; degree <= Order; ++degree ) {
    f64 closed = 0.0;
    f64 open = 0.0;
    for ( usize i = 0; i <= Order; ++i ) {
      closed += integrate::closed_newton_cotes_weight<Order, f64>(i) * integer_power(f64(i), degree);
      open += integrate::open_newton_cotes_weight<Order, f64>(i) * integer_power(f64(i + 1), degree);
    }
    const f64 closed_limit = f64(Order);
    const f64 open_limit = f64(Order + 2);
    require_true(near(closed, integer_power(closed_limit, degree + 1) / f64(degree + 1), 3e-11));
    require_true(near(open, integer_power(open_limit, degree + 1) / f64(degree + 1), 3e-10));
  }
}

template<usize Order>
static void
check_gauss_legendre() noexcept
{
  using table = integrate::coeff::gl::gl_table<f64, Order>;
  f64 weight_sum = 0.0;
  for ( usize i = 0; i < table::half; ++i ) {
    require_true(table::weights[i] > 0.0);
    require_true(table::nodes[i] >= 0.0 && table::nodes[i] < 1.0);
    if ( i != 0 ) require_true(table::nodes[i] > table::nodes[i - 1]);
    weight_sum += (table::has_zero && i == 0 ? 1.0 : 2.0) * table::weights[i];
  }
  require_true(near(weight_sum, 2.0, 2e-14));

  for ( usize degree = 0; degree < 2 * Order; ++degree ) {
    const f64 value = integrate::gauss_legendre<Order, f64>([=](f64 x) noexcept { return integer_power(x, degree); }, -1.0, 1.0);
    const f64 exact = degree & 1 ? 0.0 : 2.0 / f64(degree + 1);
    require_true(near(value, exact, 4e-12));
  }
}

template<usize Order>
static void
check_clenshaw_curtis() noexcept
{
  using table = integrate::coeff::cc::cc_table<f64, Order>;
  f64 weight_sum = 0.0;
  for ( usize i = 0; i <= Order; ++i ) {
    require_true(table::weights[i] > 0.0);
    require_true(near(table::nodes[i], -table::nodes[Order - i], 2e-15));
    require_true(near(table::weights[i], table::weights[Order - i], 2e-15));
    weight_sum += table::weights[i];
  }
  require_true(near(weight_sum, 2.0, 2e-14));

  for ( usize degree = 0; degree <= Order; ++degree ) {
    const f64 value = integrate::clenshaw_curtis<Order, f64>([=](f64 x) noexcept { return integer_power(x, degree); }, -1.0, 1.0);
    const f64 exact = degree & 1 ? 0.0 : 2.0 / f64(degree + 1);
    require_true(near(value, exact, 5e-12));
  }
}

static_assert(u32(integrate::quad_status::ok) == 0);
static_assert(u32(integrate::quad_status::max_depth) == 1);
static_assert(u32(integrate::quad_status::abnormal) == 2);

int
main()
{
  test_case("closed and open Newton-Cotes weights reproduce moments through order eight");
  check_newton_cotes<1>();
  check_newton_cotes<2>();
  check_newton_cotes<3>();
  check_newton_cotes<4>();
  check_newton_cotes<5>();
  check_newton_cotes<6>();
  check_newton_cotes<7>();
  check_newton_cotes<8>();
  end_test_case();

  test_case("fixed Legendre and generated Clenshaw-Curtis tables preserve moments");
  check_gauss_legendre<5>();
  check_gauss_legendre<8>();
  check_gauss_legendre<16>();
  check_gauss_legendre<32>();
  check_gauss_legendre<64>();
  check_clenshaw_curtis<8>();
  check_clenshaw_curtis<16>();
  check_clenshaw_curtis<32>();
  check_clenshaw_curtis<64>();
  end_test_case();

  test_case("reusable weighted Gaussian rules have positive weights and exact low moments");
  {
    integrate::hermite_rule<f64, 16> hermite{};
    hermite.generate();
    integrate::laguerre_rule<f64, 16> laguerre{};
    integrate::jacobi_rule<f64, 16> jacobi{};
    require_true(laguerre.generate());
    require_true(jacobi.generate(0.0, 0.0));
    for ( usize i = 0; i < 16; ++i ) {
      require_true(hermite.weights[i] > 0.0);
      require_true(laguerre.weights[i] > 0.0);
      require_true(jacobi.weights[i] > 0.0);
      if ( i != 0 ) {
        require_true(hermite.nodes[i] > hermite.nodes[i - 1]);
        require_true(laguerre.nodes[i] > laguerre.nodes[i - 1]);
        require_true(jacobi.nodes[i] > jacobi.nodes[i - 1]);
      }
    }
    const f64 sqrt_pi = mk::pow_ns::sqrt<f64>(constant_pi<f64>);
    require_true(near(hermite.apply([](f64) noexcept { return 1.0; }), sqrt_pi, 2e-12));
    require_true(near(hermite.apply([](f64 x) noexcept { return x * x; }), sqrt_pi * 0.5, 3e-12));
    require_true(near(laguerre.apply([](f64) noexcept { return 1.0; }), 1.0, 2e-12));
    require_true(near(laguerre.apply([](f64 x) noexcept { return x * x; }), 2.0, 5e-12));
    require_true(near(jacobi.apply([](f64) noexcept { return 1.0; }), 2.0, 2e-12));
    require_true(near(jacobi.apply([](f64 x) noexcept { return x * x; }), 2.0 / 3.0, 3e-12));
    require_true(!jacobi.generate(-1.0, 0.0));
  }
  require_true(near(integrate::gauss_chebyshev_i<16, f64>([](f64) noexcept { return 1.0; }), constant_pi<f64>, 2e-14));
  require_true(near(integrate::gauss_chebyshev_ii<16, f64>([](f64) noexcept { return 1.0; }), constant_pi<f64> * 0.5, 2e-14));
  end_test_case();

  test_case("adaptive quadrature reports limits, invalid callbacks, and roundoff distinctly");
  {
    auto one = [](f64) noexcept { return 1.0; };
    integrate::quad_options<f64> options{};
    options.abs_tol = 0.0;
    options.rel_tol = 0.0;

    integrate::quad_workspace<f64, 8> workspace{};
    options.max_evals = 21;
    auto evaluations = integrate::quad<f64>(one, 0.0, 1.0, options, workspace);
    require_true(evaluations.status == integrate::quad_status::max_evaluations);
    require_true(evaluations.n_evals == 21);

    options.max_evals = 0;
    integrate::quad_workspace<f64, 1> tiny{};
    auto intervals = integrate::quad<f64>(one, 0.0, 1.0, options, tiny);
    require_true(intervals.status == integrate::quad_status::max_intervals);
    require_true(intervals.n_intervals == 1);

    options.max_depth = 1;
    auto depth = integrate::quad<f64>(one, 0.0, 1.0, options, workspace);
    require_true(depth.status == integrate::quad_status::max_depth);

    options.max_depth = 64;
    auto roundoff = integrate::quad<f64>(one, 1.0e16, 1.0e16 + 2.0, options, workspace);
    require_true(roundoff.status == integrate::quad_status::roundoff);

    auto not_finite = [](f64) noexcept { return f64(__builtin_huge_val()); };
    auto invalid_value = integrate::quad<f64>(not_finite, 0.0, 1.0, options, workspace);
    require_true(invalid_value.status == integrate::quad_status::non_finite);

    options.n_breakpoints = 1;
    options.breakpoints = nullptr;
    auto invalid_options = integrate::quad<f64>(one, 0.0, 1.0, options, workspace);
    require_true(invalid_options.status == integrate::quad_status::invalid_input);
  }
  end_test_case();

  test_case("zero and reversed bounds preserve evaluation and estimator contracts");
  {
    usize calls = 0;
    auto exponential = [&](f64 x) noexcept {
      ++calls;
      return math::exp(x);
    };
    integrate::quad_options<f64> options{};
    options.abs_tol = 1e-12;
    options.rel_tol = 1e-12;
    integrate::quad_workspace<f64, 32> workspace{};
    auto zero = integrate::quad<f64>(exponential, 0.5, 0.5, options, workspace);
    require_true(zero.status == integrate::quad_status::ok && zero.value == 0.0 && calls == 0);
    auto reversed = integrate::quad<f64>(exponential, 1.0, 0.0, options, workspace);
    const f64 exact = 1.0 - math::exp(1.0);
    const f64 true_error = math::fabs(reversed.value - exact);
    require_true(reversed.status == integrate::quad_status::ok);
    require_true(near(reversed.value, exact, 2e-13));
    require_true(reversed.abs_err >= 0.0 && reversed.resabs >= 0.0 && reversed.resasc >= 0.0);
    require_true(true_error <= reversed.abs_err * 64.0 + 64.0 * integrate::machine_epsilon<f64>());
  }
  end_test_case();

  return 1;
}
