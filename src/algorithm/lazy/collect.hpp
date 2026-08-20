//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../unroll.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  exact   -> resize(n) then a raw-pointer fill
//  bounded -> resize(hint), fill, resize(written)
//  unknown -> reserve(hint) if possible, then push_back

namespace micron
{
namespace lz
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//   micron::vector      resize + reserve + push_back
//   micron::fvector     same, but move only
//   micron::svector     resize/reserve are = delete'd; push_back only, and it throws on overflow
//   micron::array<T,N>  none of the three; fixed N, written through operator[]
template<typename O>
concept sizable = requires(O &__o, usize __n) { __o.resize(__n); };

template<typename O>
concept reservable = requires(O &__o, usize __n) { __o.reserve(__n); };

template<typename O>
concept pushable = requires(O &__o, typename O::value_type &&__v) { __o.push_back(micron::move(__v)); };

template<typename O>
concept size_constructible = sizable<O> && requires(usize __n) { O(__n); };

namespace __impl
{

template<typename O>
constexpr O
__sized(usize __n)
{
  if constexpr ( size_constructible<O> ) {
    return O(__n);
  } else {
    O __o;
    __o.resize(__n);
    return __o;
  }
}

template<typename Out, typename V, usize... Is>
[[gnu::always_inline]] constexpr void
__fill_unrolled(Out &__out, V &__v, micron::index_sequence<Is...>)
{
  auto __i = micron::ranges::begin(__v);
  ((__out[Is] = *__i, ++__i), ...);
}

};      // namespace __impl

template<typename Out, typename V>
constexpr Out
collect_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless,
                "micron::lz::collect: this pipeline never terminates -- put a take(n) or take_while(p) before it");

  if constexpr ( kind_of<B> == size_kind::exact && sizable<Out> ) {
    const usize __n = static_cast<usize>(micron::ranges::reserve_hint(__v));
    Out __out = __impl::__sized<Out>(__n);
    auto *__restrict __dst = __out.begin();
    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    for ( ; !(__i == __e); ++__i, ++__dst ) *__dst = *__i;
    return __out;
  }

  else if constexpr ( micron::has_static_size<Out> && !sizable<Out> && !pushable<Out> ) {
    Out __out;
    static_assert(static_size_of<B> == micron::remove_cvref_t<Out>::static_size,
                  "micron::lz::collect<array<T,N>>: the pipeline's compile-time extent does not match N. "
                  "filter / take(n) / generators erase it -- collect into a vector instead");
    if constexpr ( static_size_of<B> != no_static_size && static_size_of<B> <= micron::__unroll_max ) {
      __impl::__fill_unrolled(__out, __v, micron::make_index_sequence<static_size_of<B>>{});
    } else {
      auto __i = micron::ranges::begin(__v);
      for ( usize __k = 0; __k < micron::remove_cvref_t<Out>::static_size; ++__k, ++__i ) __out[__k] = *__i;
    }
    return __out;
  }

  else if constexpr ( kind_of<B> == size_kind::bounded ) {
    const usize __n = static_cast<usize>(micron::ranges::reserve_hint(__v));
    if constexpr ( sizable<Out> ) {
      Out __out = __impl::__sized<Out>(__n);
      auto *__restrict __dst = __out.begin();

      if constexpr ( requires(const B &__b, decltype(__dst) __p) { __b.__drain_into(__p); } ) {
        __dst = __v.__drain_into(__dst);
      } else {
        auto __i = micron::ranges::begin(__v);
        const auto __e = micron::ranges::end(__v);
        for ( ; !(__i == __e); ++__i, ++__dst ) *__dst = *__i;
      }
      __out.resize(static_cast<usize>(__dst - __out.begin()));
      return __out;
    } else {
      static_assert(pushable<Out>, "micron::lz::collect: Out has neither resize() nor push_back()");
      Out __out;
      if constexpr ( reservable<Out> ) __out.reserve(__n);
      auto __i = micron::ranges::begin(__v);
      const auto __e = micron::ranges::end(__v);
      for ( ; !(__i == __e); ++__i ) __out.push_back(*__i);
      return __out;
    }
  }

  else {
    static_assert(pushable<Out>, "micron::lz::collect: an unknown-length pipeline needs an Out with push_back()");
    Out __out;
    if constexpr ( reservable<Out> ) {
      const usize __n = static_cast<usize>(micron::ranges::reserve_hint(__v));
      if ( __n ) __out.reserve(__n);
    }
    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    for ( ; !(__i == __e); ++__i ) __out.push_back(*__i);
    return __out;
  }
}

template<typename Out> struct __collect_fn {
  template<typename R>
  constexpr Out
  operator()(R &&__r) const
  {
    return collect_into<Out>(__as_view(micron::forward<R>(__r)));
  }
};

template<typename Out>
[[nodiscard]] constexpr auto
collect() noexcept
{
  return __collect_fn<Out>{};
}

template<typename Out>
[[nodiscard]] constexpr auto
to() noexcept
{
  return __collect_fn<Out>{};
}

};      // namespace lz
};      // namespace micron
