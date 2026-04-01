//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../function.hpp"
#include "../sum.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "arith.hpp"
#include "fperrors.hpp"
#include "unroll.hpp"

namespace micron
{
namespace fp
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// curried scalar arithmetic
template <typename Y>
  requires micron::is_arithmetic_v<Y>
auto
add_c(const Y y) noexcept
{
  return [y](auto cont) noexcept {
    if constexpr ( has_static_size<decltype(cont)> ) {
      __impl::__unroll_add(cont.begin(), y, make_index_sequence<decltype(cont)::static_size>{});
    } else {
      micron::add(cont, y);
    }
    return cont;
  };
}

template <typename Y>
  requires micron::is_arithmetic_v<Y>
auto
subtract_c(const Y y) noexcept
{
  return [y](auto cont) noexcept {
    if constexpr ( has_static_size<decltype(cont)> ) {
      __impl::__unroll_subtract(cont.begin(), y, make_index_sequence<decltype(cont)::static_size>{});
    } else {
      micron::subtract(cont, y);
    }
    return cont;
  };
}

template <typename Y>
  requires micron::is_arithmetic_v<Y>
auto
multiply_c(const Y y) noexcept
{
  return [y](auto cont) noexcept {
    if constexpr ( has_static_size<decltype(cont)> ) {
      __impl::__unroll_multiply(cont.begin(), y, make_index_sequence<decltype(cont)::static_size>{});
    } else {
      micron::multiply(cont, y);
    }
    return cont;
  };
}

template <typename Y>
  requires micron::is_arithmetic_v<Y>
auto
divide_c(const Y y) noexcept
{
  return [y](auto cont) noexcept {
    if constexpr ( has_static_size<decltype(cont)> ) {
      __impl::__unroll_divide(cont.begin(), y, make_index_sequence<decltype(cont)::static_size>{});
    } else {
      micron::divide(cont, y);
    }
    return cont;
  };
}

template <typename Y>
auto
pow_c(const Y y) noexcept
{
  return [y](auto cont) noexcept {
    micron::pow(cont, y);
    return cont;
  };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// safe_divide
template <is_iterable_container C, typename Y>
  requires micron::is_arithmetic_v<Y>
micron::option<C, division_by_zero_error>
safe_divide(C cont, const Y y) noexcept
{
  if ( y == Y{} )
    return micron::option<C, division_by_zero_error>{ division_by_zero_error{} };
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_divide(cont.begin(), y, make_index_sequence<C::static_size>{});
  } else {
    micron::divide(cont, y);
  }
  return micron::option<C, division_by_zero_error>{ micron::move(cont) };
}

template <typename Y>
  requires micron::is_arithmetic_v<Y>
auto
safe_divide_c(const Y y) noexcept
{
  return [y](auto cont) noexcept { return safe_divide(micron::move(cont), y); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// option-lifted scalar arithmetic
template <is_iterable_container C, typename E, typename Y>
  requires micron::is_arithmetic_v<Y>
micron::option<C, E>
add(micron::option<C, E> opt, const Y y) noexcept
{
  if ( !opt.is_first() )
    return opt;
  auto cont = opt.template cast<C>();
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_add(cont.begin(), y, make_index_sequence<C::static_size>{});
  } else {
    micron::add(cont, y);
  }
  return micron::option<C, E>{ micron::move(cont) };
}

template <is_iterable_container C, typename E, typename Y>
  requires micron::is_arithmetic_v<Y>
micron::option<C, E>
subtract(micron::option<C, E> opt, const Y y) noexcept
{
  if ( !opt.is_first() )
    return opt;
  auto cont = opt.template cast<C>();
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_subtract(cont.begin(), y, make_index_sequence<C::static_size>{});
  } else {
    micron::subtract(cont, y);
  }
  return micron::option<C, E>{ micron::move(cont) };
}

template <is_iterable_container C, typename E, typename Y>
  requires micron::is_arithmetic_v<Y>
micron::option<C, E>
multiply(micron::option<C, E> opt, const Y y) noexcept
{
  if ( !opt.is_first() )
    return opt;
  auto cont = opt.template cast<C>();
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_multiply(cont.begin(), y, make_index_sequence<C::static_size>{});
  } else {
    micron::multiply(cont, y);
  }
  return micron::option<C, E>{ micron::move(cont) };
}

template <is_iterable_container C, typename E, typename Y>
  requires micron::is_arithmetic_v<Y>
micron::option<C, division_by_zero_error>
divide(micron::option<C, E> opt, const Y y) noexcept
{
  if ( !opt.is_first() )
    return micron::option<C, division_by_zero_error>{ division_by_zero_error{} };
  if ( y == Y{} )
    return micron::option<C, division_by_zero_error>{ division_by_zero_error{} };
  auto cont = opt.template cast<C>();
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_divide(cont.begin(), y, make_index_sequence<C::static_size>{});
  } else {
    micron::divide(cont, y);
  }
  return micron::option<C, division_by_zero_error>{ micron::move(cont) };
}

