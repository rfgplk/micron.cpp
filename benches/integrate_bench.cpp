//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Deterministic quadrature, sampled-data, differentiation, and stochastic
// work/precision benchmark. Build with:
//   duck build benches/integrate_bench.cpp --perf --fp --no-ssp --no-lto -i .

#include "_integrate_bench_common.hpp"

#include "../src/math/integrate/integrate.hpp"
#include "../src/math/integrate/monte_carlo.hpp"

namespace
{

namespace ib = integrate_bench;
namespace mi = micron::math::integrate;
namespace md = micron::math::integrate::derive;
namespace mk = micron::math::mk;

constexpr usize max_samples = 262144;
alignas(64) f32 samples_f32[max_samples];
alignas(64) f32 output_f32[max_samples];
alignas(64) f32 coordinates_f32[max_samples];
alignas(64) f64 samples_f64[max_samples];
alignas(64) f64 output_f64[max_samples];
alignas(64) f64 coordinates_f64[max_samples];
volatile f64 upper_bound = 1.0;

template<typename F>
[[gnu::always_inline]] inline F
polynomial(F x) noexcept
{
  return x * x + F(1);
}

template<typename F>
void
polynomial_batch(const F *x, F *y, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) y[i] = polynomial(x[i]);
}

template<typename F>
[[gnu::always_inline]] inline F
scalar_trapezoid(const F *values, usize count, F dx) noexcept
{
  if ( count < 2 ) return F(0);
  F total = F(0.5) * (values[0] + values[count - 1]);
  for ( usize i = 1; i + 1 < count; ++i ) total += values[i];
  return dx * total;
}

template<typename F>
inline void
scalar_prefix(const F *values, F *output, usize count, F dx) noexcept
{
  if ( count == 0 ) return;
  output[0] = F(0);
  for ( usize i = 1; i < count; ++i ) output[i] = output[i - 1] + F(0.5) * dx * (values[i - 1] + values[i]);
}

template<typename F>
inline void
scalar_difference(const F *values, F *output, usize count, F dx) noexcept
{
  if ( count == 0 ) return;
  if ( count == 1 ) {
    output[0] = F(0);
    return;
  }
  output[0] = (values[1] - values[0]) / dx;
  for ( usize i = 1; i + 1 < count; ++i ) output[i] = (values[i + 1] - values[i - 1]) / (F(2) * dx);
  output[count - 1] = (values[count - 1] - values[count - 2]) / dx;
}

void
initialize() noexcept
{
  u64 state = 0x494E544547524154ULL;
  f64 coordinate = 0;
  for ( usize i = 0; i < max_samples; ++i ) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    const f64 value = f64(state >> 11) * 0x1.0p-53;
    samples_f64[i] = value;
    samples_f32[i] = f32(value);
    coordinate += 0.0005 + 0.0005 * value;
    coordinates_f64[i] = coordinate;
    coordinates_f32[i] = f32(coordinate);
  }
}

template<typename F>
constexpr const char *
type_name() noexcept
{
  if constexpr ( sizeof(F) == 4 ) return "f32";
  return "f64";
}

