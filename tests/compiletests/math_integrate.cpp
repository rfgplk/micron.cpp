//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Compile-validity gate for deterministic integration, differentiation,
// stochastic quadrature, and explicit IVP surfaces. Not run.

#include "../../src/array.hpp"
#include "../../src/math/integrate/integrate.hpp"
#include "../../src/math/integrate/monte_carlo.hpp"

namespace mi = micron::math::integrate;
namespace md = micron::math::integrate::derive;
namespace mo = micron::math::integrate::ode;

using micron::math::mat;
using micron::math::vec;

static_assert(static_cast<u32>(mi::quad_status::ok) == 0);
static_assert(static_cast<u32>(mi::quad_status::max_depth) == 1);
static_assert(static_cast<u32>(mi::quad_status::abnormal) == 2);
static_assert(mi::closed_newton_cotes_weight<1, f64>(0) == 0.5);
static_assert(mi::cubature_workspace<16, f32, 1>::capacity == 1);

template<typename F>
static F
touch_integrate() noexcept
{
  auto scalar = [](F x) noexcept -> F { return x * x + F(1); };
  auto batch = [](const F *x, F *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) y[i] = x[i] * x[i] + F(1);
  };
  auto scalar_d = [](const F(&x)[2]) noexcept -> F { return x[0] * x[0] + x[1]; };
  auto batch_d = [](const F *const *x, F *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) y[i] = x[0][i] * x[0][i] + x[1][i];
  };
  auto vector_1d = [](F x) noexcept -> vec<F, 2> { return vec<F, 2>{ { x + F(1), x * x } }; };
  auto vector_d = [](const F(&x)[2]) noexcept -> vec<F, 2> { return vec<F, 2>{ { x[0] + x[1], x[0] * x[1] } }; };
  auto vector_batch_d = [](const F *const *x, F *const *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) {
      y[0][i] = x[0][i] + x[1][i];
      y[1][i] = x[0][i] * x[1][i];
    }
  };

  F values[9]{}, coordinates[9]{}, cumulative[9]{}, derivative[9]{};
  micron::array<F, 9> values_container{}, coordinates_container{}, cumulative_container{};
  for ( usize i = 0; i < 9; ++i ) {
    coordinates[i] = F(i) / F(8);
    values[i] = scalar(coordinates[i]);
    coordinates_container[i] = coordinates[i];
    values_container[i] = values[i];
  }

  F sink = mi::trapezoid<F>(scalar, F(0), F(1), 8);
  sink += mi::trapezoid<F, mi::accumulation_policy::accurate>(values, 9, F(0.125));
  sink += mi::trapezoid<F>(coordinates, values, 9);
  sink += mi::trapezoid(values_container, F(0.125));
  sink += mi::simpson<F>(scalar, F(0), F(1), 8);
  sink += mi::simpson_38<F>(scalar, F(0), F(1), 9);
  sink += mi::simpson<F>(values, 9, F(0.125));
  sink += mi::simpson<F>(coordinates, values, 9);
  sink += mi::simpson(coordinates_container, values_container);
  sink += mi::integrate_samples<F>(coordinates, values, 9);
  sink += mi::integrate_samples(coordinates_container, values_container);
  mi::cum_trapezoid<F>(coordinates, values, cumulative, 9);
  mi::cum_trapezoid<F>(values, cumulative, 9, F(0.125));
  mi::cum_trapezoid(coordinates_container, values_container, cumulative_container);
  mi::cum_trapezoid(values_container, cumulative_container, F(0.125));
  mi::cum_simpson<F>(coordinates, values, cumulative, 9);
  mi::cum_simpson<F>(values, cumulative, 9, F(0.125));
  mi::cum_simpson(coordinates_container, values_container, cumulative_container);
  mi::cum_simpson(values_container, cumulative_container, F(0.125));
  sink += mi::sampled_romberg<F>(values, 9, F(0.125));
  sink += mi::romberg<F>(values, 9, F(0.125));
  sink += mi::sampled_romberg(values_container, F(0.125));
  sink += mi::romberg<F>(scalar, F(0), F(1), F(1e-5), 8);
  sink += mi::richardson::extrapolate<F>([&](F h) noexcept { return mi::trapezoid<F>(scalar, F(0), F(1), usize(F(1) / h)); }, F(0.25), F(2),
                                         2, 4, F(1e-4));

  sink += mi::gauss_legendre<5, F>(scalar, F(-1), F(1));
  sink += mi::gauss_legendre<8, F>(scalar, F(-1), F(1), mi::accumulation_policy::accurate);
  sink += mi::gauss_legendre<16, F>(scalar, F(-1), F(1));
  sink += mi::gauss_legendre<32, F>(scalar, F(-1), F(1));
  sink += mi::gauss_legendre<64, F>(scalar, F(-1), F(1));
  sink += mi::gauss_legendre_batch<8, F>(batch, F(-1), F(1));
  sink += mi::clenshaw_curtis<8, F>(scalar, F(-1), F(1));
  sink += mi::clenshaw_curtis<17, F>(scalar, F(-1), F(1));
  sink += mi::clenshaw_curtis_batch<16, F>(batch, F(-1), F(1), mi::accumulation_policy::accurate);

  mi::hermite_rule<F, 5> hermite{};
  hermite.generate();
  sink += hermite.apply(scalar) + hermite.apply_batch(batch);
  mi::laguerre_rule<F, 5> laguerre{};
  (void)laguerre.generate(F(0.25));
  sink += laguerre.apply(scalar);
  mi::jacobi_rule<F, 5> jacobi{};
  (void)jacobi.generate(F(-0.25), F(0.5));
  sink += jacobi.apply(scalar);
  sink += mi::gauss_hermite<8, F>(scalar);
  sink += mi::gauss_laguerre<8, F>(scalar, F(0));
  sink += mi::gauss_chebyshev_i<8, F>(scalar);
  sink += mi::gauss_chebyshev_ii<8, F>(scalar);
  sink += mi::gauss_jacobi<8, F>(scalar, F(0), F(0));
  sink += mi::newton_cotes<8, F>(scalar, F(0), F(1), 16);
  sink += mi::newton_cotes_open<8, F>(scalar, F(0), F(1));
  sink += mi::boole<F>(scalar, F(0), F(1));

  mi::quad_options<F> qoptions{};
  qoptions.abs_tol = F(1e-5);
  qoptions.rel_tol = F(1e-5);
  qoptions.rule = mi::gauss_kronrod_rule::gk15;
  mi::quad_workspace<F, 32> qworkspace{};
  sink += mi::quad<F>(scalar, F(0), F(1), qoptions, qworkspace).value;
  sink += mi::quad<F>(scalar, F(0), F(1), qworkspace, qoptions).value;
  sink += mi::quad<F>(scalar, F(0), F(1), F(1e-5), F(1e-5)).value;
  sink += mi::quad_batch<F>(batch, F(0), F(1), qoptions, qworkspace).value;
  mi::quad_vec_workspace<2, F, 32> qvworkspace{};
  sink += mi::quad_vec<2, F>(vector_1d, F(0), F(1), qoptions, qvworkspace, mi::quad_error_norm::l2).value.data[0];
  sink += mi::quad_vec<2, F>(vector_1d, F(0), F(1), qoptions, qworkspace).value.data[1];

  mi::tanh_sinh_options<F> deoptions{};
  sink += mi::tanh_sinh<F>(scalar, F(0), F(1), deoptions).value;
  sink += mi::tanh_sinh<F>(scalar, F(0), F(1), F(1e-5), F(1e-5)).value;
  sink += mi::quad_sin<F>(scalar, F(0), F(1), F(2), qoptions, qworkspace).value;
  sink += mi::quad_cos<F>(scalar, F(0), F(1), F(2), F(1e-5), F(1e-5)).value;
  sink += mi::quad_cauchy<F>(scalar, F(-1), F(1), F(0), qoptions, qworkspace).value;
  sink += mi::quad_algebraic<F>(scalar, F(0), F(1), F(0.25), F(0.5), deoptions).value;
  sink += mi::quad_algebraic_log<F>(scalar, F(0), F(1), F(0), F(0), true, true, deoptions).value;

  const F lower[2] = { F(0), F(0) };
  const F upper[2] = { F(1), F(1) };
  mi::cubature_options<F> coptions{};
  mi::cubature_workspace<2, F, 16> cworkspace{};
  sink += mi::cubature<2, F>(scalar_d, lower, upper, coptions, cworkspace).value;
  sink += mi::cubature<2, F>(scalar_d, lower, upper, F(1e-4), F(1e-4)).value;
  sink += mi::cubature_batch<2, F>(batch_d, lower, upper, coptions, cworkspace).value;
  mi::cubature_batch_workspace<2, F, 16, 7> cbworkspace{};
  sink += mi::cubature_batch<2, F>(batch_d, lower, upper, coptions, cbworkspace).value;
  sink += mi::dblquad<F>([](F x, F y) noexcept { return x + y; }, F(0), F(1), F(0), F(1), F(1e-4), F(1e-4)).value;
  sink += mi::tplquad<F>([](F x, F y, F z) noexcept { return x + y + z; }, F(0), F(1), F(0), F(1), F(0), F(1), F(1e-3), F(1e-3)).value;
  sink += mi::nquad<2, F>(scalar_d, lower, upper, F(1e-4), F(1e-4)).value;

  F nodes[5] = { F(-2), F(-1), F(0), F(1), F(2) }, weights[5]{};
  md::fornberg_workspace<F, 5, 4> fworkspace{};
  (void)md::fornberg_weights(nodes, 5, F(0), 2, weights, fworkspace);
  sink += md::forward<F>(scalar, F(0.5), F(1e-3));
  sink += md::backward<F>(scalar, F(0.5), F(1e-3));
  sink += md::central<F>(scalar, F(0.5), F(1e-3));
  sink += md::central4<F>(scalar, F(0.5), F(1e-3));
  sink += md::central_nth<4, 9, F>(scalar, F(0.5), F(0.02));
  sink += md::forward_nth<4, 9, F>(scalar, F(0.5), F(0.02));
  sink += md::backward_nth<4, 9, F>(scalar, F(0.5), F(0.02));
  sink += md::nth<12, F>(scalar, F(0.5), F(0.05));
  sink += md::central_nth_batch<4, 9, F>(batch, F(0.5), F(0.02));
  sink += md::adaptive<F>(scalar, F(0.5));
  sink += md::adaptive_diagnostic<F>(scalar, F(0.5)).value;
  sink += md::adaptive_ex<F>(scalar, F(0.5)).abs_err;
  sink += md::gradient<2, F>(scalar_d, lower, F(1e-3)).data[0];
  sink += md::jacobian<2, 2, F>(vector_d, lower, F(1e-3)).data[0];
  sink += md::hessian<2, F>(scalar_d, lower, F(1e-3)).data[0];
  md::derivative_batch_workspace<2, F, 16> dbworkspace{};
  md::jacobian_batch_workspace<2, 2, F, 16> jbworkspace{};
  sink += md::gradient_batch<2, F>(batch_d, lower, F(1e-3), dbworkspace).data[0];
  sink += md::jacobian_batch<2, 2, F>(vector_batch_d, lower, F(1e-3), jbworkspace).data[0];
  sink += md::hessian_batch<2, F>(batch_d, lower, F(1e-3), dbworkspace).data[0];
  md::diff<F>(values, derivative, 9, F(0.125));
  md::diff<F>(coordinates, values, derivative, 9);
  md::gradient_irregular<F>(coordinates, values, derivative, 9);
  md::diff2<F>(coordinates, values, derivative, 9);
  return sink + cumulative[8] + derivative[4];
}

