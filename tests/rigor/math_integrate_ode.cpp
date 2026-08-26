// math_integrate_ode.cpp — explicit IVP methods, dense output, and events

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
  return math::fabs(a - b) <= tolerance;
}

int
main()
{
  using state = vec<f64, 2>;
  auto exponential = [](f64, const state &y, state &out) noexcept {
    out.data[0] = y.data[0];
    out.data[1] = 0.0;
  };
  const state initial{ { 1.0, 3.0 } };
  const f64 e = math::exp<f64>(1.0);

  test_case("fixed Euler, midpoint, and RK4 establish their expected orders");
  {
    integrate::ode::fixed_workspace<f64, 2> workspace{};
    const auto eu = integrate::ode::euler(exponential, 0.0, initial, 1.0, 0.025, workspace);
    const auto mp = integrate::ode::midpoint(exponential, 0.0, initial, 1.0, 0.025, workspace);
    const auto rk = integrate::ode::rk4(exponential, 0.0, initial, 1.0, 0.025, workspace);
    require_true(eu.termination == integrate::ode::status::success);
    require_true(math::fabs(mp.y.data[0] - e) < math::fabs(eu.y.data[0] - e));
    require_true(math::fabs(rk.y.data[0] - e) < math::fabs(mp.y.data[0] - e));
    require_true(near(rk.y.data[0], e, 2e-8));
    require_true(rk.y.data[1] == 3.0);
  }
  end_test_case();

  test_case("RK23, RK45, and DOP853 satisfy a common work-precision contract");
  {
    integrate::ode::options<f64> options{};
    options.abs_tol = 1e-11;
    options.rel_tol = 1e-11;
    options.initial_step = 0.2;
    integrate::ode::rk23_workspace<f64, 2> w23{};
    integrate::ode::rk45_workspace<f64, 2> w45{};
    integrate::ode::dop853_workspace<f64, 2> w853{};
    const auto r23 = integrate::ode::rk23(exponential, 0.0, initial, 1.0, options, w23);
    const auto r45 = integrate::ode::rk45(exponential, 0.0, initial, 1.0, options, w45);
    const auto r853 = integrate::ode::dop853(exponential, 0.0, initial, 1.0, options, w853);
    require_true(r23.termination == integrate::ode::status::success);
    require_true(r45.termination == integrate::ode::status::success);
    require_true(r853.termination == integrate::ode::status::success);
    require_true(near(r23.y.data[0], e, 2e-10));
    require_true(near(r45.y.data[0], e, 2e-10));
    require_true(near(r853.y.data[0], e, 2e-10));
    require_true(r45.n_evals <= 7 * r45.attempted_steps);
    require_true(r853.n_evals <= 13 * r853.attempted_steps);

    const auto reverse = integrate::ode::rk45(exponential, 1.0, r45.y, 0.0, options, w45);
    require_true(reverse.termination == integrate::ode::status::success);
    require_true(near(reverse.y.data[0], 1.0, 3e-10));
  }
  end_test_case();

  test_case("terminal event truncates t_eval and the stored dense segment");
  {
    integrate::ode::options<f64> options{};
    options.abs_tol = 1e-12;
    options.rel_tol = 1e-12;
    options.initial_step = 0.4;
    integrate::ode::rk45_workspace<f64, 2> workspace{};
    const f64 times[] = { 0.0, 0.5, 0.7, 0.9 };
    state states[4]{};
    integrate::ode::output_buffer<f64, 2> output{ times, 4, states, 0 };
    integrate::ode::dense_segment<f64, 2> segments[64]{};
    integrate::ode::dense_buffer<f64, 2> dense{ segments, 64, 0 };
    usize observations = 0;
    auto observer = [&](f64, const state &) noexcept { ++observations; };
    auto event = integrate::ode::make_event([](f64, const state &y) noexcept { return y.data[0] - 2.0; },
                                            integrate::ode::event_direction::rising, true, 4);
    const auto result = integrate::ode::solve_ivp<integrate::ode::method::rk45>(exponential, 0.0, initial, 2.0, options, workspace,
                                                                                observer, event, &output, &dense);
    require_true(result.termination == integrate::ode::status::event);
    require_true(result.event_index == 4);
    require_true(near(result.event_time, math::log<f64>(2.0), 2e-9));
    require_true(output.written == 2);
    require_true(dense.written == result.accepted_steps);
    require_true(segments[dense.written - 1].t1 == result.event_time);
    require_true(near(segments[dense.written - 1].evaluate(result.event_time).data[0], 2.0, 2e-9));
    require_true(observations == result.accepted_steps + 1);
  }
  end_test_case();

  test_case("an event at the initial state terminates without stepping");
  {
    integrate::ode::options<f64> options{};
    integrate::ode::rk23_workspace<f64, 2> workspace{};
    auto event = integrate::ode::make_event([](f64, const state &y) noexcept { return y.data[0] - 1.0; });
    const auto result = integrate::ode::solve_ivp<integrate::ode::method::rk23>(exponential, 0.0, initial, 1.0, options, workspace,
                                                                                integrate::ode::null_observer{}, event);
    require_true(result.termination == integrate::ode::status::event);
    require_true(result.t == 0.0 && result.accepted_steps == 0 && result.n_evals == 0);
  }
  end_test_case();

  test_case("event localization preserves its bracket during backward integration");
  {
    integrate::ode::options<f64> options{};
    options.abs_tol = 1e-12;
    options.rel_tol = 1e-12;
    options.initial_step = 0.4;
    integrate::ode::rk45_workspace<f64, 2> workspace{};
    const state at_one{ { e, 3.0 } };
    auto event = integrate::ode::make_event([](f64, const state &y) noexcept { return y.data[0] - 2.0; },
                                            integrate::ode::event_direction::falling, true, 5);
    const auto result = integrate::ode::solve_ivp<integrate::ode::method::rk45>(exponential, 1.0, at_one, 0.0, options, workspace,
                                                                                integrate::ode::null_observer{}, event);
    require_true(result.termination == integrate::ode::status::event);
    require_true(result.event_index == 5);
    require_true(result.event_time >= 0.0 && result.event_time <= 1.0);
    require_true(near(result.event_time, math::log<f64>(2.0), 2e-9));
    require_true(near(result.y.data[0], 2.0, 2e-9));
  }
  end_test_case();

  test_case("runtime pointer state agrees with compile-time state");
  {
    integrate::ode::options<f64> options{};
    options.abs_tol = 1e-10;
    options.rel_tol = 1e-10;
    options.initial_step = 0.1;
    integrate::ode::rk45_workspace<f64, 2> fixed_workspace{};
    const auto fixed = integrate::ode::rk45(exponential, 0.0, initial, 1.0, options, fixed_workspace);

    f64 dynamic_state[2] = { 1.0, 3.0 };
    f64 stages[14]{}, temporary[2]{}, next[2]{}, error[2]{}, start[2]{}, start_f[2]{}, end_f[2]{};
    integrate::ode::runtime_workspace<f64> runtime{ stages, temporary, next, error, start, start_f, end_f, 2, 7 };
    auto rhs = [](f64, const f64 *y, f64 *out, usize) noexcept {
      out[0] = y[0];
      out[1] = 0.0;
    };
    const f64 times[] = { 0.0, 0.25, 1.0 };
    f64 sampled[6]{};
    integrate::ode::runtime_output_buffer<f64> output{ times, 3, sampled, 2, 0 };
    const auto dynamic = integrate::ode::solve_ivp<integrate::ode::method::rk45>(rhs, 0.0, dynamic_state, 2, 1.0, options, runtime,
                                                                                 integrate::ode::null_observer{}, &output);
    require_true(dynamic.termination == integrate::ode::status::success);
    require_true(dynamic.n_evals == fixed.n_evals);
    require_true(dynamic.accepted_steps == fixed.accepted_steps);
    require_true(dynamic_state[0] == fixed.y.data[0]);
    require_true(dynamic_state[1] == fixed.y.data[1]);
    require_true(output.written == 3);
    require_true(near(sampled[2], math::exp<f64>(0.25), 2e-7));
    require_true(sampled[3] == 3.0);
  }
  end_test_case();

  test_case("runtime pointer events truncate caller-owned dense segments");
  {
    integrate::ode::options<f64> options{};
    options.abs_tol = 1e-12;
    options.rel_tol = 1e-12;
    options.initial_step = 0.4;
    f64 dynamic_state[2] = { 1.0, 3.0 };
    f64 stages[14]{}, temporary[2]{}, next[2]{}, error[2]{}, start[2]{}, start_f[2]{}, end_f[2]{};
    integrate::ode::runtime_workspace<f64> workspace{ stages, temporary, next, error, start, start_f, end_f, 2, 7 };
    auto rhs = [](f64, const f64 *y, f64 *out, usize) noexcept {
      out[0] = y[0];
      out[1] = 0.0;
    };
    const f64 sample_times[] = { 0.0, 0.5, 0.8 };
    f64 sampled[6]{};
    integrate::ode::runtime_output_buffer<f64> output{ sample_times, 3, sampled, 2, 0 };
    f64 segment_times[128]{};
    f64 coefficients[64 * 4 * 2]{};
    integrate::ode::runtime_dense_buffer<f64> dense{ segment_times, coefficients, 64, 2, 0 };
    auto event = integrate::ode::make_event([](f64, const f64 *y, usize) noexcept { return y[0] - 2.0; },
                                            integrate::ode::event_direction::rising, true, 6);
    const auto result = integrate::ode::solve_ivp<integrate::ode::method::rk45>(rhs, 0.0, dynamic_state, 2, 2.0, options, workspace,
                                                                                integrate::ode::null_observer{}, event, &output, &dense);
    require_true(result.termination == integrate::ode::status::event);
    require_true(result.event_index == 6);
    require_true(near(result.event_time, math::log<f64>(2.0), 2e-9));
    require_true(near(dynamic_state[0], 2.0, 2e-9));
    require_true(output.written == 2);
    require_true(dense.written == result.accepted_steps);
    require_true(result.dense_segments_written == dense.written);
    f64 interpolated[2]{};
    dense.evaluate(dense.written - 1, result.event_time, interpolated, 2);
    require_true(near(interpolated[0], 2.0, 2e-9));
    require_true(interpolated[1] == 3.0);
  }
  end_test_case();

  test_case("invalid options, workspace, and explicit minimum step report distinct failures");
  {
    integrate::ode::rk45_workspace<f64, 2> workspace{};
    integrate::ode::options<f64> invalid{};
    invalid.min_factor = 2.0;
    const auto bad = integrate::ode::rk45(exponential, 0.0, initial, 1.0, invalid, workspace);
    require_true(bad.termination == integrate::ode::status::invalid_input);

    integrate::ode::options<f64> underflow{};
    underflow.initial_step = 0.01;
    underflow.min_step = 0.02;
    const auto small = integrate::ode::rk45(exponential, 0.0, initial, 1.0, underflow, workspace);
    require_true(small.termination == integrate::ode::status::step_underflow);
  }
  end_test_case();

  return 1;
}