template<typename F, usize Count, u64 Repetitions>
void
sampled_cases(F *values, F *output, const F *coordinates) noexcept
{
  constexpr F dx = F(0.001);
  const auto baseline_trap = ib::measure<Repetitions, Count>([&] {
    const F value = scalar_trapezoid(values, Count, dx);
    ib::consume(value);
  });
  ib::print("sampled", "trapezoid", "scalar_reference", type_name<F>(), Count, baseline_trap);
  const auto optimized_trap = ib::measure<Repetitions, Count>([&] {
    const F value = mi::trapezoid<F>(values, Count, dx);
    ib::consume(value);
  });
  ib::print("sampled", "trapezoid", "optimized", type_name<F>(), Count, optimized_trap);

  const auto simpson = ib::measure<Repetitions, Count>([&] {
    const F value = mi::simpson<F>(values, Count, dx);
    ib::consume(value);
  });
  ib::print("sampled", "simpson", "uniform", type_name<F>(), Count, simpson);
  const auto irregular_trap = ib::measure<Repetitions, Count>([&] {
    const F value = mi::trapezoid<F>(coordinates, values, Count);
    ib::consume(value);
  });
  ib::print("sampled", "trapezoid", "irregular", type_name<F>(), Count, irregular_trap);
  const auto irregular_simpson = ib::measure<Repetitions, Count>([&] {
    const F value = mi::simpson<F>(coordinates, values, Count);
    ib::consume(value);
  });
  ib::print("sampled", "simpson", "irregular", type_name<F>(), Count, irregular_simpson);

  const auto baseline_prefix = ib::measure<Repetitions, Count>([&] {
    scalar_prefix(values, output, Count, dx);
    ib::clobber(output);
  });
  ib::print("sampled", "cumulative_trapezoid", "scalar_reference", type_name<F>(), Count, baseline_prefix);
  const auto optimized_prefix = ib::measure<Repetitions, Count>([&] {
    mi::cum_trapezoid<F>(values, output, Count, dx);
    ib::clobber(output);
  });
  ib::print("sampled", "cumulative_trapezoid", "optimized", type_name<F>(), Count, optimized_prefix);
  const auto cumulative_simpson = ib::measure<Repetitions, Count>([&] {
    mi::cum_simpson<F>(values, output, Count, dx);
    ib::clobber(output);
  });
  ib::print("sampled", "cumulative_simpson", "uniform", type_name<F>(), Count, cumulative_simpson);
  const auto irregular_prefix = ib::measure<Repetitions, Count>([&] {
    mi::cum_trapezoid<F>(coordinates, values, output, Count);
    ib::clobber(output);
  });
  ib::print("sampled", "cumulative_trapezoid", "irregular", type_name<F>(), Count, irregular_prefix);
  const auto irregular_cumulative_simpson = ib::measure<Repetitions, Count>([&] {
    mi::cum_simpson<F>(coordinates, values, output, Count);
    ib::clobber(output);
  });
  ib::print("sampled", "cumulative_simpson", "irregular", type_name<F>(), Count, irregular_cumulative_simpson);

  const auto baseline_diff = ib::measure<Repetitions, Count>([&] {
    scalar_difference(values, output, Count, dx);
    ib::clobber(output);
  });
  ib::print("differentiate", "uniform_gradient", "scalar_reference", type_name<F>(), Count, baseline_diff);
  const auto optimized_diff = ib::measure<Repetitions, Count>([&] {
    md::diff<F>(values, output, Count, dx);
    ib::clobber(output);
  });
  ib::print("differentiate", "uniform_gradient", "optimized", type_name<F>(), Count, optimized_diff);
}

template<usize Order, typename F>
void
clenshaw_curtis_case() noexcept
{
  auto scalar = [](F x) noexcept { return polynomial(x); };
  auto batch = [](const F *x, F *y, usize count) noexcept { polynomial_batch(x, y, count); };
  constexpr u64 repetitions = 8192 / (Order + 1) + 1;
  const auto scalar_counters = ib::measure<repetitions, Order + 1>([&] {
    const F value = mi::clenshaw_curtis<Order, F>(scalar, F(-1), F(upper_bound));
    ib::consume(value);
  });
  ib::print("fixed", "clenshaw_curtis", "precomputed", type_name<F>(), Order, scalar_counters);
  const auto batch_counters = ib::measure<repetitions, Order + 1>([&] {
    const F value = mi::clenshaw_curtis_batch<Order, F>(batch, F(-1), F(upper_bound));
    ib::consume(value);
  });
  ib::print("fixed", "clenshaw_curtis", "precomputed_batch", type_name<F>(), Order, batch_counters);
}

template<usize Order, typename F>
void
gauss_case() noexcept
{
  auto scalar = [](F x) noexcept { return polynomial(x); };
  auto batch = [](const F *x, F *y, usize count) noexcept { polynomial_batch(x, y, count); };
  constexpr u64 repetitions = 8192 / Order + 1;
  const auto scalar_counters = ib::measure<repetitions, Order>([&] {
    const F value = mi::gauss_legendre<Order, F>(scalar, F(-1), F(upper_bound));
    ib::consume(value);
  });
  const F scalar_value = mi::gauss_legendre<Order, F>(scalar, F(-1), F(1));
  ib::print("fixed", "gauss_legendre", "scalar_callback", type_name<F>(), Order, scalar_counters,
            f64(mk::manip::fabs<F>(scalar_value - F(8.0L / 3.0L))));
  const auto batch_counters = ib::measure<repetitions, Order>([&] {
    const F value = mi::gauss_legendre_batch<Order, F>(batch, F(-1), F(upper_bound));
    ib::consume(value);
  });
  const F batch_value = mi::gauss_legendre_batch<Order, F>(batch, F(-1), F(1));
  ib::print("fixed", "gauss_legendre", "batch_callback", type_name<F>(), Order, batch_counters,
            f64(mk::manip::fabs<F>(batch_value - F(8.0L / 3.0L))));
}

