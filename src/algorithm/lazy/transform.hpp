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

template<typename V, typename F> class transform_view: public micron::view_interface<transform_view<V, F>>
{
  V __base;
  [[no_unique_address]] F __fn;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::remove_cvref_t<micron::invoke_result_t<const F &, micron::ranges::range_reference_t<V>>>;

  static constexpr size_kind __kind = kind_of<V>;
  static constexpr usize __static_size = static_size_of<V>;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {

    mutable __ui __i{};
    __us __e{};
    [[no_unique_address]] fn_carrier<F> __f{};

  public:
    using value_type = transform_view::value_type;
    using reference = micron::invoke_result_t<const F &, micron::ranges::range_reference_t<V>>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, const F &__ff) : __i(__ii), __e(__ee), __f(hold<F>(__ff)) { }

    constexpr reference
    operator*() const
    {
      return call<F>(__f, *__i);
    }

    constexpr iterator &
    operator++()
    {
      ++__i;
      return *this;
    }

    constexpr iterator
    operator++(int)
    {
      iterator __t = *this;
      ++__i;
      return __t;
    }

    constexpr iterator &
    operator--()
      requires micron::bidirectional_iterator<__ui>
    {
      --__i;
      return *this;
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

  constexpr transform_view(V __v, F __f) : __base(micron::move(__v)), __fn(micron::move(__f)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __fn };
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
    return static_cast<usize>(micron::ranges::size(__base));
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

template<typename F> struct __fmap_fn {
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return transform_view<micron::remove_cvref_t<decltype(__v)>, F>{ micron::move(__v), __f };
  }
};

template<typename F>
[[nodiscard]] constexpr auto
fmap(F &&__f)
{
  return __fmap_fn<micron::decay_t<F>>{ micron::forward<F>(__f) };
}

template<typename F, typename R>
[[nodiscard]] constexpr auto
fmap(F &&__f, R &&__r)
{
  return fmap(micron::forward<F>(__f))(micron::forward<R>(__r));
}

template<typename F>
[[nodiscard]] constexpr auto
transform(F &&__f)
{
  return fmap(micron::forward<F>(__f));
}

};      // namespace lz
};      // namespace micron