template<typename F>
static F
touch_stochastic() noexcept
{
  const F lower[2] = { F(0), F(0) };
  const F upper[2] = { F(1), F(1) };
  auto scalar = [](const F(&x)[2]) noexcept -> F { return x[0] + x[1]; };
  auto batch = [](const F *const *x, F *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) y[i] = x[0][i] + x[1][i];
  };
  micron::math::rng::xoshiro256ss generator = micron::math::rng::xoshiro256ss::from_seed(0x1234ULL);
  F sink = mi::monte_carlo<2, F>(scalar, lower, upper, 17, generator);
  sink += mi::monte_carlo_detailed<2, F>(scalar, lower, upper, 17, generator).estimate;
  sink += mi::monte_carlo_stats<2, F>(scalar, lower, upper, 17, generator).standard_error;
  sink += mi::antithetic_monte_carlo<2, F>(scalar, lower, upper, 18, generator).estimate;
  sink += mi::stratified_monte_carlo<2, F>(scalar, lower, upper, 17, generator).estimate;
  sink += mi::quasi_monte_carlo<2, F>(scalar, lower, upper, 17);
  sink += mi::scrambled_halton<2, F>(scalar, lower, upper, 17, 3, 7).estimate;
  sink += mi::scrambled_sobol<2, F>(scalar, lower, upper, 17, 3, 7).estimate;
  mi::monte_carlo_batch_workspace<2, F, 7> workspace{};
  sink += mi::monte_carlo_batch_detailed<2, F>(batch, lower, upper, 17, generator, workspace).estimate;
  sink += mi::monte_carlo_batch<2, F>(batch, lower, upper, 17, generator, workspace).estimate;
  sink += mi::scrambled_halton_batch<2, F>(batch, lower, upper, 17, 3, 7, workspace).estimate;
  sink += mi::scrambled_sobol_batch<2, F>(batch, lower, upper, 17, 3, 7, workspace).estimate;
  return sink;
}