template<usize Order>
void
newton_cotes_case() noexcept
{
  auto scalar = [](f64 x) noexcept { return polynomial(x); };
  const auto counters = ib::measure<1024, Order + 1>([&] {
    const f64 value = mi::newton_cotes<Order, f64>(scalar, 0.0, f64(upper_bound), Order * 2);
    ib::consume(value);
  });
  const f64 value = mi::newton_cotes<Order, f64>(scalar, 0.0, 1.0, Order * 2);
  ib::print("fixed", "newton_cotes", "optimized", "f64", Order, counters, mk::manip::fabs(value - 4.0 / 3.0));
  const auto open_counters = ib::measure<1024, Order + 1>([&] {
    const f64 open = mi::newton_cotes_open<Order, f64>(scalar, 0.0, f64(upper_bound));
    ib::consume(open);
  });
  const f64 open = mi::newton_cotes_open<Order, f64>(scalar, 0.0, 1.0);
  ib::print("fixed", "newton_cotes", "open", "f64", Order, open_counters, mk::manip::fabs(open - 4.0 / 3.0));
}

template<typename F>
void
fixed_cases() noexcept
{
  gauss_case<5, F>();
  gauss_case<8, F>();
  gauss_case<16, F>();
  gauss_case<32, F>();
  gauss_case<64, F>();
  auto transcendental = [](F x) noexcept { return mk::exp_ns::exp<F>(-x * x); };
  const auto transcendent = ib::measure<256, 32>([&] {
    const F value = mi::gauss_legendre<32, F>(transcendental, F(-1), F(upper_bound));
    ib::consume(value);
  });
  ib::print("fixed", "gauss_legendre_transcendental", "scalar_callback", type_name<F>(), 32, transcendent);

  auto scalar = [](F x) noexcept { return polynomial(x); };
  clenshaw_curtis_case<8, F>();
  clenshaw_curtis_case<16, F>();
  clenshaw_curtis_case<32, F>();
  clenshaw_curtis_case<64, F>();
  const auto cc17 = ib::measure<128, 18>([&] {
    const F value = mi::clenshaw_curtis<17, F>(scalar, F(-1), F(upper_bound));
    ib::consume(value);
  });
  ib::print("fixed", "clenshaw_curtis", "generic", type_name<F>(), 17, cc17);
  const auto hermite = ib::measure<256, 16>([&] {
    const F value = mi::gauss_hermite<16, F>(scalar);
    ib::consume(value);
  });
  ib::print("weighted", "gauss_hermite", "generated_rule", type_name<F>(), 16, hermite);
  const auto laguerre = ib::measure<256, 16>([&] {
    const F value = mi::gauss_laguerre<16, F>(scalar);
    ib::consume(value);
  });
  ib::print("weighted", "gauss_laguerre", "generated_rule", type_name<F>(), 16, laguerre);
  const auto jacobi = ib::measure<256, 16>([&] {
    const F value = mi::gauss_jacobi<16, F>(scalar, F(-0.25), F(0.5));
    ib::consume(value);
  });
  ib::print("weighted", "gauss_jacobi", "generated_rule", type_name<F>(), 16, jacobi);

  mi::hermite_rule<F, 16> hermite_rule{};
  mi::laguerre_rule<F, 16> laguerre_rule{};
  mi::jacobi_rule<F, 16> jacobi_rule{};
  hermite_rule.generate();
  (void)laguerre_rule.generate();
  (void)jacobi_rule.generate(F(-0.25), F(0.5));
  const auto reused_hermite = ib::measure<1024, 16>([&] { ib::consume(hermite_rule.apply(scalar)); });
  const auto reused_laguerre = ib::measure<1024, 16>([&] { ib::consume(laguerre_rule.apply(scalar)); });
  const auto reused_jacobi = ib::measure<1024, 16>([&] { ib::consume(jacobi_rule.apply(scalar)); });
  ib::print("weighted", "gauss_hermite", "reused_rule", type_name<F>(), 16, reused_hermite);
  ib::print("weighted", "gauss_laguerre", "reused_rule", type_name<F>(), 16, reused_laguerre);
  ib::print("weighted", "gauss_jacobi", "reused_rule", type_name<F>(), 16, reused_jacobi);
  const auto chebyshev_i = ib::measure<1024, 16>([&] { ib::consume(mi::gauss_chebyshev_i<16, F>(scalar)); });
  const auto chebyshev_ii = ib::measure<1024, 16>([&] { ib::consume(mi::gauss_chebyshev_ii<16, F>(scalar)); });
  ib::print("weighted", "gauss_chebyshev_i", "fixed", type_name<F>(), 16, chebyshev_i);
  ib::print("weighted", "gauss_chebyshev_ii", "fixed", type_name<F>(), 16, chebyshev_ii);
}

