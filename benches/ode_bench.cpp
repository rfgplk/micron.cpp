//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Explicit IVP method, dense-output, event, and work/precision benchmark.
// Build with:
//   duck build benches/ode_bench.cpp --perf --fp --no-ssp --no-lto -i .

#include "_integrate_bench_common.hpp"

#include "../src/math/integrate/integrate.hpp"

namespace
{

namespace ib = integrate_bench;
namespace mo = micron::math::integrate::ode;
namespace mk = micron::math::mk;

template<typename F> using state2 = micron::math::vec<F, 2>;

template<typename F>
constexpr const char *
type_name() noexcept
{
  if constexpr ( sizeof(F) == 4 ) return "f32";
  return "f64";
}

template<typename F> struct harmonic_rhs {
  inline void
  operator()(F, const state2<F> &state, state2<F> &derivative) const noexcept
  {
    derivative.data[0] = state.data[1];
    derivative.data[1] = -state.data[0];
  }
};

template<typename F>
[[nodiscard]] state2<F>
reference_rk4(F t0, state2<F> state, F t_bound, F step) noexcept
{
  harmonic_rhs<F> rhs{};
  state2<F> k1{}, k2{}, k3{}, k4{}, temporary{};
  while ( t0 < t_bound ) {
    F h = step;
    if ( t0 + h > t_bound ) h = t_bound - t0;
    rhs(t0, state, k1);
    for ( usize i = 0; i < 2; ++i ) temporary.data[i] = state.data[i] + F(0.5) * h * k1.data[i];
    rhs(t0 + F(0.5) * h, temporary, k2);
    for ( usize i = 0; i < 2; ++i ) temporary.data[i] = state.data[i] + F(0.5) * h * k2.data[i];
    rhs(t0 + F(0.5) * h, temporary, k3);
    for ( usize i = 0; i < 2; ++i ) temporary.data[i] = state.data[i] + h * k3.data[i];
    rhs(t0 + h, temporary, k4);
    for ( usize i = 0; i < 2; ++i ) state.data[i] += h * (k1.data[i] + F(2) * k2.data[i] + F(2) * k3.data[i] + k4.data[i]) / F(6);
    t0 += h;
  }
  return state;
}

template<typename F>
[[nodiscard]] F
harmonic_error(const state2<F> &state, F time) noexcept
{
  const F x_error = state.data[0] - mk::trig::cos<F>(time);
  const F v_error = state.data[1] + mk::trig::sin<F>(time);
  return mk::pow_ns::sqrt<F>(x_error * x_error + v_error * v_error);
}

template<typename F>
void
fixed_cases() noexcept
{
  constexpr usize steps = 100;
  const F initial_data[2] = { F(1), F(0) };
  const state2<F> initial{ { initial_data[0], initial_data[1] } };
  harmonic_rhs<F> rhs{};
  const auto reference = ib::measure<2048, steps>([&] {
    const auto value = reference_rk4(F(0), initial, F(1), F(0.01));
    ib::consume(value);
  });
  const auto reference_value = reference_rk4(F(0), initial, F(1), F(0.01));
  ib::print("ode_fixed", "rk4", "scalar_reference", type_name<F>(), 2, reference, f64(harmonic_error(reference_value, F(1))), 0, 500, steps,
            0);

  mo::fixed_workspace<F, 2> workspace{};
  const auto euler_counters = ib::measure<2048, steps>([&] {
    const auto value = mo::euler(rhs, F(0), initial, F(1), F(0.01), workspace);
    ib::consume(value);
  });
  const auto euler_result = mo::euler(rhs, F(0), initial, F(1), F(0.01), workspace);
  ib::print("ode_fixed", "euler", "workspace", type_name<F>(), 2, euler_counters, f64(harmonic_error(euler_result.y, F(1))), 0,
            euler_result.n_evals, euler_result.accepted_steps, euler_result.rejected_steps);

  const auto midpoint_counters = ib::measure<2048, steps>([&] {
    const auto value = mo::midpoint(rhs, F(0), initial, F(1), F(0.01), workspace);
    ib::consume(value);
  });
  const auto midpoint_result = mo::midpoint(rhs, F(0), initial, F(1), F(0.01), workspace);
  ib::print("ode_fixed", "midpoint", "workspace", type_name<F>(), 2, midpoint_counters, f64(harmonic_error(midpoint_result.y, F(1))), 0,
            midpoint_result.n_evals, midpoint_result.accepted_steps, midpoint_result.rejected_steps);

  const auto rk4_counters = ib::measure<2048, steps>([&] {
    const auto value = mo::rk4(rhs, F(0), initial, F(1), F(0.01), workspace);
    ib::consume(value);
  });
  const auto rk4_result = mo::rk4(rhs, F(0), initial, F(1), F(0.01), workspace);
  ib::print("ode_fixed", "rk4", "workspace", type_name<F>(), 2, rk4_counters, f64(harmonic_error(rk4_result.y, F(1))), 0,
            rk4_result.n_evals, rk4_result.accepted_steps, rk4_result.rejected_steps);
}

template<mo::method Method, typename F>
void
adaptive_case(const char *name, F tolerance) noexcept
{
  harmonic_rhs<F> rhs{};
  const state2<F> initial{ { F(1), F(0) } };
  mo::options<F> options{};
  options.abs_tol = tolerance;
  options.rel_tol = tolerance;
  options.initial_step = F(0.05);
  if constexpr ( Method == mo::method::rk23 ) {
    mo::rk23_workspace<F, 2> workspace{};
    const auto counters = ib::measure<512, 1>([&] {
      const auto value = mo::rk23(rhs, F(0), initial, F(1), options, workspace);
      ib::consume(value);
    });
    const auto value = mo::rk23(rhs, F(0), initial, F(1), options, workspace);
    ib::print("ode_adaptive", name, "pi_controller", type_name<F>(), 2, counters, f64(harmonic_error(value.y, F(1))), f64(tolerance),
              value.n_evals, value.accepted_steps, value.rejected_steps);
  } else if constexpr ( Method == mo::method::rk45 ) {
    mo::rk45_workspace<F, 2> workspace{};
    const auto counters = ib::measure<512, 1>([&] {
      const auto value = mo::rk45(rhs, F(0), initial, F(1), options, workspace);
      ib::consume(value);
    });
    const auto value = mo::rk45(rhs, F(0), initial, F(1), options, workspace);
    ib::print("ode_adaptive", name, "pi_controller", type_name<F>(), 2, counters, f64(harmonic_error(value.y, F(1))), f64(tolerance),
              value.n_evals, value.accepted_steps, value.rejected_steps);
  } else {
    mo::dop853_workspace<F, 2> workspace{};
    const auto counters = ib::measure<256, 1>([&] {
      const auto value = mo::dop853(rhs, F(0), initial, F(1), options, workspace);
      ib::consume(value);
    });
    const auto value = mo::dop853(rhs, F(0), initial, F(1), options, workspace);
    ib::print("ode_adaptive", name, "pi_controller", type_name<F>(), 2, counters, f64(harmonic_error(value.y, F(1))), f64(tolerance),
              value.n_evals, value.accepted_steps, value.rejected_steps);
  }
}

alignas(64) state2<f64> dense_states[65];
alignas(64) mo::dense_segment<f64, 2> dense_segments[256];
alignas(64) f64 runtime_stages[26], runtime_temporary[2], runtime_next[2], runtime_error[2], runtime_start[2], runtime_start_f[2],
    runtime_end_f[2], runtime_states[130];

void
dense_event_runtime_cases() noexcept
{
  harmonic_rhs<f64> rhs{};
  const state2<f64> initial{ { 1, 0 } };
  mo::options<f64> options{};
  options.abs_tol = 1e-10;
  options.rel_tol = 1e-10;
  options.initial_step = 0.05;
  f64 times[65]{};
  for ( usize i = 0; i < 65; ++i ) times[i] = f64(i) / 64.0;
  mo::rk45_workspace<f64, 2> workspace{};
  const auto dense_counters = ib::measure<256, 1>([&] {
    mo::output_buffer<f64, 2> output{ times, 65, dense_states, 0 };
    mo::dense_buffer<f64, 2> dense{ dense_segments, 256, 0 };
    const auto value
        = mo::solve_ivp<mo::method::rk45>(rhs, 0.0, initial, 1.0, options, workspace, mo::null_observer{}, mo::no_event{}, &output, &dense);
    ib::consume(value);
    ib::clobber(dense_states);
  });
  mo::output_buffer<f64, 2> output{ times, 65, dense_states, 0 };
  mo::dense_buffer<f64, 2> dense{ dense_segments, 256, 0 };
  const auto dense_result
      = mo::solve_ivp<mo::method::rk45>(rhs, 0.0, initial, 1.0, options, workspace, mo::null_observer{}, mo::no_event{}, &output, &dense);
  ib::print("ode_feature", "dense_output_65", "rk45_cubic_hermite", "f64", 2, dense_counters, f64(harmonic_error(dense_states[64], f64(1))),
            0, dense_result.n_evals, dense_result.accepted_steps, dense_result.rejected_steps);

  auto event = mo::make_event([](f64, const state2<f64> &state) noexcept { return state.data[0]; }, mo::event_direction::falling, true, 0);
  const f64 half_pi = micron::math::constant_pi<f64> * 0.5;
  const auto event_counters = ib::measure<256, 1>([&] {
    const auto value = mo::solve_ivp<mo::method::rk45>(rhs, 0.0, initial, 3.0, options, workspace, mo::null_observer{}, event);
    ib::consume(value);
  });
  const auto event_result = mo::solve_ivp<mo::method::rk45>(rhs, 0.0, initial, 3.0, options, workspace, mo::null_observer{}, event);
  ib::print("ode_feature", "terminal_event", "brent_guarded", "f64", 2, event_counters, mk::manip::fabs(event_result.event_time - half_pi),
            0, event_result.n_evals, event_result.accepted_steps, event_result.rejected_steps);

  auto runtime_rhs = [](f64, const f64 *state, f64 *derivative, usize) noexcept {
    derivative[0] = state[1];
    derivative[1] = -state[0];
  };
  const auto runtime_counters = ib::measure<256, 1>([&] {
    f64 state[2] = { 1, 0 };
    mo::runtime_workspace<f64> runtime{
      runtime_stages, runtime_temporary, runtime_next, runtime_error, runtime_start, runtime_start_f, runtime_end_f, 2, 13
    };
    mo::runtime_output_buffer<f64> runtime_output{ times, 65, runtime_states, 2, 0 };
    const auto value
        = mo::solve_ivp<mo::method::rk45>(runtime_rhs, 0.0, state, 2, 1.0, options, runtime, mo::null_observer{}, &runtime_output);
    ib::consume(value);
    ib::clobber(runtime_states);
  });
  f64 state[2] = { 1, 0 };
  mo::runtime_workspace<f64> runtime{
    runtime_stages, runtime_temporary, runtime_next, runtime_error, runtime_start, runtime_start_f, runtime_end_f, 2, 13
  };
  const auto runtime_result = mo::rk45(runtime_rhs, 0.0, state, 2, 1.0, options, runtime);
  ib::print("ode_feature", "runtime_state", "rk45_pointer_length", "f64", 2, runtime_counters,
            f64(harmonic_error(state2<f64>{ { state[0], state[1] } }, f64(1))), 0, runtime_result.n_evals, runtime_result.accepted_steps,
            runtime_result.rejected_steps);
}

void
scientific_ivp_cases() noexcept
{
  using state4 = micron::math::vec<f64, 4>;
  auto kepler = [](f64, const state4 &state, state4 &derivative) noexcept {
    const f64 radius2 = state.data[0] * state.data[0] + state.data[1] * state.data[1];
    const f64 inverse_r3 = 1.0 / (radius2 * mk::pow_ns::sqrt(radius2));
    derivative.data[0] = state.data[2];
    derivative.data[1] = state.data[3];
    derivative.data[2] = -state.data[0] * inverse_r3;
    derivative.data[3] = -state.data[1] * inverse_r3;
  };
  constexpr f64 eccentricity = 0.1;
  const state4 initial{ { 1.0 - eccentricity, 0, 0, mk::pow_ns::sqrt((1.0 + eccentricity) / (1.0 - eccentricity)) } };
  mo::options<f64> options{};
  options.abs_tol = 1e-10;
  options.rel_tol = 1e-10;
  options.initial_step = 0.02;
  mo::dop853_workspace<f64, 4> workspace{};
  const f64 period = 2.0 * micron::math::constant_pi<f64>;
  const auto counters = ib::measure<64, 1>([&] {
    const auto value = mo::dop853(kepler, 0.0, initial, period, options, workspace);
    ib::consume(value);
  });
  const auto result = mo::dop853(kepler, 0.0, initial, period, options, workspace);
  f64 error = 0;
  for ( usize i = 0; i < 4; ++i ) {
    const f64 difference = result.y.data[i] - initial.data[i];
    error += difference * difference;
  }
  ib::print("ode_oracle", "kepler_period", "dop853", "f64", 4, counters, mk::pow_ns::sqrt(error), 1e-10, result.n_evals,
            result.accepted_steps, result.rejected_steps);

  harmonic_rhs<f64> harmonic{};
  const state2<f64> harmonic_initial{ { 1, 0 } };
  mo::rk45_workspace<f64, 2> reverse_workspace{};
  const auto forward = mo::rk45(harmonic, 0.0, harmonic_initial, 1.0, options, reverse_workspace);
  const auto reverse_counters = ib::measure<256, 1>([&] {
    const auto value = mo::rk45(harmonic, 1.0, forward.y, 0.0, options, reverse_workspace);
    ib::consume(value);
  });
  const auto reverse = mo::rk45(harmonic, 1.0, forward.y, 0.0, options, reverse_workspace);
  const f64 reverse_error = mk::pow_ns::sqrt((reverse.y.data[0] - 1.0) * (reverse.y.data[0] - 1.0) + reverse.y.data[1] * reverse.y.data[1]);
  ib::print("ode_oracle", "reverse_time", "rk45", "f64", 2, reverse_counters, reverse_error, 1e-10, reverse.n_evals, reverse.accepted_steps,
            reverse.rejected_steps);

  using state3 = micron::math::vec<f64, 3>;
  auto lorenz = [](f64, const state3 &state, state3 &derivative) noexcept {
    derivative.data[0] = 10.0 * (state.data[1] - state.data[0]);
    derivative.data[1] = state.data[0] * (28.0 - state.data[2]) - state.data[1];
    derivative.data[2] = state.data[0] * state.data[1] - (8.0 / 3.0) * state.data[2];
  };
  const state3 lorenz_initial{ { 1, 1, 1 } };
  mo::rk45_workspace<f64, 3> lorenz_workspace{};
  const auto lorenz_counters = ib::measure<128, 1>([&] {
    const auto value = mo::rk45(lorenz, 0.0, lorenz_initial, 1.0, options, lorenz_workspace);
    ib::consume(value);
  });
  const auto lorenz_result = mo::rk45(lorenz, 0.0, lorenz_initial, 1.0, options, lorenz_workspace);
  ib::print("ode_oracle", "lorenz_t1", "rk45", "f64", 3, lorenz_counters, 0, 1e-10, lorenz_result.n_evals, lorenz_result.accepted_steps,
            lorenz_result.rejected_steps);
}

};      // namespace