template<typename F>
static F
touch_ode() noexcept
{
  using state = vec<F, 2>;
  auto rhs = [](F, const state &y, state &out) noexcept {
    out.data[0] = y.data[1];
    out.data[1] = -y.data[0];
  };
  const state initial{ { F(1), F(0) } };
  mo::fixed_workspace<F, 2> fixed{};
  F sink = mo::euler(rhs, F(0), initial, F(0.1), F(0.01), fixed).y.data[0];
  sink += mo::midpoint(rhs, F(0), initial, F(0.1), F(0.01), fixed).y.data[0];
  sink += mo::rk4(rhs, F(0), initial, F(0.1), F(0.01), fixed).y.data[0];
  sink += mo::euler_step(rhs, F(0), initial, F(0.01), fixed).data[0];
  sink += mo::midpoint_step(rhs, F(0), initial, F(0.01), fixed).data[0];
  sink += mo::rk4_step(rhs, F(0), initial, F(0.01), fixed).data[0];

  mo::options<F> options{};
  options.abs_tol = F(1e-5);
  options.rel_tol = F(1e-5);
  options.initial_step = F(0.02);
  mo::rk23_workspace<F, 2> w23{};
  mo::rk45_workspace<F, 2> w45{};
  mo::dop853_workspace<F, 2> w853{};
  sink += mo::rk23(rhs, F(0), initial, F(0.1), options, w23).y.data[0];
  sink += mo::rk45(rhs, F(0), initial, F(0.1), options, w45).y.data[0];
  sink += mo::dop853(rhs, F(0), initial, F(0.1), options, w853).y.data[0];

  const F times[2] = { F(0), F(0.1) };
  state sampled[2]{};
  mo::output_buffer<F, 2> output{ times, 2, sampled, 0 };
  mo::dense_segment<F, 2> segments[8]{};
  mo::dense_buffer<F, 2> dense{ segments, 8, 0 };
  auto observer = [](F, const state &) noexcept { };
  auto event = mo::make_event([](F, const state &y) noexcept { return y.data[0] - F(0.5); }, mo::event_direction::falling, true, 3);
  sink += mo::solve_ivp<mo::method::rk45>(rhs, F(0), initial, F(0.1), options, w45, observer, event, &output, &dense).y.data[0];
  sink += mo::solve<mo::method::rk23>(rhs, F(0), initial, F(0.1), options, w23).y.data[0];
  sink += segments[0].evaluate(F(0.05)).data[0];

  F runtime_state[2] = { F(1), F(0) };
  F stages[26]{}, temporary[2]{}, next[2]{}, error[2]{}, start[2]{}, start_f[2]{}, end_f[2]{};
  mo::runtime_workspace<F> runtime{ stages, temporary, next, error, start, start_f, end_f, 2, 13 };
  auto runtime_rhs = [](F, const F *y, F *out, usize) noexcept {
    out[0] = y[1];
    out[1] = -y[0];
  };
  F runtime_samples[4]{};
  mo::runtime_output_buffer<F> runtime_output{ times, 2, runtime_samples, 2, 0 };
  F runtime_dense_times[8]{}, runtime_dense_coefficients[4 * 4 * 2]{};
  mo::runtime_dense_buffer<F> runtime_dense{ runtime_dense_times, runtime_dense_coefficients, 4, 2, 0 };
  sink += F(mo::solve_ivp<mo::method::rk23>(runtime_rhs, F(0), runtime_state, 2, F(0.1), options, runtime).accepted_steps);
  sink += F(mo::rk45(runtime_rhs, F(0), runtime_state, 2, F(0.1), options, runtime).accepted_steps);
  sink += F(mo::dop853(runtime_rhs, F(0), runtime_state, 2, F(0.1), options, runtime).accepted_steps);
  sink += F(
      mo::solve_ivp<mo::method::rk45>(runtime_rhs, F(0), runtime_state, 2, F(0.1), options, runtime, mo::null_observer{}, &runtime_output)
          .accepted_steps);
  auto runtime_event = mo::make_event([](F, const F *y, usize) noexcept { return y[0] - F(0.5); }, mo::event_direction::falling, true, 4);
  sink += F(mo::solve_ivp<mo::method::rk45>(runtime_rhs, F(0), runtime_state, 2, F(0.1), options, runtime, mo::null_observer{},
                                            runtime_event, &runtime_output, &runtime_dense)
                .accepted_steps);
  sink += F(mo::euler(runtime_rhs, F(0), runtime_state, 2, F(0.1), F(0.01), runtime).accepted_steps);
  sink += F(mo::midpoint(runtime_rhs, F(0), runtime_state, 2, F(0.1), F(0.01), runtime).accepted_steps);
  sink += F(mo::rk4(runtime_rhs, F(0), runtime_state, 2, F(0.1), F(0.01), runtime).accepted_steps);
  runtime.dense_evaluate(F(0.05), runtime_samples, 2);
  return sink + runtime_samples[0];
}

int
main()
{
  const f64 result = f64(touch_integrate<f32>()) + touch_integrate<f64>() + f64(touch_stochastic<f32>()) + touch_stochastic<f64>()
                     + f64(touch_ode<f32>()) + touch_ode<f64>();
  return static_cast<int>(result);
}