template<typename F>
void
adaptive_cases() noexcept
{
  mi::quad_options<F> options{};
  options.abs_tol = sizeof(F) <= 4 ? F(1e-5) : F(1e-8);
  options.rel_tol = sizeof(F) <= 4 ? F(1e-5) : F(1e-8);
  mi::quad_workspace<F, 256> workspace{};
  auto finite = [](F x) noexcept { return polynomial(x); };
  const auto finite_counters = ib::measure<512, 1>([&] {
    const auto value = mi::quad<F>(finite, F(0), F(upper_bound), options, workspace);
    ib::consume(value);
  });
  const auto finite_result = mi::quad<F>(finite, F(0), F(1), options, workspace);
  ib::print("adaptive", "finite_polynomial", "gk21_heap", type_name<F>(), 1, finite_counters,
            f64(mk::manip::fabs<F>(finite_result.value - F(4.0L / 3.0L))), f64(finite_result.abs_err), finite_result.n_evals);

  auto singular = [](F x) noexcept { return F(1) / mk::pow_ns::sqrt<F>(x); };
  const auto singular_counters = ib::measure<32, 1>([&] {
    const auto value = mi::quad<F>(singular, F(0), F(1), options, workspace);
    ib::consume(value);
  });
  const auto singular_result = mi::quad<F>(singular, F(0), F(1), options, workspace);
  ib::print("adaptive", "endpoint_singularity", "gk21_heap", type_name<F>(), 1, singular_counters,
            f64(mk::manip::fabs<F>(singular_result.value - F(2))), f64(singular_result.abs_err), singular_result.n_evals);

  mi::tanh_sinh_options<F> de{};
  de.abs_tol = options.abs_tol;
  de.rel_tol = options.rel_tol;
  const auto de_counters = ib::measure<32, 1>([&] {
    const auto value = mi::tanh_sinh<F>(singular, F(0), F(1), de);
    ib::consume(value);
  });
  const auto de_result = mi::tanh_sinh<F>(singular, F(0), F(1), de);
  ib::print("adaptive", "endpoint_singularity", "tanh_sinh", type_name<F>(), 1, de_counters,
            f64(mk::manip::fabs<F>(de_result.value - F(2))), f64(de_result.abs_err), de_result.n_evals);

  auto decay = [](F x) noexcept { return mk::exp_ns::exp<F>(-x); };
  const F infinity = F(__builtin_huge_val());
  const auto improper_counters = ib::measure<32, 1>([&] {
    const auto value = mi::quad<F>(decay, F(0), infinity, options, workspace);
    ib::consume(value);
  });
  const auto improper_result = mi::quad<F>(decay, F(0), infinity, options, workspace);
  ib::print("adaptive", "semi_infinite", "gk21_heap", type_name<F>(), 1, improper_counters,
            f64(mk::manip::fabs<F>(improper_result.value - F(1))), f64(improper_result.abs_err), improper_result.n_evals);

  auto one = [](F) noexcept { return F(1); };
  const auto oscillatory_counters = ib::measure<64, 1>([&] {
    const auto value = mi::quad_sin<F>(one, F(0), F(1), F(40), options, workspace);
    ib::consume(value);
  });
  const auto oscillatory_result = mi::quad_sin<F>(one, F(0), F(1), F(40), options, workspace);
  const F oscillatory_truth = (F(1) - mk::trig::cos<F>(F(40))) / F(40);
  ib::print("weighted", "oscillatory_sin", "adaptive", type_name<F>(), 40, oscillatory_counters,
            f64(mk::manip::fabs<F>(oscillatory_result.value - oscillatory_truth)), f64(oscillatory_result.abs_err),
            oscillatory_result.n_evals);
  const auto cosine_counters = ib::measure<64, 1>([&] {
    const auto value = mi::quad_cos<F>(one, F(0), F(1), F(40), options, workspace);
    ib::consume(value);
  });
  const auto cosine_result = mi::quad_cos<F>(one, F(0), F(1), F(40), options, workspace);
  const F cosine_truth = mk::trig::sin<F>(F(40)) / F(40);
  ib::print("weighted", "oscillatory_cos", "adaptive", type_name<F>(), 40, cosine_counters,
            f64(mk::manip::fabs<F>(cosine_result.value - cosine_truth)), f64(cosine_result.abs_err), cosine_result.n_evals);

  auto affine = [](F x) noexcept { return F(1) + x; };
  const auto cauchy_counters = ib::measure<128, 1>([&] {
    const auto value = mi::quad_cauchy<F>(affine, F(-1), F(1), F(0), options, workspace);
    ib::consume(value);
  });
  const auto cauchy_result = mi::quad_cauchy<F>(affine, F(-1), F(1), F(0), options, workspace);
  ib::print("weighted", "cauchy_principal_value", "symmetric_cancellation", type_name<F>(), 1, cauchy_counters,
            f64(mk::manip::fabs<F>(cauchy_result.value - F(2))), f64(cauchy_result.abs_err), cauchy_result.n_evals);

  const auto algebraic_counters = ib::measure<32, 1>([&] {
    const auto value = mi::quad_algebraic<F>(one, F(0), F(1), F(-0.5), F(0), de);
    ib::consume(value);
  });
  const auto algebraic_result = mi::quad_algebraic<F>(one, F(0), F(1), F(-0.5), F(0), de);
  ib::print("weighted", "algebraic_endpoint", "tanh_sinh", type_name<F>(), 1, algebraic_counters,
            f64(mk::manip::fabs<F>(algebraic_result.value - F(2))), f64(algebraic_result.abs_err), algebraic_result.n_evals);
  const auto logarithmic_counters = ib::measure<32, 1>([&] {
    const auto value = mi::quad_algebraic_log<F>(one, F(0), F(1), F(0), F(0), true, false, de);
    ib::consume(value);
  });
  const auto logarithmic_result = mi::quad_algebraic_log<F>(one, F(0), F(1), F(0), F(0), true, false, de);
  ib::print("weighted", "logarithmic_endpoint", "tanh_sinh", type_name<F>(), 1, logarithmic_counters,
            f64(mk::manip::fabs<F>(logarithmic_result.value + F(1))), f64(logarithmic_result.abs_err), logarithmic_result.n_evals);

  auto vector_function = [](F x) noexcept { return micron::math::vec<F, 2>{ { x, x * x } }; };
  mi::quad_vec_workspace<2, F, 128> vector_workspace{};
  const auto vector_counters = ib::measure<128, 1>([&] {
    const auto value = mi::quad_vec<2, F>(vector_function, F(0), F(upper_bound), options, vector_workspace);
    ib::consume(value);
  });
  const auto vector_result = mi::quad_vec<2, F>(vector_function, F(0), F(1), options, vector_workspace);
  ib::print("adaptive", "quad_vec2", "shared_evaluations", type_name<F>(), 2, vector_counters,
            f64(mk::manip::fabs<F>(vector_result.value.data[1] - F(1.0L / 3.0L))), f64(vector_result.error_norm), vector_result.n_evals);
}

