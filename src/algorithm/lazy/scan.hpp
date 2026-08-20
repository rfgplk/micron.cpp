//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "order.hpp"
#include "source.hpp"

#include "../../vector/fvector.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scanl / scan / scanr

namespace micron
{
namespace lz
{

template<typename V, typename A, typename F> class scanl_view: public micron::view_interface<scanl_view<V, A, F>>
{
  V __base;
  A __init;
  [[no_unique_address]] F __fn;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = A;

  static constexpr size_kind __kind = kind_of<V>;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    A __acc{};
    bool __done = false;
    [[no_unique_address]] fn_carrier<F> __f{};

  public:
    using value_type = A;
    using reference = const A &;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, A __a, const F &__ff) : __i(__ii), __e(__ee), __acc(__a), __f(hold<F>(__ff)) { }

    constexpr const A &
    operator*() const
    {
      return __acc;
    }

    constexpr iterator &
    operator++()
    {
      if ( __i == __e ) {
        __done = true;
        return *this;
      }
      __acc = call<F>(__f, __acc, *__i);
      ++__i;
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
      return __done;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr scanl_view(V __v, A __a, F __f) : __base(micron::move(__v)), __init(__a), __fn(micron::move(__f)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __init, __fn };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    return static_cast<usize>(micron::ranges::size(__base)) + 1u;
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base)) + 1u;
  }
};

template<typename A, typename F> struct __scanl_fn {
  A __init;
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return scanl_view<micron::remove_cvref_t<decltype(__v)>, A, F>{ micron::move(__v), __init, __f };
  }
};

template<typename A, typename F>
[[nodiscard]] constexpr auto
scanl(A __init, F &&__f)
{
  return __scanl_fn<A, micron::decay_t<F>>{ micron::move(__init), micron::forward<F>(__f) };
}

template<typename A, typename F>
[[nodiscard]] constexpr auto
scan(A __init, F &&__f)
{
  return scanl(micron::move(__init), micron::forward<F>(__f));
}

template<typename A, typename F> struct __scanr_fn {
  [[no_unique_address]] F __f;
  A __init;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless,
                  "micron::lz::scanr: this pipeline never terminates, and scanr's first output needs its last input");

    micron::fvector<A> __out;

    if constexpr ( reversible<B> ) {
      const usize __n = count_into(__v);
      __out.resize(__n + 1u);
      A __carry = __init;
      __out[__n] = __carry;
      usize __k = __n;
      auto __b = micron::ranges::begin(__v);
      auto __i = micron::ranges::end(__v);
      while ( !(__i == __b) ) {
        --__i;
        --__k;
        __carry = __f(*__i, __carry);
        __out[__k] = __carry;
      }
    } else {
      auto __buf = __impl::__drain(__v);
      const usize __n = __buf.size();
      __out.resize(__n + 1u);
      A __carry = __init;
      __out[__n] = __carry;
      for ( usize __k = __n; __k-- > 0; ) {
        __carry = __f(__buf[__k], __carry);
        __out[__k] = __carry;
      }
    }
    return buffer_view<A>{ micron::move(__out) };
  }
};

template<typename F, typename A>
[[nodiscard]] constexpr auto
scanr(F &&__f, A __init)
{
  return __scanr_fn<A, micron::decay_t<F>>{ micron::forward<F>(__f), micron::move(__init) };
}

};      // namespace lz
};      // namespace micron
