// math_integrate_expanded.cpp — scientific quadrature, cubature, and IVP rigor

#include "../../src/math/integrate/integrate.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;
using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 tolerance)
{
  return math::fabs(a - b) <= tolerance;
}

int
main()
{
  test_case("global quad workspace, breakpoints, and improper bounds");
  {
    integrate::quad_workspace<f64, 64> workspace{};
    integrate::quad_options<f64> options{};
    options.abs_tol = 1e-11;
    options.rel_tol = 1e-11;
    const f64 points[] = { 0.25, 0.75 };
    options.breakpoints = points;
    options.n_breakpoints = 2;
    auto square = [](f64 x) noexcept { return x * x; };
    auto finite = integrate::quad<f64>(square, 0.0, 1.0, options, workspace);
    require_true(finite.status == integrate::quad_status::ok);
    require_true(near(finite.value, 1.0 / 3.0, 1e-11));

    options.breakpoints = nullptr;
    options.n_breakpoints = 0;
    auto decay = [](f64 x) noexcept { return math::exp<f64>(-x); };
    auto improper = integrate::quad<f64>(decay, 0.0, f64(__builtin_huge_val()), options, workspace);
    require_true(improper.status == integrate::quad_status::ok);
    require_true(near(improper.value, 1.0, 2e-9));
  }
  end_test_case();

  test_case("coupled vector quadrature with max and L2 control");
  {
    usize calls = 0;
    auto vector_function = [&](f64 x) noexcept {
      ++calls;
      return vec<f64, 2>{ { x * x, math::exp<f64>(-x) } };
    };
    integrate::quad_options<f64> options{};
    options.abs_tol = 1e-11;
    options.rel_tol = 1e-11;
    integrate::quad_vec_workspace<2, f64, 64> workspace{};
    auto maximum = integrate::quad_vec<2, f64>(vector_function, 0.0, 1.0, options, workspace, integrate::quad_error_norm::maximum);
    require_true(maximum.status == integrate::quad_status::ok);
    require_true(maximum.n_evals == calls);
    require_true(near(maximum.value.data[0], 1.0 / 3.0, 2e-12));
    require_true(near(maximum.value.data[1], 1.0 - math::exp<f64>(-1.0), 2e-12));
    auto reversed = integrate::quad_vec<2, f64>(vector_function, 1.0, 0.0, options, workspace, integrate::quad_error_norm::l2);
    require_true(reversed.status == integrate::quad_status::ok);
    require_true(near(reversed.value.data[0], -1.0 / 3.0, 2e-12));
  }
  end_test_case();

  test_case("double-exponential, fixed Gaussian, Clenshaw-Curtis, and Newton-Cotes");
  {
    auto singular = [](f64 x) noexcept { return 1.0 / math::mk::pow_ns::sqrt<f64>(x); };
    integrate::tanh_sinh_options<f64> de{};
    de.abs_tol = 1e-10;
    de.rel_tol = 1e-10;
    auto ts = integrate::tanh_sinh<f64>(singular, 0.0, 1.0, de);
    require_true(near(ts.value, 2.0, 2e-8));

    auto singular_f32 = [](f32 x) noexcept { return f32(1) / math::mk::pow_ns::sqrt<f32>(x); };
    integrate::tanh_sinh_options<f32> de_f32{};
    de_f32.abs_tol = 1e-5f;
    de_f32.rel_tol = 1e-5f;
    auto ts_f32 = integrate::tanh_sinh<f32>(singular_f32, 0.0f, 1.0f, de_f32);
    const f32 ts_f32_error = math::fabs(ts_f32.value - 2.0f);
    require_true(ts_f32.status == integrate::quad_status::ok);
    require_true(ts_f32_error <= 2e-5f);
    require_true(ts_f32_error <= ts_f32.abs_err + 16 * integrate::machine_epsilon<f32>());
    auto decay_f32 = [](f32 x) noexcept { return math::exp<f32>(-x); };
    auto improper_f32 = integrate::tanh_sinh<f32>(decay_f32, 0.0f, f32(__builtin_huge_valf()), de_f32);
    require_true(improper_f32.status == integrate::quad_status::ok);
    require_true(math::fabs(improper_f32.value - 1.0f) <= 3e-5f);

    auto polynomial = [](f64 x) noexcept {
      f64 p = 1.0;
      for ( usize i = 0; i < 20; ++i ) p *= x;
      return p;
    };
    const f64 gl = integrate::gauss_legendre<64, f64>(polynomial, -1.0, 1.0);
    const f64 cc = integrate::clenshaw_curtis<32, f64>(polynomial, -1.0, 1.0);
    const f64 nc = integrate::boole<f64>([](f64 x) noexcept { return x * x * x * x; }, 0.0, 1.0);
    const f64 gh = integrate::gauss_hermite<8, f64>([](f64) noexcept { return 1.0; });
    const f64 gla = integrate::gauss_laguerre<8, f64>([](f64) noexcept { return 1.0; });
    const f64 gj = integrate::gauss_jacobi<8, f64>([](f64) noexcept { return 1.0; }, 0.0, 0.0);
    const f64 gj_cheb = integrate::gauss_jacobi<8, f64>([](f64) noexcept { return 1.0; }, -0.5, -0.5);
    require_true(near(gl, 2.0 / 21.0, 2e-14));
    require_true(near(cc, 2.0 / 21.0, 2e-13));
    require_true(near(nc, 0.2, 2e-14));
    require_true(near(gh, math::mk::pow_ns::sqrt<f64>(constant_pi<f64>), 2e-12));
    require_true(near(gla, 1.0, 2e-12));
    require_true(near(gj, 2.0, 2e-12));
    require_true(near(gj_cheb, constant_pi<f64>, 2e-12));
  }
  end_test_case();

  test_case("sampled irregular Simpson and high derivative");
  {
    const f64 xs[] = { 0.0, 0.2, 0.55, 0.8, 1.0 };
    f64 ys[5]{};
    for ( usize i = 0; i < 5; ++i ) ys[i] = xs[i] * xs[i];
    require_true(near(integrate::simpson<f64>(xs, ys, 5), 1.0 / 3.0, 2e-14));
    f64 cumulative[5]{};
    integrate::cum_simpson<f64>(xs, ys, cumulative, 5);
    require_true(near(cumulative[4], 1.0 / 3.0, 2e-14));
    auto sixth_power = [](f64 x) noexcept {
      const f64 x2 = x * x;
      return x2 * x2 * x2;
    };
    require_true(near(integrate::derive::nth<6, f64>(sixth_power, 0.4, 0.02), 720.0, 2e-4));
  }
  end_test_case();

  test_case("adaptive Genz-Malik cubature");
  {
    const f64 lo[2] = { 0.0, 0.0 };
    const f64 hi[2] = { 1.0, 1.0 };
    integrate::cubature_workspace<2, f64, 32> workspace{};
    integrate::cubature_options<f64> options{};
    options.abs_tol = 1e-10;
    options.rel_tol = 1e-10;
    auto surface = [](const f64(&x)[2]) noexcept { return x[0] * x[0] + x[1] * x[1]; };
    auto value = integrate::cubature<2, f64>(surface, lo, hi, options, workspace);
    require_true(value.status == integrate::quad_status::ok);
    require_true(near(value.value, 2.0 / 3.0, 2e-12));

    usize batches = 0;
    auto surface_batch = [&](const f64 *const *x, f64 *out, usize count) noexcept {
      ++batches;
      for ( usize i = 0; i < count; ++i ) out[i] = x[0][i] * x[0][i] + x[1][i] * x[1][i];
    };
    integrate::cubature_batch_workspace<2, f64, 32, 7> batch_workspace{};
    auto batch_value = integrate::cubature_batch<2, f64>(surface_batch, lo, hi, options, batch_workspace);
    require_true(batch_value.status == integrate::quad_status::ok);
    require_true(near(batch_value.value, value.value, 2e-14));
    require_true(batch_value.n_evals == value.n_evals);
    require_true(batches > 1);
  }
  end_test_case();

  test_case("fixed and adaptive IVP, dense output, reverse time, and event");
  {
    using state = vec<f64, 2>;
    auto harmonic = [](f64, const state &y, state &dy) noexcept {
      dy.data[0] = y.data[1];
      dy.data[1] = -y.data[0];
    };
    const f64 half_pi = constant_pi<f64> * 0.5;
    state initial{ { 1.0, 0.0 } };
    integrate::ode::rk45_workspace<f64, 2> workspace{};
    integrate::ode::options<f64> options{};
    options.abs_tol = 1e-11;
    options.rel_tol = 1e-11;
    options.initial_step = 0.02;
    const f64 times[] = { 0.25, 0.5, 1.0 };
    state sampled[3]{};
    integrate::ode::output_buffer<f64, 2> output{ times, 3, sampled, 0 };
    auto event = integrate::ode::make_event([](f64, const state &y) noexcept { return y.data[0]; },
                                            integrate::ode::event_direction::falling, true, 7);
    auto solved = integrate::ode::solve_ivp<integrate::ode::method::rk45>(harmonic, 0.0, initial, 3.0, options, workspace,
                                                                          integrate::ode::null_observer{}, event, &output);
    require_true(solved.termination == integrate::ode::status::event);
    require_true(solved.event_index == 7);
    require_true(near(solved.event_time, half_pi, 2e-8));
    require_true(output.written == 3);
    require_true(near(sampled[1].data[0], math::cos<f64>(0.5), 2e-7));

    integrate::ode::dop853_workspace<f64, 2> dop_workspace{};
    auto forward = integrate::ode::solve_ivp<integrate::ode::method::dop853>(harmonic, 0.0, initial, half_pi, options, dop_workspace);
    require_true(forward.termination == integrate::ode::status::success);
    require_true(near(forward.y.data[0], 0.0, 2e-9));
    require_true(near(forward.y.data[1], -1.0, 2e-9));
    auto backward = integrate::ode::solve_ivp<integrate::ode::method::dop853>(harmonic, half_pi, forward.y, 0.0, options, dop_workspace);
    require_true(backward.termination == integrate::ode::status::success);
    require_true(near(backward.y.data[0], 1.0, 2e-9));

    f64 dynamic_state[2] = { 1.0, 0.0 };
    f64 stages[13 * 2]{}, temporary[2]{}, next[2]{}, error[2]{}, start[2]{}, start_f[2]{}, end_f[2]{};
    integrate::ode::runtime_workspace<f64> runtime{ stages, temporary, next, error, start, start_f, end_f, 2, 13 };
    auto dynamic_rhs = [](f64, const f64 *y, f64 *dy, usize) noexcept {
      dy[0] = y[1];
      dy[1] = -y[0];
    };
    auto dynamic = integrate::ode::solve_ivp<integrate::ode::method::rk45>(dynamic_rhs, 0.0, dynamic_state, 2, half_pi, options, runtime);
    require_true(dynamic.termination == integrate::ode::status::success);
    require_true(near(dynamic_state[0], 0.0, 2e-8));
    require_true(near(dynamic_state[1], -1.0, 2e-8));
  }
  end_test_case();

  return 1;
}