void
cubature_and_derivative_cases() noexcept
{
  const f64 lower[2] = { 0, 0 }, upper[2] = { 1, 1 };
  auto surface = [](const f64(&x)[2]) noexcept { return x[0] * x[0] + x[1] * x[1]; };
  auto surface_batch = [](const f64 *const *x, f64 *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) y[i] = x[0][i] * x[0][i] + x[1][i] * x[1][i];
  };
  mi::cubature_options<f64> options{};
  options.abs_tol = 1e-9;
  options.rel_tol = 1e-9;
  mi::cubature_workspace<2, f64, 32> workspace{};
  const auto scalar_counters = ib::measure<64, 1>([&] {
    const auto value = mi::cubature<2, f64>(surface, lower, upper, options, workspace);
    ib::consume(value);
  });
  const auto scalar_result = mi::cubature<2, f64>(surface, lower, upper, options, workspace);
  ib::print("cubature", "genz_malik_d2", "scalar_callback", "f64", 2, scalar_counters, mk::manip::fabs(scalar_result.value - 2.0 / 3.0),
            scalar_result.abs_err, scalar_result.n_evals);
  mi::cubature_batch_workspace<2, f64, 32, 17> batch_workspace{};
  const auto batch_counters = ib::measure<64, 1>([&] {
    const auto value = mi::cubature_batch<2, f64>(surface_batch, lower, upper, options, batch_workspace);
    ib::consume(value);
  });
  const auto batch_result = mi::cubature_batch<2, f64>(surface_batch, lower, upper, options, batch_workspace);
  ib::print("cubature", "genz_malik_d2", "batch_callback", "f64", 2, batch_counters, mk::manip::fabs(batch_result.value - 2.0 / 3.0),
            batch_result.abs_err, batch_result.n_evals);

  auto scalar_1d = [](f64 x) noexcept { return mk::exp_ns::exp(x); };
  auto batch_1d = [](const f64 *x, f64 *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) y[i] = mk::exp_ns::exp(x[i]);
  };
  const auto derivative_scalar = ib::measure<2048, 9>([&] {
    const f64 value = md::central_nth<4, 9, f64>(scalar_1d, 0.25, 0.01);
    ib::consume(value);
  });
  ib::print("differentiate", "fourth_derivative", "scalar_callback", "f64", 9, derivative_scalar);
  const auto derivative_batch = ib::measure<2048, 9>([&] {
    const f64 value = md::central_nth_batch<4, 9, f64>(batch_1d, 0.25, 0.01);
    ib::consume(value);
  });
  ib::print("differentiate", "fourth_derivative", "batch_callback", "f64", 9, derivative_batch);

  md::derivative_batch_workspace<2, f64, 16> derivative_workspace{};
  const auto hessian_scalar = ib::measure<1024, 9>([&] {
    const auto value = md::hessian<2, f64>(surface, lower, 0.001);
    ib::consume(value);
  });
  ib::print("differentiate", "hessian_d2", "scalar_callback", "f64", 9, hessian_scalar);
  const auto hessian_batch = ib::measure<1024, 9>([&] {
    const auto value = md::hessian_batch<2, f64>(surface_batch, lower, 0.001, derivative_workspace);
    ib::consume(value);
  });
  ib::print("differentiate", "hessian_d2", "batch_callback", "f64", 9, hessian_batch);
}