template <is_iterable_container C, typename E, typename Y>
micron::option<C, E>
pow(micron::option<C, E> opt, const Y y) noexcept
{
  if ( !opt.is_first() )
    return opt;
  auto cont = opt.template cast<C>();
  micron::pow(cont, y);
  return micron::option<C, E>{ micron::move(cont) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// element-wise zip arithmetic
template <is_iterable_container C>
C
add_zip(C a, const C &b) noexcept
{
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_zip_add(a.begin(), b.begin(), make_index_sequence<C::static_size>{});
  } else {
    auto *fa = a.begin();
    const auto *ea = a.end();
    const auto *fb = b.begin();
    for ( ; fa != ea; ++fa, ++fb )
      *fa = *fa + *fb;
  }
  return a;
}

template <is_iterable_container C>
C
subtract_zip(C a, const C &b) noexcept
{
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_zip_subtract(a.begin(), b.begin(), make_index_sequence<C::static_size>{});
  } else {
    auto *fa = a.begin();
    const auto *ea = a.end();
    const auto *fb = b.begin();
    for ( ; fa != ea; ++fa, ++fb )
      *fa = *fa - *fb;
  }
  return a;
}

template <is_iterable_container C>
C
multiply_zip(C a, const C &b) noexcept
{
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_zip_multiply(a.begin(), b.begin(), make_index_sequence<C::static_size>{});
  } else {
    auto *fa = a.begin();
    const auto *ea = a.end();
    const auto *fb = b.begin();
    for ( ; fa != ea; ++fa, ++fb )
      *fa = *fa * *fb;
  }
  return a;
}

template <is_iterable_container C>
micron::option<C, division_by_zero_error>
divide_zip(C a, const C &b) noexcept
{
  auto *fa = a.begin();
  const auto *ea = a.end();
  const auto *fb = b.begin();
  for ( ; fa != ea; ++fa, ++fb ) {
    if ( *fb == typename C::value_type{} )
      return micron::option<C, division_by_zero_error>{ division_by_zero_error{} };
    *fa = *fa / *fb;
  }
  return micron::option<C, division_by_zero_error>{ micron::move(a) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// inner_product
template <is_iterable_container C, typename R = typename C::value_type>
R
inner_product(const C &a, const C &b, R init = R{}) noexcept
{
  if constexpr ( has_static_size<C> ) {
    return __impl::__unroll_inner_product(a.begin(), b.begin(), init, make_index_sequence<C::static_size>{});
  } else {
    const auto *fa = a.begin();
    const auto *ea = a.end();
    const auto *fb = b.begin();
    for ( ; fa != ea; ++fa, ++fb )
      init = init + (*fa * *fb);
    return init;
  }
}

template <is_iterable_container C, typename R, typename AddFn, typename MulFn>
  requires fn_fold<AddFn, R, R> && fn_binary_codomain<MulFn, typename C::value_type>
R
inner_product(const C &a, const C &b, R init, AddFn add_fn, MulFn mul_fn) noexcept
{
  const auto *fa = a.begin();
  const auto *ea = a.end();
  const auto *fb = b.begin();
  for ( ; fa != ea; ++fa, ++fb )
    init = add_fn(micron::move(init), mul_fn(*fa, *fb));
  return init;
}

template <is_iterable_container C, typename R, typename AddFn, typename MulFn>
  requires fn_fold<AddFn, R, R> && micron::is_invocable_v<MulFn, const typename C::value_type *, const typename C::value_type *>
           && (!fn_binary_codomain<MulFn, typename C::value_type>)
R
inner_product(const C &a, const C &b, R init, AddFn add_fn, MulFn mul_fn) noexcept
{
  const auto *fa = a.begin();
  const auto *ea = a.end();
  const auto *fb = b.begin();
  for ( ; fa != ea; ++fa, ++fb )
    init = add_fn(micron::move(init), mul_fn(fa, fb));
  return init;
}

template <is_iterable_container C, typename R>
R
inner_product(const C &a, const C &b, R init, micron::function<R(R, R)> add_fn,
              micron::function<typename C::value_type(typename C::value_type, typename C::value_type)> mul_fn) noexcept
{
  const auto *fa = a.begin();
  const auto *ea = a.end();
  const auto *fb = b.begin();
  for ( ; fa != ea; ++fa, ++fb )
    init = add_fn(micron::move(init), mul_fn(*fa, *fb));
  return init;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// negate
template <is_iterable_container C>
C
negate(C cont) noexcept
{
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_negate(cont.begin(), make_index_sequence<C::static_size>{});
  } else {
    auto *first = cont.begin();
    const auto *end = cont.end();
    for ( ; first != end; ++first )
      *first = -(*first);
  }
  return cont;
}

template <is_iterable_container C, typename E>
micron::option<C, E>
negate(micron::option<C, E> opt) noexcept
{
  if ( !opt.is_first() )
    return opt;
  return micron::option<C, E>{ fp::negate(opt.template cast<C>()) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// abs
template <is_iterable_container C>
  requires micron::is_signed_v<typename C::value_type>
C
abs(C cont) noexcept
{
  auto *first = cont.begin();
  const auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = (*first < typename C::value_type{}) ? -(*first) : *first;
  return cont;
}

template <is_iterable_container C, typename E>
  requires micron::is_signed_v<typename C::value_type>
micron::option<C, E>
abs(micron::option<C, E> opt) noexcept
{
  if ( !opt.is_first() )
    return opt;
  return micron::option<C, E>{ fp::abs(opt.template cast<C>()) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// safe_inner_product
template <is_iterable_container C, typename R = typename C::value_type>
micron::option<R, bad_zip_error>
safe_inner_product(const C &a, const C &b, R init = R{}) noexcept
{
  if ( a.size() != b.size() )
    return micron::option<R, bad_zip_error>{ bad_zip_error{} };
  return micron::option<R, bad_zip_error>{ fp::inner_product(a, b, init) };
}

};     // namespace fp
};     // namespace micron
