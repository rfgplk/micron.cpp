//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "flatten.hpp"
#include "source.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// intersperse / intercalate / step_by
namespace micron
{
namespace lz
{

template<typename V> class intersperse_view: public micron::view_interface<intersperse_view<V>>
{
  using __T = micron::ranges::range_value_t<V>;

  V __base;
  __T __sep;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = __T;

  static constexpr size_kind __kind = kind_of<V>;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    const __T *__sp = nullptr;
    bool __on_sep = false;
    bool __live = false;

  public:
    using value_type = __T;
    using reference = const __T &;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, const __T *__s) : __i(__ii), __e(__ee), __sp(__s) { __live = !(__i == __e); }

    constexpr const __T &
    operator*() const
    {

      return __on_sep ? *__sp : *__i;
    }

    constexpr iterator &
    operator++()
    {
      if ( __on_sep ) {
        __on_sep = false;
        return *this;
      }
      ++__i;
      if ( __i == __e ) {
        __live = false;
        return *this;
      }
      __on_sep = true;
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
      return !__live;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr intersperse_view(V __v, __T __s) : __base(micron::move(__v)), __sep(__s) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), micron::addressof(__sep) };
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
    const usize __n = static_cast<usize>(micron::ranges::size(__base));
    return __n ? __n + __n - 1u : 0u;
  }

  constexpr usize
  reserve_hint() const
  {
    const usize __n = static_cast<usize>(micron::ranges::reserve_hint(__base));
    return __n ? __n + __n - 1u : 0u;
  }
};

template<typename T> struct __intersperse_fn {
  T __sep;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return intersperse_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v), __sep };
  }
};

template<typename T>
[[nodiscard]] constexpr auto
intersperse(T __sep)
{
  return __intersperse_fn<T>{ __sep };
}

template<typename T>
[[nodiscard]] constexpr auto
intersperse_c(T __sep)
{
  return intersperse(__sep);
}

template<typename Sep> struct __intercalate_fn {
  Sep __sep;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return flatten()(intersperse(__sep)(micron::forward<R>(__r)));
  }
};

template<typename Sep>
[[nodiscard]] constexpr auto
intercalate(Sep __sep)
{
  return __intercalate_fn<Sep>{ micron::move(__sep) };
}

template<typename V> class step_view: public micron::view_interface<step_view<V>>
{
  V __base;
  usize __n;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = (kind_of<V> == size_kind::endless) ? size_kind::endless : degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    usize __step = 1;

  public:
    using value_type = micron::ranges::range_value_t<V>;
    using reference = micron::ranges::range_reference_t<V>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, usize __s) : __i(__ii), __e(__ee), __step(__s ? __s : 1u) { }

    constexpr reference
    operator*() const
    {
      return *__i;
    }

    constexpr iterator &
    operator++()
    {
      for ( usize __k = 0; __k < __step && !(__i == __e); ++__k ) ++__i;
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

  constexpr step_view(V __v, usize __s) : __base(micron::move(__v)), __n(__s ? __s : 1u) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __n };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const
  {
    const usize __u = static_cast<usize>(micron::ranges::reserve_hint(__base));
    return (__u + __n - 1u) / __n;
  }
};

struct __step_by_fn {
  usize __n;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return step_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v), __n };
  }
};

[[nodiscard]] constexpr auto
step_by(usize __n) noexcept
{
  return __step_by_fn{ __n };
}

};      // namespace lz
};      // namespace micron