void
stochastic_cases() noexcept
{
  const f64 lower[2] = { 0, 0 }, upper[2] = { 1, 1 };
  auto surface = [](const f64(&x)[2]) noexcept { return x[0] * x[0] + x[1] * x[1]; };
  auto surface_batch = [](const f64 *const *x, f64 *y, usize count) noexcept {
    for ( usize i = 0; i < count; ++i ) y[i] = x[0][i] * x[0][i] + x[1][i] * x[1][i];
  };
  constexpr usize count = 4096;
  const auto ordinary = ib::measure<8, count>([&] {
    auto generator = micron::math::rng::xoshiro256ss::from_seed(0x12345678ULL);
    const auto value = mi::monte_carlo_detailed<2, f64>(surface, lower, upper, count, generator);
    ib::consume(value);
  });
  auto generator = micron::math::rng::xoshiro256ss::from_seed(0x12345678ULL);
  const auto ordinary_result = mi::monte_carlo_detailed<2, f64>(surface, lower, upper, count, generator);
  ib::print("stochastic", "monte_carlo", "scalar_callback", "f64", count, ordinary, mk::manip::fabs(ordinary_result.estimate - 2.0 / 3.0),
            ordinary_result.standard_error, ordinary_result.evaluations);

  mi::monte_carlo_batch_workspace<2, f64, 64> batch_workspace{};
  const auto batched = ib::measure<8, count>([&] {
    auto batch_generator = micron::math::rng::xoshiro256ss::from_seed(0x12345678ULL);
    const auto value = mi::monte_carlo_batch<2, f64>(surface_batch, lower, upper, count, batch_generator, batch_workspace);
    ib::consume(value);
  });
  auto batch_generator = micron::math::rng::xoshiro256ss::from_seed(0x12345678ULL);
  const auto batch_result = mi::monte_carlo_batch<2, f64>(surface_batch, lower, upper, count, batch_generator, batch_workspace);
  ib::print("stochastic", "monte_carlo", "batch_callback", "f64", count, batched, mk::manip::fabs(batch_result.estimate - 2.0 / 3.0),
            batch_result.standard_error, batch_result.evaluations);

  const auto antithetic = ib::measure<8, count>([&] {
    auto antithetic_generator = micron::math::rng::xoshiro256ss::from_seed(0x12345678ULL);
    const auto value = mi::antithetic_monte_carlo<2, f64>(surface, lower, upper, count, antithetic_generator);
    ib::consume(value);
  });
  ib::print("stochastic", "monte_carlo", "antithetic", "f64", count, antithetic);
  const auto stratified = ib::measure<8, count>([&] {
    auto stratified_generator = micron::math::rng::xoshiro256ss::from_seed(0x12345678ULL);
    const auto value = mi::stratified_monte_carlo<2, f64>(surface, lower, upper, count, stratified_generator);
    ib::consume(value);
  });
  ib::print("stochastic", "monte_carlo", "stratified", "f64", count, stratified);
  const auto halton = ib::measure<8, count>([&] {
    const f64 value = mi::quasi_monte_carlo<2, f64>(surface, lower, upper, count);
    ib::consume(value);
  });
  ib::print("stochastic", "qmc", "halton", "f64", count, halton);
  const auto scrambled_halton = ib::measure<2, count * 4>([&] {
    const auto value = mi::scrambled_halton<2, f64>(surface, lower, upper, count, 4, 0x51514d43ULL);
    ib::consume(value);
  });
  ib::print("stochastic", "randomized_qmc", "scrambled_halton", "f64", count * 4, scrambled_halton);
  const auto scrambled_halton_batched = ib::measure<2, count * 4>([&] {
    const auto value = mi::scrambled_halton_batch<2, f64>(surface_batch, lower, upper, count, 4, 0x51514d43ULL, batch_workspace);
    ib::consume(value);
  });
  ib::print("stochastic", "randomized_qmc", "scrambled_halton_batch", "f64", count * 4, scrambled_halton_batched);
  const auto scrambled_sobol = ib::measure<2, count * 4>([&] {
    const auto value = mi::scrambled_sobol<2, f64>(surface, lower, upper, count, 4, 0x51514d43ULL);
    ib::consume(value);
  });
  ib::print("stochastic", "randomized_qmc", "scrambled_sobol", "f64", count * 4, scrambled_sobol);
  const auto scrambled_sobol_batched = ib::measure<2, count * 4>([&] {
    const auto value = mi::scrambled_sobol_batch<2, f64>(surface_batch, lower, upper, count, 4, 0x51514d43ULL, batch_workspace);
    ib::consume(value);
  });
  ib::print("stochastic", "randomized_qmc", "scrambled_sobol_batch", "f64", count * 4, scrambled_sobol_batched);
}

};      // namespace

int
main()
{
  ib::pin_cpu2();
  initialize();
  ib::header();
  sampled_cases<f32, 2048, 128>(samples_f32, output_f32, coordinates_f32);
  sampled_cases<f64, 2048, 128>(samples_f64, output_f64, coordinates_f64);
  sampled_cases<f32, 65536, 8>(samples_f32, output_f32, coordinates_f32);
  sampled_cases<f64, 65536, 8>(samples_f64, output_f64, coordinates_f64);
  sampled_cases<f32, max_samples, 1>(samples_f32, output_f32, coordinates_f32);
  sampled_cases<f64, max_samples, 1>(samples_f64, output_f64, coordinates_f64);
  fixed_cases<f32>();
  fixed_cases<f64>();
  newton_cotes_case<1>();
  newton_cotes_case<2>();
  newton_cotes_case<3>();
  newton_cotes_case<4>();
  newton_cotes_case<5>();
  newton_cotes_case<6>();
  newton_cotes_case<7>();
  newton_cotes_case<8>();
  adaptive_cases<f32>();
  adaptive_cases<f64>();
  cubature_and_derivative_cases();
  stochastic_cases();
  return 0;
}
