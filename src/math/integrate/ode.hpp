//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../quants/vec.hpp"
#include "common.hpp"

namespace micron
{
namespace math
{
namespace integrate
{
namespace ode
{

enum class method : u32 { euler = 0, midpoint = 1, rk4 = 2, rk23 = 3, rk45 = 4, dop853 = 5 };

enum class status : u32 {
  success = 0,
  event = 1,
  max_steps = 2,
  step_underflow = 3,
  rejection_limit = 4,
  non_finite = 5,
  invalid_input = 6,
  storage_full = 7,
};

enum class event_direction : i32 { any = 0, rising = 1, falling = -1 };

template<ieee754_floating F> struct options {
  union {
    F abs_tol{ sizeof(F) <= 4 ? F(1e-6L) : F(1e-9L) };
    F atol;
  };

  union {
    F rel_tol{ sizeof(F) <= 4 ? F(1e-4L) : F(1e-7L) };
    F rtol;
  };

  F initial_step{ 0 };
  F min_step{ 0 };
  F max_step{ 0 };
  F safety{ F(0.9) };
  F min_factor{ F(0.2) };
  F max_factor{ F(10) };
  F pi_beta{ F(0.04) };
  usize max_steps{ 100000 };
  usize rejection_limit{ 32 };
};

template<ieee754_floating F, usize N> struct dense_segment {
  F t0{ 0 };
  F t1{ 0 };
  vec<F, N> y0{};
  vec<F, N> y1{};
  vec<F, N> f0{};
  vec<F, N> f1{};

  [[nodiscard]] inline vec<F, N>
  evaluate(F t) const noexcept
  {
    vec<F, N> output{};
    const F h = t1 - t0;
    if ( h == F(0) ) return y1;
    F theta = (t - t0) / h;
    if ( theta < F(0) ) theta = F(0);
    if ( theta > F(1) ) theta = F(1);
    const F theta2 = theta * theta;
    const F theta3 = theta2 * theta;
    const F h00 = F(2) * theta3 - F(3) * theta2 + F(1);
    const F h10 = theta3 - F(2) * theta2 + theta;
    const F h01 = -F(2) * theta3 + F(3) * theta2;
    const F h11 = theta3 - theta2;
    for ( usize i = 0; i < N; ++i ) output.data[i] = h00 * y0.data[i] + h * h10 * f0.data[i] + h01 * y1.data[i] + h * h11 * f1.data[i];
    return output;
  }
};

template<ieee754_floating F, usize N> struct output_buffer {
  const F *times{ nullptr };
  usize count{ 0 };
  vec<F, N> *states{ nullptr };
  usize written{ 0 };
};

template<ieee754_floating F, usize N> struct dense_buffer {
  dense_segment<F, N> *segments{ nullptr };
  usize capacity{ 0 };
  usize written{ 0 };
};

template<ieee754_floating F, usize N> struct result {
  F t{ 0 };
  vec<F, N> y{};
  usize n_evals{ 0 };
  usize accepted_steps{ 0 };
  usize rejected_steps{ 0 };
  usize attempted_steps{ 0 };
  status termination{ status::success };
  usize event_index{ usize(-1) };
  F event_time{ 0 };
  usize t_eval_written{ 0 };
  usize dense_segments_written{ 0 };
  F last_step{ 0 };
};

template<ieee754_floating F, usize N> using ivp_result = result<F, N>;

template<ieee754_floating F, usize N> struct fixed_workspace {
  vec<F, N> k1{};
  vec<F, N> k2{};
  vec<F, N> k3{};
  vec<F, N> k4{};
  vec<F, N> temporary{};
  vec<F, N> next{};
  dense_segment<F, N> last{};
};

template<ieee754_floating F, usize N> struct rk23_workspace {
  vec<F, N> stages[4]{};
  vec<F, N> temporary{};
  vec<F, N> next{};
  vec<F, N> error{};
  dense_segment<F, N> last{};
  bool has_fsal{ false };
};

template<ieee754_floating F, usize N> struct rk45_workspace {
  vec<F, N> stages[7]{};
  vec<F, N> temporary{};
  vec<F, N> next{};
  vec<F, N> error{};
  dense_segment<F, N> last{};
  bool has_fsal{ false };
};

template<ieee754_floating F, usize N> struct dop853_workspace {
  vec<F, N> stages[13]{};
  vec<F, N> temporary{};
  vec<F, N> next{};
  vec<F, N> error{};
  dense_segment<F, N> last{};
  bool has_fsal{ false };
};

struct null_observer {
  template<typename... Args>
  constexpr void
  operator()(Args &&...) const noexcept
  {
  }
};

struct no_event {
};

template<typename Fn> struct event_spec {
  Fn function;
  event_direction direction{ event_direction::any };
  bool terminal{ true };
  usize index{ 0 };
};

template<typename Fn>
[[nodiscard]] inline event_spec<Fn>
make_event(Fn function, event_direction direction = event_direction::any, bool terminal = true, usize index = 0) noexcept
{
  return event_spec<Fn>{ function, direction, terminal, index };
}

namespace __impl
{

template<ieee754_floating F>
[[nodiscard]] inline bool
valid_options(const options<F> &settings) noexcept
{
  return settings.abs_tol > F(0) && settings.rel_tol >= F(0) && settings.max_steps != 0 && settings.rejection_limit != 0
         && ieee::is_finite<F>(settings.abs_tol) && ieee::is_finite<F>(settings.rel_tol) && ieee::is_finite<F>(settings.initial_step)
         && ieee::is_finite<F>(settings.min_step) && ieee::is_finite<F>(settings.max_step) && ieee::is_finite<F>(settings.safety)
         && ieee::is_finite<F>(settings.min_factor) && ieee::is_finite<F>(settings.max_factor) && ieee::is_finite<F>(settings.pi_beta)
         && settings.min_step >= F(0) && settings.max_step >= F(0) && (settings.max_step == F(0) || settings.min_step <= settings.max_step)
         && settings.safety > F(0) && settings.min_factor > F(0) && settings.min_factor <= F(1) && settings.max_factor >= F(1)
         && settings.pi_beta >= F(0);
}

template<usize N, ieee754_floating F>
[[nodiscard]] inline bool
valid_output(const output_buffer<F, N> *output, F t0, F t_bound) noexcept
{
  if ( output == nullptr ) return true;
  if ( output->written > output->count ) return false;
  if ( output->count == 0 ) return true;
  if ( output->times == nullptr || output->states == nullptr ) return false;
  const F direction = t_bound >= t0 ? F(1) : F(-1);
  for ( usize i = output->written; i < output->count; ++i ) {
    if ( !ieee::is_finite<F>(output->times[i]) || direction * (output->times[i] - t0) < F(0)
         || direction * (output->times[i] - t_bound) > F(0) )
      return false;
    if ( i != output->written && direction * (output->times[i] - output->times[i - 1]) < F(0) ) return false;
  }
  return true;
}

template<usize N, ieee754_floating F>
[[nodiscard]] inline bool
valid_dense(const dense_buffer<F, N> *storage) noexcept
{
  if ( storage == nullptr ) return true;
  return storage->written <= storage->capacity && (storage->capacity == 0 || storage->segments != nullptr);
}

template<usize N, ieee754_floating F, typename Rhs>
[[gnu::always_inline]] inline void
rhs(Rhs &function, F t, const vec<F, N> &y, vec<F, N> &out) noexcept
{
  if constexpr ( requires { function(t, y, out); } ) {
    function(t, y, out);
  } else if constexpr ( requires { function(t, y.data, out.data, N); } ) {
    function(t, y.data, out.data, N);
  } else if constexpr ( requires { function(t, y.data, out.data); } ) {
    function(t, y.data, out.data);
  } else {
    out = function(t, y);
  }
}

template<usize N, ieee754_floating F>
[[nodiscard]] inline bool
finite(const vec<F, N> &value) noexcept
{
  for ( usize i = 0; i < N; ++i )
    if ( !ieee::is_finite<F>(value.data[i]) ) return false;
  return true;
}

template<usize N, ieee754_floating F, typename Observer>
inline void
observe(Observer &observer, F t, const vec<F, N> &y) noexcept
{
  if constexpr ( requires { observer(t, y); } )
    observer(t, y);
  else if constexpr ( requires { observer(t, y.data, N); } )
    observer(t, y.data, N);
}

template<usize N, ieee754_floating F, typename Event>
[[nodiscard]] inline F
event_value(Event &event, F t, const vec<F, N> &y) noexcept
{
  if constexpr ( requires { event.function(t, y); } )
    return F(event.function(t, y));
  else
    return F(event.function(t, y.data, N));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline bool
crossed(F before, F after, event_direction direction) noexcept
{
  if ( direction == event_direction::rising ) return before < F(0) && after >= F(0);
  if ( direction == event_direction::falling ) return before > F(0) && after <= F(0);
  return (before < F(0) && after >= F(0)) || (before > F(0) && after <= F(0));
}

template<ieee754_floating F, typename Evaluate>
[[nodiscard]] inline F
locate_bracket(F start, F end, F before, F after, Evaluate evaluate) noexcept
{
  F a = start;
  F b = end;
  F fa = before;
  F fb = after;
  if ( fb == F(0) ) return b;
  if ( fa == F(0) ) return a;
  if ( b < a ) {
    const F time = a;
    a = b;
    b = time;
    const F value = fa;
    fa = fb;
    fb = value;
  }
  if ( mk::manip::fabs<F>(fa) < mk::manip::fabs<F>(fb) ) {
    const F time = a;
    a = b;
    b = time;
    const F value = fa;
    fa = fb;
    fb = value;
  }
  F c = a;
  F fc = fa;
  F d = c;
  bool bisected = true;
  for ( usize iteration = 0; iteration < 64; ++iteration ) {
    F trial{};
    if ( fa != fc && fb != fc ) {
      trial = a * fb * fc / ((fa - fb) * (fa - fc)) + b * fa * fc / ((fb - fa) * (fb - fc)) + c * fa * fb / ((fc - fa) * (fc - fb));
    } else {
      trial = b - fb * (b - a) / (fb - fa);
    }
    const F guarded = (F(3) * a + b) * F(0.25);
    const F guard_lo = guarded < b ? guarded : b;
    const F guard_hi = guarded < b ? b : guarded;
    const F tolerance = F(16) * machine_epsilon<F>() * (mk::manip::fabs<F>(a) + mk::manip::fabs<F>(b) + F(1));
    const bool outside = trial <= guard_lo || trial >= guard_hi;
    const bool slow_bisection = bisected && mk::manip::fabs<F>(trial - b) >= F(0.5) * mk::manip::fabs<F>(b - c);
    const bool slow_interpolation = !bisected && mk::manip::fabs<F>(trial - b) >= F(0.5) * mk::manip::fabs<F>(c - d);
    const bool stale_bisection = bisected && mk::manip::fabs<F>(b - c) <= tolerance;
    const bool stale_interpolation = !bisected && mk::manip::fabs<F>(c - d) <= tolerance;
    if ( outside || slow_bisection || slow_interpolation || stale_bisection || stale_interpolation || !ieee::is_finite<F>(trial) ) {
      trial = F(0.5) * (a + b);
      bisected = true;
    } else {
      bisected = false;
    }

    const F value = evaluate(trial);
    if ( value == F(0) ) return trial;
    if ( !ieee::is_finite<F>(value) ) {
      trial = F(0.5) * (a + b);
      const F midpoint_value = evaluate(trial);
      if ( !ieee::is_finite<F>(midpoint_value) ) return b;
      d = c;
      c = b;
      fc = fb;
      if ( (fa < F(0) && midpoint_value > F(0)) || (fa > F(0) && midpoint_value < F(0)) ) {
        b = trial;
        fb = midpoint_value;
      } else {
        a = trial;
        fa = midpoint_value;
      }
    } else {
      d = c;
      c = b;
      fc = fb;
      if ( (fa < F(0) && value > F(0)) || (fa > F(0) && value < F(0)) ) {
        b = trial;
        fb = value;
      } else {
        a = trial;
        fa = value;
      }
    }
    if ( mk::manip::fabs<F>(fa) < mk::manip::fabs<F>(fb) ) {
      const F time = a;
      a = b;
      b = time;
      const F value = fa;
      fa = fb;
      fb = value;
    }
    if ( mk::manip::fabs<F>(b - a) <= tolerance ) break;
  }
  return b;
}

template<usize N, ieee754_floating F, typename Event>
[[nodiscard]] inline F
locate_event(Event &event, const dense_segment<F, N> &dense, F before, F after) noexcept
{
  auto evaluate = [&](F time) noexcept {
    const vec<F, N> state = dense.evaluate(time);
    return event_value<N, F>(event, time, state);
  };
  return locate_bracket<F>(dense.t0, dense.t1, before, after, evaluate);
}

template<usize N, ieee754_floating F>
inline void
write_outputs(const dense_segment<F, N> &dense, F direction, output_buffer<F, N> *output) noexcept
{
  if ( output == nullptr || output->times == nullptr || output->states == nullptr ) return;
  while ( output->written < output->count ) {
    const F time = output->times[output->written];
    if ( direction * (time - dense.t0) < F(0) ) {
      ++output->written;
      continue;
    }
    if ( direction * (time - dense.t1) > F(0) ) break;
    output->states[output->written] = dense.evaluate(time);
    ++output->written;
  }
}

template<usize N, ieee754_floating F>
[[nodiscard]] inline bool
store_dense(const dense_segment<F, N> &dense, dense_buffer<F, N> *storage) noexcept
{
  if ( storage == nullptr || storage->segments == nullptr ) return true;
  if ( storage->written >= storage->capacity ) return false;
  storage->segments[storage->written++] = dense;
  return true;
}

template<usize N, ieee754_floating F>
[[nodiscard]] inline F
error_norm(const vec<F, N> &error, const vec<F, N> &before, const vec<F, N> &after, const options<F> &settings) noexcept
{
  F sum = F(0);
  for ( usize i = 0; i < N; ++i ) {
    const F magnitude = mk::manip::fabs<F>(before.data[i]) > mk::manip::fabs<F>(after.data[i]) ? mk::manip::fabs<F>(before.data[i])
                                                                                               : mk::manip::fabs<F>(after.data[i]);
    const F scale = settings.abs_tol + settings.rel_tol * magnitude;
    const F ratio = error.data[i] / scale;
    sum += ratio * ratio;
  }
  return mk::pow_ns::sqrt<F>(sum / F(N));
}

template<ieee754_floating F>
[[nodiscard]] inline F
clamp(F value, F lo, F hi) noexcept
{
  if ( value < lo ) return lo;
  if ( value > hi ) return hi;
  return value;
}

template<method Method> inline constexpr usize estimator_order = Method == method::rk23 ? 2 : (Method == method::rk45 ? 4 : 7);

template<method Method, ieee754_floating F>
[[nodiscard]] inline F
step_factor(F error, F previous_error, const options<F> &settings, bool rejected) noexcept
{
  if ( error == F(0) ) return settings.max_factor;
  const F exponent = -F(1) / F(estimator_order<Method> + 1);
  F factor = settings.safety * mk::pow_ns::pow<F>(error, exponent);
  if ( previous_error > F(0) ) factor *= mk::pow_ns::pow<F>(previous_error, settings.pi_beta);
  factor = clamp(factor, settings.min_factor, settings.max_factor);
  if ( rejected && factor > F(1) ) factor = F(1);
  return factor;
}

template<ieee754_floating F> struct dop853_coefficients {
  static constexpr F c[12] = {
    F(0),
    F(0.0526001519587677318785587544488L),
    F(0.0789002279381515978178381316732L),
    F(0.118350341907227396726757197510L),
    F(0.281649658092772603273242802490L),
    F(1.0L / 3.0L),
    F(0.25L),
    F(4.0L / 13.0L),
    F(0.651282051282051282051282051282L),
    F(0.6L),
    F(6.0L / 7.0L),
    F(1),
  };
  static constexpr F a[12][12] = {
    {},
    { F(5.26001519587677318785587544488e-2L) },
    { F(1.97250569845378994544595329183e-2L), F(5.91751709536136983633785987549e-2L) },
    { F(2.95875854768068491816892993775e-2L), F(0), F(8.87627564304205475450678981324e-2L) },
    { F(2.41365134159266685502369798665e-1L), F(0), F(-8.84549479328286085344864962717e-1L), F(9.24834003261792003115737966543e-1L) },
    { F(3.7037037037037037037037037037e-2L), F(0), F(0), F(1.70828608729473871279604482173e-1L), F(1.25467687566822425016691814123e-1L) },
    { F(3.7109375e-2L), F(0), F(0), F(1.70252211019544039314978060272e-1L), F(6.02165389804559606850219397283e-2L), F(-1.7578125e-2L) },
    { F(3.70920001185047927108779319836e-2L), F(0), F(0), F(1.70383925712239993810214054705e-1L), F(1.07262030446373284651809199168e-1L),
      F(-1.53194377486244017527936158236e-2L), F(8.27378916381402288758473766002e-3L) },
    { F(6.24110958716075717114429577812e-1L), F(0), F(0), F(-3.36089262944694129406857109825L), F(-8.68219346841726006818189891453e-1L),
      F(2.75920996994467083049415600797e1L), F(2.01540675504778934086186788979e1L), F(-4.34898841810699588477366255144e1L) },
    { F(4.77662536438264365890433908527e-1L), F(0), F(0), F(-2.48811461997166764192642586468L), F(-5.90290826836842996371446475743e-1L),
      F(2.12300514481811942347288949897e1L), F(1.52792336328824235832596922938e1L), F(-3.32882109689848629194453265587e1L),
      F(-2.03312017085086261358222928593e-2L) },
    { F(-9.3714243008598732571704021658e-1L), F(0), F(0), F(5.18637242884406370830023853209L), F(1.09143734899672957818500254654L),
      F(-8.14978701074692612513997267357L), F(-1.85200656599969598641566180701e1L), F(2.27394870993505042818970056734e1L),
      F(2.49360555267965238987089396762L), F(-3.0467644718982195003823669022L) },
    { F(2.27331014751653820792359768449L), F(0), F(0), F(-1.05344954667372501984066689879e1L), F(-2.00087205822486249909675718444L),
      F(-1.79589318631187989172765950534e1L), F(2.79488845294199600508499808837e1L), F(-2.85899827713502369474065508674L),
      F(-8.87285693353062954433549289258L), F(1.23605671757943030647266201528e1L), F(6.43392746015763530355970484046e-1L) },
  };
  static constexpr F b[12] = {
    F(5.42937341165687622380535766363e-2L),
    F(0),
    F(0),
    F(0),
    F(0),
    F(4.45031289275240888144113950566L),
    F(1.89151789931450038304281599044L),
    F(-5.8012039600105847814672114227L),
    F(3.1116436695781989440891606237e-1L),
    F(-1.52160949662516078556178806805e-1L),
    F(2.01365400804030348374776537501e-1L),
    F(4.47106157277725905176885569043e-2L),
  };
  static constexpr F e3[13] = {
    b[0] - F(0.244094488188976377952755905512L),
    b[1],
    b[2],
    b[3],
    b[4],
    b[5],
    b[6],
    b[7],
    b[8] - F(0.733846688281611857341361741547L),
    b[9],
    b[10],
    b[11] - F(0.0220588235294117647058823529412L),
    F(0),
  };
  static constexpr F e5[13] = {
    F(0.01312004499419488073250102996L),
    F(0),
    F(0),
    F(0),
    F(0),
    F(-1.225156446376204440720569753L),
    F(-0.4957589496572501915214079952L),
    F(1.664377182454986536961530415L),
    F(-0.3503288487499736816886487290L),
    F(0.3341791187130174790297318841L),
    F(0.0819232064851157269448072613L),
    F(-0.02235530786388629525884427845L),
    F(0),
  };
};

template<usize N, ieee754_floating F, typename Rhs>
[[nodiscard]] inline bool
rk23_step(Rhs &function, F t, const vec<F, N> &y, F h, rk23_workspace<F, N> &workspace, usize &evaluations) noexcept
{
  if ( !workspace.has_fsal ) {
    rhs<N, F>(function, t, y, workspace.stages[0]);
    ++evaluations;
  }
  for ( usize i = 0; i < N; ++i ) workspace.temporary.data[i] = y.data[i] + h * F(0.5) * workspace.stages[0].data[i];
  rhs<N, F>(function, t + h * F(0.5), workspace.temporary, workspace.stages[1]);
  for ( usize i = 0; i < N; ++i ) workspace.temporary.data[i] = y.data[i] + h * F(0.75) * workspace.stages[1].data[i];
  rhs<N, F>(function, t + h * F(0.75), workspace.temporary, workspace.stages[2]);
  for ( usize i = 0; i < N; ++i )
    workspace.next.data[i] = y.data[i]
                             + h
                                   * (F(2.0L / 9.0L) * workspace.stages[0].data[i] + F(1.0L / 3.0L) * workspace.stages[1].data[i]
                                      + F(4.0L / 9.0L) * workspace.stages[2].data[i]);
  rhs<N, F>(function, t + h, workspace.next, workspace.stages[3]);
  evaluations += 3;
  for ( usize i = 0; i < N; ++i )
    workspace.error.data[i] = h
                              * (F(-5.0L / 72.0L) * workspace.stages[0].data[i] + F(1.0L / 12.0L) * workspace.stages[1].data[i]
                                 + F(1.0L / 9.0L) * workspace.stages[2].data[i] - F(1.0L / 8.0L) * workspace.stages[3].data[i]);
  return finite(workspace.next) && finite(workspace.error) && finite(workspace.stages[3]);
}

template<usize N, ieee754_floating F, typename Rhs>
[[nodiscard]] inline bool
rk45_step(Rhs &function, F t, const vec<F, N> &y, F h, rk45_workspace<F, N> &workspace, usize &evaluations) noexcept
{
  if ( !workspace.has_fsal ) {
    rhs<N, F>(function, t, y, workspace.stages[0]);
    ++evaluations;
  }
  constexpr F c[6] = { F(1.0L / 5.0L), F(3.0L / 10.0L), F(4.0L / 5.0L), F(8.0L / 9.0L), F(1), F(1) };
  constexpr F a[5][5] = {
    { F(1.0L / 5.0L) },
    { F(3.0L / 40.0L), F(9.0L / 40.0L) },
    { F(44.0L / 45.0L), F(-56.0L / 15.0L), F(32.0L / 9.0L) },
    { F(19372.0L / 6561.0L), F(-25360.0L / 2187.0L), F(64448.0L / 6561.0L), F(-212.0L / 729.0L) },
    { F(9017.0L / 3168.0L), F(-355.0L / 33.0L), F(46732.0L / 5247.0L), F(49.0L / 176.0L), F(-5103.0L / 18656.0L) },
  };
  for ( usize stage = 1; stage <= 5; ++stage ) {
    for ( usize i = 0; i < N; ++i ) {
      F combination = F(0);
      for ( usize j = 0; j < stage; ++j ) combination += a[stage - 1][j] * workspace.stages[j].data[i];
      workspace.temporary.data[i] = y.data[i] + h * combination;
    }
    rhs<N, F>(function, t + c[stage - 1] * h, workspace.temporary, workspace.stages[stage]);
  }
  constexpr F b[6] = { F(35.0L / 384.0L), F(0), F(500.0L / 1113.0L), F(125.0L / 192.0L), F(-2187.0L / 6784.0L), F(11.0L / 84.0L) };
  for ( usize i = 0; i < N; ++i ) {
    F combination = F(0);
    for ( usize stage = 0; stage < 6; ++stage ) combination += b[stage] * workspace.stages[stage].data[i];
    workspace.next.data[i] = y.data[i] + h * combination;
  }
  rhs<N, F>(function, t + h, workspace.next, workspace.stages[6]);
  evaluations += 6;
  constexpr F e[7] = { F(71.0L / 57600.0L), F(0), F(-71.0L / 16695.0L), F(71.0L / 1920.0L), F(-17253.0L / 339200.0L), F(22.0L / 525.0L),
                       F(-1.0L / 40.0L) };
  for ( usize i = 0; i < N; ++i ) {
    F combination = F(0);
    for ( usize stage = 0; stage < 7; ++stage ) combination += e[stage] * workspace.stages[stage].data[i];
    workspace.error.data[i] = h * combination;
  }
  return finite(workspace.next) && finite(workspace.error) && finite(workspace.stages[6]);
}

template<usize N, ieee754_floating F, typename Rhs>
[[nodiscard]] inline bool
dop853_step(Rhs &function, F t, const vec<F, N> &y, F h, dop853_workspace<F, N> &workspace, usize &evaluations) noexcept
{
  using table = dop853_coefficients<F>;
  if ( !workspace.has_fsal ) {
    rhs<N, F>(function, t, y, workspace.stages[0]);
    ++evaluations;
  }
  for ( usize stage = 1; stage < 12; ++stage ) {
    for ( usize i = 0; i < N; ++i ) {
      F combination = F(0);
      for ( usize j = 0; j < stage; ++j ) combination += table::a[stage][j] * workspace.stages[j].data[i];
      workspace.temporary.data[i] = y.data[i] + h * combination;
    }
    rhs<N, F>(function, t + table::c[stage] * h, workspace.temporary, workspace.stages[stage]);
  }
  for ( usize i = 0; i < N; ++i ) {
    F combination = F(0);
    for ( usize stage = 0; stage < 12; ++stage ) combination += table::b[stage] * workspace.stages[stage].data[i];
    workspace.next.data[i] = y.data[i] + h * combination;
  }
  rhs<N, F>(function, t + h, workspace.next, workspace.stages[12]);
  evaluations += 12;
  for ( usize i = 0; i < N; ++i ) {
    F err5 = F(0), err3 = F(0);
    for ( usize stage = 0; stage < 13; ++stage ) {
      err5 += table::e5[stage] * workspace.stages[stage].data[i];
      err3 += table::e3[stage] * workspace.stages[stage].data[i];
    }
    const F denominator = mk::pow_ns::sqrt<F>(err5 * err5 + F(0.01) * err3 * err3);
    workspace.error.data[i] = denominator == F(0) ? F(0) : h * err5 * mk::manip::fabs<F>(err5) / denominator;
  }
  return finite(workspace.next) && finite(workspace.error) && finite(workspace.stages[12]);
}

template<method Method, usize N, ieee754_floating F> struct workspace_for;

template<usize N, ieee754_floating F> struct workspace_for<method::rk23, N, F> {
  using type = rk23_workspace<F, N>;
};

template<usize N, ieee754_floating F> struct workspace_for<method::rk45, N, F> {
  using type = rk45_workspace<F, N>;
};

template<usize N, ieee754_floating F> struct workspace_for<method::dop853, N, F> {
  using type = dop853_workspace<F, N>;
};

template<method Method, usize N, ieee754_floating F, typename Rhs, typename Workspace>
[[nodiscard]] inline bool
adaptive_step(Rhs &function, F t, const vec<F, N> &y, F h, Workspace &workspace, usize &evaluations) noexcept
{
  if constexpr ( Method == method::rk23 )
    return rk23_step<N, F>(function, t, y, h, workspace, evaluations);
  else if constexpr ( Method == method::rk45 )
    return rk45_step<N, F>(function, t, y, h, workspace, evaluations);
  else
    return dop853_step<N, F>(function, t, y, h, workspace, evaluations);
}

template<method Method, typename Workspace>
inline void
accept_fsal(Workspace &workspace) noexcept
{
  constexpr usize endpoint = Method == method::rk23 ? 3 : (Method == method::rk45 ? 6 : 12);
  workspace.stages[0] = workspace.stages[endpoint];
  workspace.has_fsal = true;
}

};      // namespace __impl

template<method Method, usize N, ieee754_floating F, typename Rhs, typename Workspace, typename Observer = null_observer,
         typename Event = no_event>
  requires(Method == method::rk23 || Method == method::rk45 || Method == method::dop853)
[[nodiscard]] inline result<F, N>
solve_ivp(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
          const options<F> &settings, Workspace &workspace, Observer observer = {}, Event event = {}, output_buffer<F, N> *output = nullptr,
          dense_buffer<F, N> *dense_storage = nullptr) noexcept
{
  result<F, N> result_value{};
  result_value.t = t0;
  result_value.y = initial;
  workspace.has_fsal = false;
  if ( !__impl::valid_options(settings) || !__impl::finite(initial) || !ieee::is_finite<F>(t0) || !ieee::is_finite<F>(t_bound)
       || !__impl::valid_output(output, t0, t_bound) || !__impl::valid_dense(dense_storage) ) {
    result_value.termination = status::invalid_input;
    return result_value;
  }
  if ( t0 == t_bound ) {
    __impl::observe<N, F>(observer, t0, initial);
    if ( output != nullptr )
      while ( output->written < output->count && output->times[output->written] == t0 ) output->states[output->written++] = initial;
    result_value.t_eval_written = output == nullptr ? 0 : output->written;
    return result_value;
  }

  const F direction = t_bound > t0 ? F(1) : F(-1);
  const F span = mk::manip::fabs<F>(t_bound - t0);
  F step_abs = settings.initial_step == F(0) ? span / F(100) : mk::manip::fabs<F>(settings.initial_step);
  if ( step_abs == F(0) ) step_abs = span;
  const F max_step = settings.max_step > F(0) ? settings.max_step : span;
  if ( step_abs > max_step ) step_abs = max_step;
  F previous_error = F(1);
  usize consecutive_rejections = 0;
  bool rejected_before_accept = false;

  __impl::observe<N, F>(observer, t0, initial);
  if ( output != nullptr && output->times != nullptr && output->states != nullptr ) {
    while ( output->written < output->count && output->times[output->written] == t0 ) {
      output->states[output->written] = initial;
      ++output->written;
    }
  }

  F event_before = F(0);
  if constexpr ( !micron::is_same_v<Event, no_event> ) {
    event_before = __impl::event_value<N, F>(event, t0, initial);
    if ( !ieee::is_finite<F>(event_before) ) {
      result_value.termination = status::non_finite;
      return result_value;
    }
    if ( event_before == F(0) ) {
      result_value.event_index = event.index;
      result_value.event_time = t0;
      if ( event.terminal ) {
        result_value.termination = status::event;
        result_value.t_eval_written = output == nullptr ? 0 : output->written;
        return result_value;
      }
    }
  }

  while ( direction * (result_value.t - t_bound) < F(0) ) {
    if ( result_value.attempted_steps >= settings.max_steps ) {
      result_value.termination = status::max_steps;
      break;
    }
    F min_step = settings.min_step;
    if ( min_step <= F(0) ) min_step = F(16) * machine_epsilon<F>() * (mk::manip::fabs<F>(result_value.t) + F(1));
    if ( step_abs < min_step ) {
      result_value.termination = status::step_underflow;
      break;
    }
    F h = direction * step_abs;
    if ( direction * (result_value.t + h - t_bound) > F(0) ) h = t_bound - result_value.t;
    if ( result_value.t + h == result_value.t ) {
      result_value.termination = status::step_underflow;
      break;
    }

    ++result_value.attempted_steps;
    if ( !__impl::adaptive_step<Method, N, F>(function, result_value.t, result_value.y, h, workspace, result_value.n_evals) ) {
      result_value.termination = status::non_finite;
      break;
    }
    const F norm = __impl::error_norm<N, F>(workspace.error, result_value.y, workspace.next, settings);
    if ( !ieee::is_finite<F>(norm) ) {
      result_value.termination = status::non_finite;
      break;
    }
    if ( norm > F(1) ) {
      ++result_value.rejected_steps;
      ++consecutive_rejections;
      rejected_before_accept = true;
      if ( consecutive_rejections > settings.rejection_limit ) {
        result_value.termination = status::rejection_limit;
        break;
      }
      step_abs *= __impl::step_factor<Method>(norm, previous_error, settings, true);
      workspace.has_fsal = true;
      continue;
    }

    consecutive_rejections = 0;
    workspace.last.t0 = result_value.t;
    workspace.last.t1 = result_value.t + h;
    workspace.last.y0 = result_value.y;
    workspace.last.y1 = workspace.next;
    workspace.last.f0 = workspace.stages[0];
    constexpr usize endpoint = Method == method::rk23 ? 3 : (Method == method::rk45 ? 6 : 12);
    workspace.last.f1 = workspace.stages[endpoint];
    result_value.t += h;
    result_value.y = workspace.next;
    result_value.last_step = h;
    ++result_value.accepted_steps;
    __impl::accept_fsal<Method>(workspace);

    bool terminal_event = false;
    if constexpr ( !micron::is_same_v<Event, no_event> ) {
      const F event_after = __impl::event_value<N, F>(event, result_value.t, result_value.y);
      if ( !ieee::is_finite<F>(event_after) ) {
        result_value.termination = status::non_finite;
        break;
      }
      if ( __impl::crossed<F>(event_before, event_after, event.direction) ) {
        const F root = __impl::locate_event<N, F>(event, workspace.last, event_before, event_after);
        result_value.event_index = event.index;
        result_value.event_time = root;
        if ( event.terminal ) {
          const F event_step = root - workspace.last.t0;
          result_value.t = root;
          result_value.y = workspace.last.evaluate(root);
          workspace.last.t1 = root;
          workspace.last.y1 = result_value.y;
          __impl::rhs<N, F>(function, root, result_value.y, workspace.last.f1);
          ++result_value.n_evals;
          result_value.last_step = event_step;
          result_value.termination = status::event;
          terminal_event = true;
        }
      }
      event_before = event_after;
    }

    __impl::write_outputs<N, F>(workspace.last, direction, output);
    if ( !__impl::store_dense<N, F>(workspace.last, dense_storage) ) {
      result_value.termination = status::storage_full;
      break;
    }
    if ( terminal_event ) {
      __impl::observe<N, F>(observer, result_value.t, result_value.y);
      break;
    }

    __impl::observe<N, F>(observer, result_value.t, result_value.y);
    F factor = __impl::step_factor<Method>(norm, previous_error, settings, rejected_before_accept);
    rejected_before_accept = false;
    previous_error = norm > F(1e-4L) ? norm : F(1e-4L);
    step_abs *= factor;
    if ( step_abs > max_step ) step_abs = max_step;
  }

  result_value.t_eval_written = output == nullptr ? 0 : output->written;
  result_value.dense_segments_written = dense_storage == nullptr ? 0 : dense_storage->written;
  return result_value;
}

template<method Method, usize N, ieee754_floating F, typename Rhs, typename Workspace, typename Observer = null_observer,
         typename Event = no_event>
  requires(Method == method::rk23 || Method == method::rk45 || Method == method::dop853)
[[nodiscard]] inline result<F, N>
solve(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
      const options<F> &settings, Workspace &workspace, Observer observer = {}, Event event = {}) noexcept
{
  return solve_ivp<Method, N, F>(function, t0, initial, t_bound, settings, workspace, observer, event);
}

namespace __impl
{

template<method Method, usize N, ieee754_floating F, typename Rhs>
inline void
fixed_step(Rhs &function, F t, const vec<F, N> &y, F h, fixed_workspace<F, N> &workspace, vec<F, N> &next, usize &evaluations) noexcept
{
  rhs<N, F>(function, t, y, workspace.k1);
  if constexpr ( Method == method::euler ) {
    for ( usize i = 0; i < N; ++i ) next.data[i] = y.data[i] + h * workspace.k1.data[i];
    rhs<N, F>(function, t + h, next, workspace.k2);
    evaluations += 2;
  } else if constexpr ( Method == method::midpoint ) {
    for ( usize i = 0; i < N; ++i ) workspace.temporary.data[i] = y.data[i] + F(0.5) * h * workspace.k1.data[i];
    rhs<N, F>(function, t + F(0.5) * h, workspace.temporary, workspace.k2);
    for ( usize i = 0; i < N; ++i ) next.data[i] = y.data[i] + h * workspace.k2.data[i];
    rhs<N, F>(function, t + h, next, workspace.k3);
    evaluations += 3;
  } else {
    for ( usize i = 0; i < N; ++i ) workspace.temporary.data[i] = y.data[i] + F(0.5) * h * workspace.k1.data[i];
    rhs<N, F>(function, t + F(0.5) * h, workspace.temporary, workspace.k2);
    for ( usize i = 0; i < N; ++i ) workspace.temporary.data[i] = y.data[i] + F(0.5) * h * workspace.k2.data[i];
    rhs<N, F>(function, t + F(0.5) * h, workspace.temporary, workspace.k3);
    for ( usize i = 0; i < N; ++i ) workspace.temporary.data[i] = y.data[i] + h * workspace.k3.data[i];
    rhs<N, F>(function, t + h, workspace.temporary, workspace.k4);
    for ( usize i = 0; i < N; ++i )
      next.data[i] = y.data[i]
                     + h * (workspace.k1.data[i] + F(2) * workspace.k2.data[i] + F(2) * workspace.k3.data[i] + workspace.k4.data[i]) / F(6);
    rhs<N, F>(function, t + h, next, workspace.temporary);
    evaluations += 5;
  }
}

};      // namespace __impl

template<method Method, usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer, typename Event = no_event>
  requires(Method == method::euler || Method == method::midpoint || Method == method::rk4)
[[nodiscard]] inline result<F, N>
integrate_fixed(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
                micron::__type_identity_t<F> step, fixed_workspace<F, N> &workspace, Observer observer = {}, Event event = {},
                output_buffer<F, N> *output = nullptr, dense_buffer<F, N> *dense_storage = nullptr, usize max_steps = 100000) noexcept
{
  result<F, N> result_value{};
  result_value.t = t0;
  result_value.y = initial;
  if ( step == F(0) || max_steps == 0 || !__impl::finite(initial) || !ieee::is_finite<F>(step) || !ieee::is_finite<F>(t0)
       || !ieee::is_finite<F>(t_bound) || !__impl::valid_output(output, t0, t_bound) || !__impl::valid_dense(dense_storage) ) {
    result_value.termination = status::invalid_input;
    return result_value;
  }
  if ( t0 == t_bound ) {
    __impl::observe<N, F>(observer, t0, initial);
    if ( output != nullptr && output->written < output->count && output->times[output->written] == t0 ) {
      output->states[output->written++] = initial;
      result_value.t_eval_written = output->written;
    }
    return result_value;
  }
  const F direction = t_bound > t0 ? F(1) : F(-1);
  const F step_abs = mk::manip::fabs<F>(step);
  __impl::observe<N, F>(observer, t0, initial);
  if ( output != nullptr ) {
    while ( output->written < output->count && output->times[output->written] == t0 ) output->states[output->written++] = initial;
  }
  F event_before = F(0);
  if constexpr ( !micron::is_same_v<Event, no_event> ) {
    event_before = __impl::event_value<N, F>(event, t0, initial);
    if ( !ieee::is_finite<F>(event_before) ) {
      result_value.termination = status::non_finite;
      return result_value;
    }
    if ( event_before == F(0) ) {
      result_value.event_index = event.index;
      result_value.event_time = t0;
      if ( event.terminal ) {
        result_value.termination = status::event;
        result_value.t_eval_written = output == nullptr ? 0 : output->written;
        return result_value;
      }
    }
  }
  while ( direction * (result_value.t - t_bound) < F(0) ) {
    if ( result_value.attempted_steps >= max_steps ) {
      result_value.termination = status::max_steps;
      break;
    }
    F h = direction * step_abs;
    if ( direction * (result_value.t + h - t_bound) > F(0) ) h = t_bound - result_value.t;
    vec<F, N> &next = workspace.next;
    __impl::fixed_step<Method, N, F>(function, result_value.t, result_value.y, h, workspace, next, result_value.n_evals);
    ++result_value.attempted_steps;
    if ( !__impl::finite(next) ) {
      result_value.termination = status::non_finite;
      break;
    }
    workspace.last.t0 = result_value.t;
    workspace.last.t1 = result_value.t + h;
    workspace.last.y0 = result_value.y;
    workspace.last.y1 = next;
    workspace.last.f0 = workspace.k1;
    if constexpr ( Method == method::euler )
      workspace.last.f1 = workspace.k2;
    else if constexpr ( Method == method::midpoint )
      workspace.last.f1 = workspace.k3;
    else
      workspace.last.f1 = workspace.temporary;
    result_value.t += h;
    result_value.y = next;
    result_value.last_step = h;
    ++result_value.accepted_steps;
    bool terminal_event = false;
    if constexpr ( !micron::is_same_v<Event, no_event> ) {
      const F event_after = __impl::event_value<N, F>(event, result_value.t, result_value.y);
      if ( !ieee::is_finite<F>(event_after) ) {
        result_value.termination = status::non_finite;
        break;
      }
      if ( __impl::crossed<F>(event_before, event_after, event.direction) ) {
        const F root = __impl::locate_event<N, F>(event, workspace.last, event_before, event_after);
        result_value.event_index = event.index;
        result_value.event_time = root;
        if ( event.terminal ) {
          const F event_step = root - workspace.last.t0;
          result_value.t = root;
          result_value.y = workspace.last.evaluate(root);
          workspace.last.t1 = root;
          workspace.last.y1 = result_value.y;
          __impl::rhs<N, F>(function, root, result_value.y, workspace.last.f1);
          ++result_value.n_evals;
          result_value.last_step = event_step;
          result_value.termination = status::event;
          terminal_event = true;
        }
      }
      event_before = event_after;
    }
    __impl::write_outputs(workspace.last, direction, output);
    if ( !__impl::store_dense(workspace.last, dense_storage) ) {
      result_value.termination = status::storage_full;
      break;
    }
    if ( terminal_event ) {
      __impl::observe<N, F>(observer, result_value.t, result_value.y);
      break;
    }
    __impl::observe<N, F>(observer, result_value.t, result_value.y);
  }
  result_value.t_eval_written = output == nullptr ? 0 : output->written;
  result_value.dense_segments_written = dense_storage == nullptr ? 0 : dense_storage->written;
  return result_value;
}

template<usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer>
[[nodiscard]] inline result<F, N>
euler(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
      micron::__type_identity_t<F> step, fixed_workspace<F, N> &workspace, Observer observer = {}) noexcept
{
  return integrate_fixed<method::euler>(function, t0, initial, t_bound, step, workspace, observer);
}

template<usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer>
[[nodiscard]] inline result<F, N>
midpoint(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
         micron::__type_identity_t<F> step, fixed_workspace<F, N> &workspace, Observer observer = {}) noexcept
{
  return integrate_fixed<method::midpoint>(function, t0, initial, t_bound, step, workspace, observer);
}

template<usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer>
[[nodiscard]] inline result<F, N>
rk4(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
    micron::__type_identity_t<F> step, fixed_workspace<F, N> &workspace, Observer observer = {}) noexcept
{
  return integrate_fixed<method::rk4>(function, t0, initial, t_bound, step, workspace, observer);
}

template<usize N, ieee754_floating F, typename Rhs>
[[nodiscard]] inline vec<F, N>
euler_step(Rhs function, micron::__type_identity_t<F> t, const vec<F, N> &state, micron::__type_identity_t<F> step,
           fixed_workspace<F, N> &workspace) noexcept
{
  usize evaluations = 0;
  __impl::fixed_step<method::euler>(function, t, state, step, workspace, workspace.next, evaluations);
  return workspace.next;
}

template<usize N, ieee754_floating F, typename Rhs>
[[nodiscard]] inline vec<F, N>
midpoint_step(Rhs function, micron::__type_identity_t<F> t, const vec<F, N> &state, micron::__type_identity_t<F> step,
              fixed_workspace<F, N> &workspace) noexcept
{
  usize evaluations = 0;
  __impl::fixed_step<method::midpoint>(function, t, state, step, workspace, workspace.next, evaluations);
  return workspace.next;
}

template<usize N, ieee754_floating F, typename Rhs>
[[nodiscard]] inline vec<F, N>
rk4_step(Rhs function, micron::__type_identity_t<F> t, const vec<F, N> &state, micron::__type_identity_t<F> step,
         fixed_workspace<F, N> &workspace) noexcept
{
  usize evaluations = 0;
  __impl::fixed_step<method::rk4>(function, t, state, step, workspace, workspace.next, evaluations);
  return workspace.next;
}

template<usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer, typename Event = no_event>
[[nodiscard]] inline result<F, N>
rk23(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
     const options<F> &settings, rk23_workspace<F, N> &workspace, Observer observer = {}, Event event = {}) noexcept
{
  return solve_ivp<method::rk23>(function, t0, initial, t_bound, settings, workspace, observer, event);
}

template<usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer, typename Event = no_event>
[[nodiscard]] inline result<F, N>
rk45(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
     const options<F> &settings, rk45_workspace<F, N> &workspace, Observer observer = {}, Event event = {}) noexcept
{
  return solve_ivp<method::rk45>(function, t0, initial, t_bound, settings, workspace, observer, event);
}

template<usize N, ieee754_floating F, typename Rhs, typename Observer = null_observer, typename Event = no_event>
[[nodiscard]] inline result<F, N>
dop853(Rhs function, micron::__type_identity_t<F> t0, const vec<F, N> &initial, micron::__type_identity_t<F> t_bound,
       const options<F> &settings, dop853_workspace<F, N> &workspace, Observer observer = {}, Event event = {}) noexcept
{
  return solve_ivp<method::dop853>(function, t0, initial, t_bound, settings, workspace, observer, event);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Runtime-length state surface. Every pointer names caller-owned storage.

template<ieee754_floating F> struct runtime_workspace {
  F *stages{ nullptr };                // stage_capacity * length
  F *temporary{ nullptr };             // length
  F *next{ nullptr };                  // length
  F *error{ nullptr };                 // length
  F *start{ nullptr };                 // length, retained for last dense step
  F *start_derivative{ nullptr };      // length
  F *end_derivative{ nullptr };        // length
  usize length{ 0 };
  usize stage_capacity{ 0 };
  bool has_fsal{ false };
  F last_t0{ 0 };
  F last_t1{ 0 };

  [[nodiscard]] inline bool
  valid(usize n, usize required_stages) const noexcept
  {
    return n != 0 && length >= n && stage_capacity >= required_stages && stages != nullptr && temporary != nullptr && next != nullptr
           && error != nullptr && start != nullptr && start_derivative != nullptr && end_derivative != nullptr;
  }

  inline void
  dense_evaluate(F t, F *out, usize n) const noexcept
  {
    const F h = last_t1 - last_t0;
    if ( h == F(0) ) {
      for ( usize i = 0; i < n; ++i ) out[i] = next[i];
      return;
    }
    F theta = (t - last_t0) / h;
    if ( theta < F(0) ) theta = F(0);
    if ( theta > F(1) ) theta = F(1);
    const F theta2 = theta * theta;
    const F theta3 = theta2 * theta;
    const F h00 = F(2) * theta3 - F(3) * theta2 + F(1);
    const F h10 = theta3 - F(2) * theta2 + theta;
    const F h01 = -F(2) * theta3 + F(3) * theta2;
    const F h11 = theta3 - theta2;
    for ( usize i = 0; i < n; ++i ) out[i] = h00 * start[i] + h * h10 * start_derivative[i] + h01 * next[i] + h * h11 * end_derivative[i];
  }
};

template<ieee754_floating F> struct runtime_output_buffer {
  const F *times{ nullptr };
  usize count{ 0 };
  F *states{ nullptr };
  usize stride{ 0 };
  usize written{ 0 };
};

template<ieee754_floating F> struct runtime_dense_buffer {
  F *times{ nullptr };             // interleaved t0, t1 pairs
  F *coefficients{ nullptr };      // y0, y1, f0, f1 blocks per segment
  usize capacity{ 0 };
  usize stride{ 0 };
  usize written{ 0 };

  [[nodiscard]] inline bool
  valid(usize n) const noexcept
  {
    return written <= capacity && (stride == 0 || stride >= n) && (capacity == 0 || (times != nullptr && coefficients != nullptr));
  }

  inline void
  evaluate(usize segment, F t, F *out, usize n) const noexcept
  {
    if ( out == nullptr || segment >= written ) return;
    const usize width = stride == 0 ? n : stride;
    const F t0 = times[2 * segment];
    const F t1 = times[2 * segment + 1];
    const F *base = coefficients + segment * 4 * width;
    const F *y0 = base;
    const F *y1 = base + width;
    const F *f0 = base + 2 * width;
    const F *f1 = base + 3 * width;
    const F h = t1 - t0;
    if ( h == F(0) ) {
      for ( usize i = 0; i < n; ++i ) out[i] = y1[i];
      return;
    }
    F theta = (t - t0) / h;
    if ( theta < F(0) ) theta = F(0);
    if ( theta > F(1) ) theta = F(1);
    const F theta2 = theta * theta;
    const F theta3 = theta2 * theta;
    const F h00 = F(2) * theta3 - F(3) * theta2 + F(1);
    const F h10 = theta3 - F(2) * theta2 + theta;
    const F h01 = -F(2) * theta3 + F(3) * theta2;
    const F h11 = theta3 - theta2;
    for ( usize i = 0; i < n; ++i ) out[i] = h00 * y0[i] + h * h10 * f0[i] + h01 * y1[i] + h * h11 * f1[i];
  }
};

template<ieee754_floating F> struct runtime_result {
  F t{ 0 };
  usize n_evals{ 0 };
  usize accepted_steps{ 0 };
  usize rejected_steps{ 0 };
  usize attempted_steps{ 0 };
  status termination{ status::success };
  usize event_index{ usize(-1) };
  F event_time{ 0 };
  usize t_eval_written{ 0 };
  usize dense_segments_written{ 0 };
  F last_step{ 0 };
};

namespace __impl
{

template<ieee754_floating F, typename Rhs>
inline void
runtime_rhs(Rhs &function, F t, const F *y, F *out, usize n) noexcept
{
  if constexpr ( requires { function(t, y, out, n); } )
    function(t, y, out, n);
  else
    function(t, y, out);
}

template<ieee754_floating F>
[[nodiscard]] inline bool
runtime_finite(const F *values, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !ieee::is_finite<F>(values[i]) ) return false;
  return true;
}

template<ieee754_floating F>
[[nodiscard]] inline F
runtime_error_norm(const F *error, const F *before, const F *after, usize n, const options<F> &settings) noexcept
{
  F sum = F(0);
  for ( usize i = 0; i < n; ++i ) {
    const F before_abs = mk::manip::fabs<F>(before[i]);
    const F after_abs = mk::manip::fabs<F>(after[i]);
    const F scale = settings.abs_tol + settings.rel_tol * (before_abs > after_abs ? before_abs : after_abs);
    const F ratio = error[i] / scale;
    sum += ratio * ratio;
  }
  return mk::pow_ns::sqrt<F>(sum / F(n));
}

template<method Method> inline constexpr usize runtime_stages = Method == method::rk23 ? 4 : (Method == method::rk45 ? 7 : 13);

template<method Method, ieee754_floating F, typename Rhs>
[[nodiscard]] inline bool
runtime_adaptive_step(Rhs &function, F t, const F *y, usize n, F h, runtime_workspace<F> &workspace, usize &evaluations) noexcept
{
  auto stage = [&](usize s) noexcept -> F * { return workspace.stages + s * n; };
  if ( !workspace.has_fsal ) {
    runtime_rhs(function, t, y, stage(0), n);
    ++evaluations;
  }
  if constexpr ( Method == method::rk23 ) {
    for ( usize i = 0; i < n; ++i ) workspace.temporary[i] = y[i] + h * F(0.5) * stage(0)[i];
    runtime_rhs(function, t + h * F(0.5), workspace.temporary, stage(1), n);
    for ( usize i = 0; i < n; ++i ) workspace.temporary[i] = y[i] + h * F(0.75) * stage(1)[i];
    runtime_rhs(function, t + h * F(0.75), workspace.temporary, stage(2), n);
    for ( usize i = 0; i < n; ++i )
      workspace.next[i] = y[i] + h * (F(2.0L / 9.0L) * stage(0)[i] + F(1.0L / 3.0L) * stage(1)[i] + F(4.0L / 9.0L) * stage(2)[i]);
    runtime_rhs(function, t + h, workspace.next, stage(3), n);
    evaluations += 3;
    for ( usize i = 0; i < n; ++i )
      workspace.error[i] = h
                           * (F(-5.0L / 72.0L) * stage(0)[i] + F(1.0L / 12.0L) * stage(1)[i] + F(1.0L / 9.0L) * stage(2)[i]
                              - F(1.0L / 8.0L) * stage(3)[i]);
  } else if constexpr ( Method == method::rk45 ) {
    constexpr F c[5] = { F(1.0L / 5.0L), F(3.0L / 10.0L), F(4.0L / 5.0L), F(8.0L / 9.0L), F(1) };
    constexpr F a[5][5] = {
      { F(1.0L / 5.0L) },
      { F(3.0L / 40.0L), F(9.0L / 40.0L) },
      { F(44.0L / 45.0L), F(-56.0L / 15.0L), F(32.0L / 9.0L) },
      { F(19372.0L / 6561.0L), F(-25360.0L / 2187.0L), F(64448.0L / 6561.0L), F(-212.0L / 729.0L) },
      { F(9017.0L / 3168.0L), F(-355.0L / 33.0L), F(46732.0L / 5247.0L), F(49.0L / 176.0L), F(-5103.0L / 18656.0L) },
    };
    for ( usize s = 1; s <= 5; ++s ) {
      for ( usize i = 0; i < n; ++i ) {
        F combination = F(0);
        for ( usize j = 0; j < s; ++j ) combination += a[s - 1][j] * stage(j)[i];
        workspace.temporary[i] = y[i] + h * combination;
      }
      runtime_rhs(function, t + c[s - 1] * h, workspace.temporary, stage(s), n);
    }
    constexpr F b[6] = { F(35.0L / 384.0L), F(0), F(500.0L / 1113.0L), F(125.0L / 192.0L), F(-2187.0L / 6784.0L), F(11.0L / 84.0L) };
    for ( usize i = 0; i < n; ++i ) {
      F combination = F(0);
      for ( usize s = 0; s < 6; ++s ) combination += b[s] * stage(s)[i];
      workspace.next[i] = y[i] + h * combination;
    }
    runtime_rhs(function, t + h, workspace.next, stage(6), n);
    evaluations += 6;
    constexpr F e[7] = { F(71.0L / 57600.0L), F(0), F(-71.0L / 16695.0L), F(71.0L / 1920.0L), F(-17253.0L / 339200.0L), F(22.0L / 525.0L),
                         F(-1.0L / 40.0L) };
    for ( usize i = 0; i < n; ++i ) {
      F combination = F(0);
      for ( usize s = 0; s < 7; ++s ) combination += e[s] * stage(s)[i];
      workspace.error[i] = h * combination;
    }
  } else {
    using table = dop853_coefficients<F>;
    for ( usize s = 1; s < 12; ++s ) {
      for ( usize i = 0; i < n; ++i ) {
        F combination = F(0);
        for ( usize j = 0; j < s; ++j ) combination += table::a[s][j] * stage(j)[i];
        workspace.temporary[i] = y[i] + h * combination;
      }
      runtime_rhs(function, t + table::c[s] * h, workspace.temporary, stage(s), n);
    }
    for ( usize i = 0; i < n; ++i ) {
      F combination = F(0);
      for ( usize s = 0; s < 12; ++s ) combination += table::b[s] * stage(s)[i];
      workspace.next[i] = y[i] + h * combination;
    }
    runtime_rhs(function, t + h, workspace.next, stage(12), n);
    evaluations += 12;
    for ( usize i = 0; i < n; ++i ) {
      F err5 = F(0), err3 = F(0);
      for ( usize s = 0; s < 13; ++s ) {
        err5 += table::e5[s] * stage(s)[i];
        err3 += table::e3[s] * stage(s)[i];
      }
      const F denominator = mk::pow_ns::sqrt<F>(err5 * err5 + F(0.01) * err3 * err3);
      workspace.error[i] = denominator == F(0) ? F(0) : h * err5 * mk::manip::fabs<F>(err5) / denominator;
    }
  }
  constexpr usize endpoint = Method == method::rk23 ? 3 : (Method == method::rk45 ? 6 : 12);
  return runtime_finite(workspace.next, n) && runtime_finite(workspace.error, n) && runtime_finite(stage(endpoint), n);
}

template<ieee754_floating F>
inline void
runtime_write_outputs(runtime_workspace<F> &workspace, F direction, usize n, runtime_output_buffer<F> *output) noexcept
{
  if ( output == nullptr || output->times == nullptr || output->states == nullptr ) return;
  const usize stride = output->stride == 0 ? n : output->stride;
  while ( output->written < output->count ) {
    const F time = output->times[output->written];
    if ( direction * (time - workspace.last_t0) < F(0) ) {
      ++output->written;
      continue;
    }
    if ( direction * (time - workspace.last_t1) > F(0) ) break;
    workspace.dense_evaluate(time, output->states + output->written * stride, n);
    ++output->written;
  }
}

template<ieee754_floating F, typename Event>
[[nodiscard]] inline F
runtime_event_value(Event &event, F t, const F *state, usize n) noexcept
{
  if constexpr ( requires { event.function(t, state, n); } )
    return F(event.function(t, state, n));
  else
    return F(event.function(t, state));
}

template<ieee754_floating F, typename Event>
[[nodiscard]] inline F
runtime_locate_event(Event &event, runtime_workspace<F> &workspace, usize n, F before, F after) noexcept
{
  auto evaluate = [&](F time) noexcept {
    workspace.dense_evaluate(time, workspace.temporary, n);
    return runtime_event_value<F>(event, time, workspace.temporary, n);
  };
  return locate_bracket<F>(workspace.last_t0, workspace.last_t1, before, after, evaluate);
}

template<ieee754_floating F>
[[nodiscard]] inline bool
runtime_store_dense(const runtime_workspace<F> &workspace, usize n, runtime_dense_buffer<F> *storage) noexcept
{
  if ( storage == nullptr ) return true;
  if ( storage->written >= storage->capacity ) return false;
  const usize segment = storage->written++;
  const usize width = storage->stride == 0 ? n : storage->stride;
  storage->times[2 * segment] = workspace.last_t0;
  storage->times[2 * segment + 1] = workspace.last_t1;
  F *base = storage->coefficients + segment * 4 * width;
  F *y0 = base;
  F *y1 = base + width;
  F *f0 = base + 2 * width;
  F *f1 = base + 3 * width;
  for ( usize i = 0; i < n; ++i ) {
    y0[i] = workspace.start[i];
    y1[i] = workspace.next[i];
    f0[i] = workspace.start_derivative[i];
    f1[i] = workspace.end_derivative[i];
  }
  return true;
}

template<ieee754_floating F>
[[nodiscard]] inline bool
runtime_valid_output(const runtime_output_buffer<F> *output, usize n, F t0, F t_bound) noexcept
{
  if ( output == nullptr ) return true;
  if ( output->written > output->count || (output->stride != 0 && output->stride < n) ) return false;
  if ( output->count == 0 ) return true;
  if ( output->times == nullptr || output->states == nullptr ) return false;
  const F direction = t_bound >= t0 ? F(1) : F(-1);
  for ( usize i = output->written; i < output->count; ++i ) {
    if ( !ieee::is_finite<F>(output->times[i]) || direction * (output->times[i] - t0) < F(0)
         || direction * (output->times[i] - t_bound) > F(0) )
      return false;
    if ( i != output->written && direction * (output->times[i] - output->times[i - 1]) < F(0) ) return false;
  }
  return true;
}

template<ieee754_floating F>
[[nodiscard]] inline bool
runtime_valid_dense(const runtime_dense_buffer<F> *storage, usize n) noexcept
{
  return storage == nullptr || storage->valid(n);
}

};      // namespace __impl

template<method Method, ieee754_floating F, typename Rhs, typename Observer, typename Event>
  requires(Method == method::rk23 || Method == method::rk45 || Method == method::dop853)
[[nodiscard]] inline runtime_result<F>
__solve_ivp_runtime(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
                    const options<F> &settings, runtime_workspace<F> &workspace, Observer observer, Event event,
                    runtime_output_buffer<F> *output, runtime_dense_buffer<F> *dense_storage) noexcept
{
  runtime_result<F> result_value{};
  result_value.t = t0;
  constexpr usize required_stages = __impl::runtime_stages<Method>;
  if ( state == nullptr || !workspace.valid(n, required_stages) || !__impl::valid_options(settings) || !__impl::runtime_finite(state, n)
       || !ieee::is_finite<F>(t0) || !ieee::is_finite<F>(t_bound) || !__impl::runtime_valid_output(output, n, t0, t_bound)
       || !__impl::runtime_valid_dense(dense_storage, n) ) {
    result_value.termination = status::invalid_input;
    return result_value;
  }
  workspace.has_fsal = false;
  const usize output_stride = output == nullptr || output->stride == 0 ? n : output->stride;
  if ( t0 == t_bound ) {
    if constexpr ( requires { observer(t0, state, n); } ) observer(t0, state, n);
    if ( output != nullptr ) {
      while ( output->written < output->count && output->times[output->written] == t0 ) {
        for ( usize i = 0; i < n; ++i ) output->states[output->written * output_stride + i] = state[i];
        ++output->written;
      }
      result_value.t_eval_written = output->written;
    }
    if constexpr ( !micron::is_same_v<Event, no_event> ) {
      const F initial_event = __impl::runtime_event_value<F>(event, t0, state, n);
      if ( !ieee::is_finite<F>(initial_event) )
        result_value.termination = status::non_finite;
      else if ( initial_event == F(0) ) {
        result_value.event_index = event.index;
        result_value.event_time = t0;
        if ( event.terminal ) result_value.termination = status::event;
      }
    }
    return result_value;
  }
  const F direction = t_bound > t0 ? F(1) : F(-1);
  const F span = mk::manip::fabs<F>(t_bound - t0);
  F step_abs = settings.initial_step == F(0) ? span / F(100) : mk::manip::fabs<F>(settings.initial_step);
  const F max_step = settings.max_step > F(0) ? settings.max_step : span;
  if ( step_abs > max_step ) step_abs = max_step;
  F previous_error = F(1);
  usize consecutive_rejections = 0;
  bool rejected_before_accept = false;
  if constexpr ( requires { observer(t0, state, n); } ) observer(t0, state, n);
  if ( output != nullptr ) {
    while ( output->written < output->count && output->times[output->written] == t0 ) {
      for ( usize i = 0; i < n; ++i ) output->states[output->written * output_stride + i] = state[i];
      ++output->written;
    }
  }
  F event_before = F(0);
  if constexpr ( !micron::is_same_v<Event, no_event> ) {
    event_before = __impl::runtime_event_value<F>(event, t0, state, n);
    if ( !ieee::is_finite<F>(event_before) ) {
      result_value.termination = status::non_finite;
      return result_value;
    }
    if ( event_before == F(0) ) {
      result_value.event_index = event.index;
      result_value.event_time = t0;
      if ( event.terminal ) {
        result_value.termination = status::event;
        result_value.t_eval_written = output == nullptr ? 0 : output->written;
        return result_value;
      }
    }
  }
  while ( direction * (result_value.t - t_bound) < F(0) ) {
    if ( result_value.attempted_steps >= settings.max_steps ) {
      result_value.termination = status::max_steps;
      break;
    }
    F min_step = settings.min_step;
    if ( min_step <= F(0) ) min_step = F(16) * machine_epsilon<F>() * (mk::manip::fabs<F>(result_value.t) + F(1));
    if ( step_abs < min_step ) {
      result_value.termination = status::step_underflow;
      break;
    }
    F h = direction * step_abs;
    if ( direction * (result_value.t + h - t_bound) > F(0) ) h = t_bound - result_value.t;
    ++result_value.attempted_steps;
    if ( !__impl::runtime_adaptive_step<Method>(function, result_value.t, state, n, h, workspace, result_value.n_evals) ) {
      result_value.termination = status::non_finite;
      break;
    }
    const F norm = __impl::runtime_error_norm(workspace.error, state, workspace.next, n, settings);
    if ( !ieee::is_finite<F>(norm) ) {
      result_value.termination = status::non_finite;
      break;
    }
    if ( norm > F(1) ) {
      ++result_value.rejected_steps;
      if ( ++consecutive_rejections > settings.rejection_limit ) {
        result_value.termination = status::rejection_limit;
        break;
      }
      rejected_before_accept = true;
      step_abs *= __impl::step_factor<Method>(norm, previous_error, settings, true);
      workspace.has_fsal = true;
      continue;
    }
    consecutive_rejections = 0;
    workspace.last_t0 = result_value.t;
    workspace.last_t1 = result_value.t + h;
    constexpr usize endpoint = Method == method::rk23 ? 3 : (Method == method::rk45 ? 6 : 12);
    F *first_stage = workspace.stages;
    F *last_stage = workspace.stages + endpoint * n;
    for ( usize i = 0; i < n; ++i ) {
      workspace.start[i] = state[i];
      workspace.start_derivative[i] = first_stage[i];
      workspace.end_derivative[i] = last_stage[i];
      state[i] = workspace.next[i];
      first_stage[i] = last_stage[i];
    }
    workspace.has_fsal = true;
    result_value.t += h;
    result_value.last_step = h;
    ++result_value.accepted_steps;

    bool terminal_event = false;
    if constexpr ( !micron::is_same_v<Event, no_event> ) {
      const F event_after = __impl::runtime_event_value<F>(event, result_value.t, state, n);
      if ( !ieee::is_finite<F>(event_after) ) {
        result_value.termination = status::non_finite;
        break;
      }
      if ( __impl::crossed<F>(event_before, event_after, event.direction) ) {
        const F root = __impl::runtime_locate_event<F>(event, workspace, n, event_before, event_after);
        result_value.event_index = event.index;
        result_value.event_time = root;
        if ( event.terminal ) {
          workspace.dense_evaluate(root, workspace.temporary, n);
          workspace.last_t1 = root;
          for ( usize i = 0; i < n; ++i ) state[i] = workspace.next[i] = workspace.temporary[i];
          __impl::runtime_rhs(function, root, state, workspace.end_derivative, n);
          ++result_value.n_evals;
          result_value.t = root;
          result_value.last_step = root - workspace.last_t0;
          result_value.termination = status::event;
          terminal_event = true;
        }
      }
      event_before = event_after;
    }

    __impl::runtime_write_outputs(workspace, direction, n, output);
    if ( !__impl::runtime_store_dense(workspace, n, dense_storage) ) {
      result_value.termination = status::storage_full;
      break;
    }
    if constexpr ( requires { observer(result_value.t, state, n); } ) observer(result_value.t, state, n);
    if ( terminal_event ) break;
    const F factor = __impl::step_factor<Method>(norm, previous_error, settings, rejected_before_accept);
    rejected_before_accept = false;
    previous_error = norm > F(1e-4L) ? norm : F(1e-4L);
    step_abs *= factor;
    if ( step_abs > max_step ) step_abs = max_step;
  }
  result_value.t_eval_written = output == nullptr ? 0 : output->written;
  result_value.dense_segments_written = dense_storage == nullptr ? 0 : dense_storage->written;
  return result_value;
}

template<method Method, ieee754_floating F, typename Rhs, typename Observer = null_observer>
  requires(Method == method::rk23 || Method == method::rk45 || Method == method::dop853)
[[nodiscard]] inline runtime_result<F>
solve_ivp(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
          const options<F> &settings, runtime_workspace<F> &workspace, Observer observer = {}, runtime_output_buffer<F> *output = nullptr,
          runtime_dense_buffer<F> *dense_storage = nullptr) noexcept
{
  return __solve_ivp_runtime<Method>(function, t0, state, n, t_bound, settings, workspace, observer, no_event{}, output, dense_storage);
}

template<method Method, ieee754_floating F, typename Rhs, typename Observer, typename Event>
  requires((Method == method::rk23 || Method == method::rk45 || Method == method::dop853)
           && requires(Event &event) {
                event.function;
                event.direction;
                event.terminal;
                event.index;
              })
[[nodiscard]] inline runtime_result<F>
solve_ivp(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
          const options<F> &settings, runtime_workspace<F> &workspace, Observer observer, Event event,
          runtime_output_buffer<F> *output = nullptr, runtime_dense_buffer<F> *dense_storage = nullptr) noexcept
{
  return __solve_ivp_runtime<Method>(function, t0, state, n, t_bound, settings, workspace, observer, event, output, dense_storage);
}

template<method Method, ieee754_floating F, typename Rhs>
  requires(Method == method::euler || Method == method::midpoint || Method == method::rk4)
[[nodiscard]] inline runtime_result<F>
integrate_fixed(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
                micron::__type_identity_t<F> step, runtime_workspace<F> &workspace, usize max_steps = 100000) noexcept
{
  runtime_result<F> result_value{};
  result_value.t = t0;
  constexpr usize required = Method == method::euler ? 2 : (Method == method::midpoint ? 3 : 5);
  if ( state == nullptr || step == F(0) || max_steps == 0 || !workspace.valid(n, required) || !ieee::is_finite<F>(step)
       || !ieee::is_finite<F>(t0) || !ieee::is_finite<F>(t_bound) || !__impl::runtime_finite(state, n) ) {
    result_value.termination = status::invalid_input;
    return result_value;
  }
  if ( t0 == t_bound ) return result_value;
  const F direction = t_bound > t0 ? F(1) : F(-1);
  const F step_abs = mk::manip::fabs<F>(step);
  auto stage = [&](usize s) noexcept -> F * { return workspace.stages + s * n; };
  while ( direction * (result_value.t - t_bound) < F(0) ) {
    if ( result_value.attempted_steps >= max_steps ) {
      result_value.termination = status::max_steps;
      break;
    }
    F h = direction * step_abs;
    if ( direction * (result_value.t + h - t_bound) > F(0) ) h = t_bound - result_value.t;
    if ( result_value.t + h == result_value.t ) {
      result_value.termination = status::step_underflow;
      break;
    }
    __impl::runtime_rhs(function, result_value.t, state, stage(0), n);
    if constexpr ( Method == method::euler ) {
      for ( usize i = 0; i < n; ++i ) workspace.next[i] = state[i] + h * stage(0)[i];
      __impl::runtime_rhs(function, result_value.t + h, workspace.next, stage(1), n);
      result_value.n_evals += 2;
    } else if constexpr ( Method == method::midpoint ) {
      for ( usize i = 0; i < n; ++i ) workspace.temporary[i] = state[i] + F(0.5) * h * stage(0)[i];
      __impl::runtime_rhs(function, result_value.t + F(0.5) * h, workspace.temporary, stage(1), n);
      for ( usize i = 0; i < n; ++i ) workspace.next[i] = state[i] + h * stage(1)[i];
      __impl::runtime_rhs(function, result_value.t + h, workspace.next, stage(2), n);
      result_value.n_evals += 3;
    } else {
      for ( usize i = 0; i < n; ++i ) workspace.temporary[i] = state[i] + F(0.5) * h * stage(0)[i];
      __impl::runtime_rhs(function, result_value.t + F(0.5) * h, workspace.temporary, stage(1), n);
      for ( usize i = 0; i < n; ++i ) workspace.temporary[i] = state[i] + F(0.5) * h * stage(1)[i];
      __impl::runtime_rhs(function, result_value.t + F(0.5) * h, workspace.temporary, stage(2), n);
      for ( usize i = 0; i < n; ++i ) workspace.temporary[i] = state[i] + h * stage(2)[i];
      __impl::runtime_rhs(function, result_value.t + h, workspace.temporary, stage(3), n);
      for ( usize i = 0; i < n; ++i )
        workspace.next[i] = state[i] + h * (stage(0)[i] + F(2) * stage(1)[i] + F(2) * stage(2)[i] + stage(3)[i]) / F(6);
      __impl::runtime_rhs(function, result_value.t + h, workspace.next, stage(4), n);
      result_value.n_evals += 5;
    }
    const usize endpoint = Method == method::euler ? 1 : (Method == method::midpoint ? 2 : 4);
    if ( !__impl::runtime_finite(workspace.next, n) || !__impl::runtime_finite(stage(endpoint), n) ) {
      result_value.termination = status::non_finite;
      break;
    }
    for ( usize i = 0; i < n; ++i ) {
      workspace.start[i] = state[i];
      workspace.start_derivative[i] = stage(0)[i];
      workspace.end_derivative[i] = stage(endpoint)[i];
      state[i] = workspace.next[i];
    }
    result_value.t += h;
    result_value.last_step = h;
    ++result_value.accepted_steps;
    ++result_value.attempted_steps;
  }
  return result_value;
}

template<ieee754_floating F, typename Rhs>
[[nodiscard]] inline runtime_result<F>
euler(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
      micron::__type_identity_t<F> step, runtime_workspace<F> &workspace) noexcept
{
  return integrate_fixed<method::euler>(function, t0, state, n, t_bound, step, workspace);
}

template<ieee754_floating F, typename Rhs>
[[nodiscard]] inline runtime_result<F>
midpoint(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
         micron::__type_identity_t<F> step, runtime_workspace<F> &workspace) noexcept
{
  return integrate_fixed<method::midpoint>(function, t0, state, n, t_bound, step, workspace);
}

template<ieee754_floating F, typename Rhs>
[[nodiscard]] inline runtime_result<F>
rk4(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound,
    micron::__type_identity_t<F> step, runtime_workspace<F> &workspace) noexcept
{
  return integrate_fixed<method::rk4>(function, t0, state, n, t_bound, step, workspace);
}

template<ieee754_floating F, typename Rhs, typename Observer = null_observer>
[[nodiscard]] inline runtime_result<F>
rk23(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound, const options<F> &settings,
     runtime_workspace<F> &workspace, Observer observer = {}) noexcept
{
  return solve_ivp<method::rk23>(function, t0, state, n, t_bound, settings, workspace, observer);
}

template<ieee754_floating F, typename Rhs, typename Observer = null_observer>
[[nodiscard]] inline runtime_result<F>
rk45(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound, const options<F> &settings,
     runtime_workspace<F> &workspace, Observer observer = {}) noexcept
{
  return solve_ivp<method::rk45>(function, t0, state, n, t_bound, settings, workspace, observer);
}

template<ieee754_floating F, typename Rhs, typename Observer = null_observer>
[[nodiscard]] inline runtime_result<F>
dop853(Rhs function, micron::__type_identity_t<F> t0, F *state, usize n, micron::__type_identity_t<F> t_bound, const options<F> &settings,
       runtime_workspace<F> &workspace, Observer observer = {}) noexcept
{
  return solve_ivp<method::dop853>(function, t0, state, n, t_bound, settings, workspace, observer);
}

};      // namespace ode
};      // namespace integrate
};      // namespace math
};      // namespace micron
