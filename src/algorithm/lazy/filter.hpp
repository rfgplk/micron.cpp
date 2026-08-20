//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

namespace micron
{
namespace lz
{

template<typename V, typename P> class filter_view: public micron::view_interface<filter_view<V, P>>
{
  V __base;
  [[no_unique_address]] P __pred;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {

    mutable __ui __i{};
    __us __e{};
    [[no_unique_address]] fn_carrier<P> __p{};

    constexpr void
    __seek()
    {
      while ( !(__i == __e) && !call<P>(__p, *__i) ) ++__i;
    }

  public:
    using value_type = micron::ranges::range_value_t<V>;
    using reference = micron::ranges::range_reference_t<V>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, const P &__pp) : __i(__ii), __e(__ee), __p(hold<P>(__pp)) { __seek(); }

    constexpr reference
    operator*() const
    {
      return *__i;
    }

    constexpr iterator &
    operator++()
    {
      ++__i;
      __seek();
      return *this;
    }

    constexpr iterator
    operator++(int)
    {
      iterator __t = *this;
      ++*this;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const
    {
      return __i == __e;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr filter_view(V __v, P __p) : __base(micron::move(__v)), __pred(micron::move(__p)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __pred };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  template<typename T>
  constexpr T *
  __drain_into(T *__restrict __dst) const
    requires(flat_exact<V>)
  {
    const auto *__f = micron::ranges::begin(__base);
    const auto *__l = micron::ranges::end(__base);
    const fn_carrier<P> __p = hold<P>(__pred);
    for ( ; __f != __l; ++__f )
      if ( call<P>(__p, *__f) ) *__dst++ = static_cast<T>(*__f);
    return __dst;
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

template<typename P> struct __filter_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return filter_view<micron::remove_cvref_t<decltype(__v)>, P>{ micron::move(__v), __p };
  }
};

template<typename P>
[[nodiscard]] constexpr auto
filter(P &&__p)
{
  return __filter_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename P> struct __negate_pred {
  [[no_unique_address]] P __p;

  template<typename T>
  constexpr bool
  operator()(T &&__t) const
  {
    return !__p(micron::forward<T>(__t));
  }
};

template<typename P>
[[nodiscard]] constexpr auto
reject(P &&__p)
{
  return filter(__negate_pred<micron::decay_t<P>>{ micron::forward<P>(__p) });
}

};      // namespace lz
};      // namespace micron