int
main()
{
  ib::pin_cpu2();
  ib::header();
  fixed_cases<f32>();
  fixed_cases<f64>();
  adaptive_case<mo::method::rk23, f32>("rk23", 1e-5f);
  adaptive_case<mo::method::rk45, f32>("rk45", 1e-5f);
  adaptive_case<mo::method::dop853, f32>("dop853", 1e-5f);
  adaptive_case<mo::method::rk23, f64>("rk23_tol1e-4", 1e-4);
  adaptive_case<mo::method::rk45, f64>("rk45_tol1e-4", 1e-4);
  adaptive_case<mo::method::dop853, f64>("dop853_tol1e-4", 1e-4);
  adaptive_case<mo::method::rk23, f64>("rk23_tol1e-8", 1e-8);
  adaptive_case<mo::method::rk45, f64>("rk45_tol1e-8", 1e-8);
  adaptive_case<mo::method::dop853, f64>("dop853_tol1e-8", 1e-8);
  adaptive_case<mo::method::rk23, f64>("rk23_tol1e-12", 1e-12);
  adaptive_case<mo::method::rk45, f64>("rk45_tol1e-12", 1e-12);
  adaptive_case<mo::method::dop853, f64>("dop853_tol1e-12", 1e-12);
  dense_event_runtime_cases();
  scientific_ivp_cases();
  return 0;
}
