//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// WARNING: **minimal** NN activation implementation

#include "../concepts.hpp"
#include "../types.hpp"
#include "log.hpp"
#include "mk.hpp"

namespace micron
{
namespace math
{
namespace activation
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
relu(F x) noexcept
{
  return x > F(0) ? x : F(0);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
leaky_relu(F x, F alpha = F(0.01)) noexcept
{
  return x > F(0) ? x : alpha * x;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
elu(F x, F alpha = F(1)) noexcept
{
  return x > F(0) ? x : alpha * mk::exp_ns::expm1<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
gelu(F x) noexcept
{
  constexpr F inv_sqrt2 = F(0.7071067811865475);
  return F(0.5) * x * (F(1) + mk::special::erf<F>(x * inv_sqrt2));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
gelu_approx(F x) noexcept
{
  constexpr F sqrt_2_over_pi = F(0.7978845608028654);
  const F x3 = x * x * x;
  const F u = sqrt_2_over_pi * (x + F(0.044715) * x3);
  return F(0.5) * x * (F(1) + mk::hyp::tanh<F>(u));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
silu(F x) noexcept
{
  return x * logistic<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
swish(F x, F beta = F(1)) noexcept
{
  return x * logistic<F>(beta * x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
mish(F x) noexcept
{
  return x * mk::hyp::tanh<F>(softplus<F>(x));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
hard_tanh(F x, F lo = F(-1), F hi = F(1)) noexcept
{
  return x < lo ? lo : (x > hi ? hi : x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
softsign(F x) noexcept
{
  return x / (F(1) + mk::manip::fabs<F>(x));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
hardswish(F x) noexcept
{
  if ( x <= F(-3) ) return F(0);
  if ( x >= F(3) ) return x;
  return x * (x + F(3)) * F(1.0 / 6.0);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
threshold(F x, F t, F v) noexcept
{
  return x > t ? x : v;
}

#define __micron_activations_bfn(NAME)                                                                                                     \
  template<ieee754_floating F> [[gnu::flatten]] inline void NAME(F *__restrict__ x, usize n) noexcept                                      \
  {                                                                                                                                        \
    for ( usize i = 0; i < n; ++i ) x[i] = NAME<F>(x[i]);                                                                                  \
  }                                                                                                                                        \
  template<ieee754_floating F> [[gnu::flatten]] inline void NAME(const F *__restrict__ in, F *__restrict__ out, usize n) noexcept          \
  {                                                                                                                                        \
    for ( usize i = 0; i < n; ++i ) out[i] = NAME<F>(in[i]);                                                                               \
  }                                                                                                                                        \
  template<is_iterable_container C>                                                                                                        \
    requires ieee754_floating<typename C::value_type>                                                                                      \
  [[gnu::flatten]] inline void NAME(C &c) noexcept                                                                                         \
  {                                                                                                                                        \
    NAME<typename C::value_type>(c.begin(), c.size());                                                                                     \
  }                                                                                                                                        \
  template<is_iterable_container C>                                                                                                        \
    requires ieee754_floating<typename C::value_type>                                                                                      \
  [[gnu::flatten]] inline void NAME(const C &in, C &out) noexcept                                                                          \
  {                                                                                                                                        \
    NAME<typename C::value_type>(in.cbegin(), out.begin(), in.size());                                                                     \
  }                                                                                                                                        \
  template<typename... Cs>                                                                                                                 \
    requires(sizeof...(Cs) >= 2) && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<Cs> && ...)                                 \
            && (ieee754_floating<typename Cs::value_type> && ...)                                                                          \
  [[gnu::flatten]] inline void NAME(Cs &...cs) noexcept                                                                                    \
  {                                                                                                                                        \
    ((NAME<typename Cs::value_type>(cs.begin(), cs.size())), ...);                                                                         \
  }

__micron_activations_bfn(relu);
__micron_activations_bfn(gelu);
__micron_activations_bfn(gelu_approx);
__micron_activations_bfn(silu);
__micron_activations_bfn(mish);
__micron_activations_bfn(softsign);
__micron_activations_bfn(hardswish);

#undef __micron_activations_bfn

template<ieee754_floating F>
[[gnu::flatten]] inline void
leaky_relu(F *__restrict__ x, usize n, F alpha = F(0.01)) noexcept
{
  for ( usize i = 0; i < n; ++i ) x[i] = leaky_relu<F>(x[i], alpha);
}

template<ieee754_floating F>
[[gnu::flatten]] inline void
leaky_relu(const F *__restrict__ in, F *__restrict__ out, usize n, F alpha = F(0.01)) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = leaky_relu<F>(in[i], alpha);
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
leaky_relu(C &c, typename C::value_type alpha = typename C::value_type(0.01)) noexcept
{
  leaky_relu<typename C::value_type>(c.begin(), c.size(), alpha);
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
leaky_relu(const C &in, C &out, typename C::value_type alpha = typename C::value_type(0.01)) noexcept
{
  leaky_relu<typename C::value_type>(in.cbegin(), out.begin(), in.size(), alpha);
}

template<typename C, typename... Cs>
  requires(sizeof...(Cs) >= 1) && is_iterable_container<C> && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<C>)
          && (!micron::is_const_v<Cs> && ...) && ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
leaky_relu(typename C::value_type alpha, C &c, Cs &...cs) noexcept
{
  leaky_relu<typename C::value_type>(c.begin(), c.size(), alpha);
  ((leaky_relu<typename Cs::value_type>(cs.begin(), cs.size(), typename Cs::value_type(alpha))), ...);
}

template<typename... Cs>
  requires(sizeof...(Cs) >= 2) && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<Cs> && ...)
          && (ieee754_floating<typename Cs::value_type> && ...)
[[gnu::flatten]] inline void
leaky_relu(Cs &...cs) noexcept
{
  ((leaky_relu<typename Cs::value_type>(cs.begin(), cs.size(), typename Cs::value_type(0.01))), ...);
}

template<ieee754_floating F>
[[gnu::flatten]] inline void
elu(F *__restrict__ x, usize n, F alpha = F(1)) noexcept
{
  for ( usize i = 0; i < n; ++i ) x[i] = elu<F>(x[i], alpha);
}

template<ieee754_floating F>
[[gnu::flatten]] inline void
elu(const F *__restrict__ in, F *__restrict__ out, usize n, F alpha = F(1)) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = elu<F>(in[i], alpha);
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
elu(C &c, typename C::value_type alpha = typename C::value_type(1)) noexcept
{
  elu<typename C::value_type>(c.begin(), c.size(), alpha);
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
elu(const C &in, C &out, typename C::value_type alpha = typename C::value_type(1)) noexcept
{
  elu<typename C::value_type>(in.cbegin(), out.begin(), in.size(), alpha);
}

template<typename C, typename... Cs>
  requires(sizeof...(Cs) >= 1) && is_iterable_container<C> && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<C>)
          && (!micron::is_const_v<Cs> && ...) && ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
elu(typename C::value_type alpha, C &c, Cs &...cs) noexcept
{
  elu<typename C::value_type>(c.begin(), c.size(), alpha);
  ((elu<typename Cs::value_type>(cs.begin(), cs.size(), typename Cs::value_type(alpha))), ...);
}

template<typename... Cs>
  requires(sizeof...(Cs) >= 2) && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<Cs> && ...)
          && (ieee754_floating<typename Cs::value_type> && ...)
[[gnu::flatten]] inline void
elu(Cs &...cs) noexcept
{
  ((elu<typename Cs::value_type>(cs.begin(), cs.size(), typename Cs::value_type(1))), ...);
}

};      // namespace activation
};      // namespace math
};      // namespace micron
