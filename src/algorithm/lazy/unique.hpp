//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../fpfilter.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unique / nub / nub_by

namespace micron
{
namespace lz
{

template<typename V, typename Eq> class unique_view: public micron::view_interface<unique_view<V, Eq>>
{
  V __base;
  [[no_unique_address]] Eq __eq;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;
  using __T = micron::ranges::range_value_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = __T;

  static constexpr size_kind __kind = degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    __T __kept{};
    bool __live = false;
    [[no_unique_address]] fn_carrier<Eq> __q{};

  public:
    using value_type = __T;
    using reference = const __T &;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, const Eq &__qq) : __i(__ii), __e(__ee), __q(hold<Eq>(__qq))
    {
      if ( !(__i == __e) ) {
        __kept = *__i;
        __live = true;
      }
    }

    constexpr const __T &
    operator*() const
    {
      return __kept;
    }

    constexpr iterator &
    operator++()
    {

      ++__i;
      while ( !(__i == __e) ) {
        auto &&__x = *__i;
        if ( !call<Eq>(__q, static_cast<const __T &>(__x), static_cast<const __T &>(__kept)) ) {
          __kept = static_cast<__T>(__x);
          return *this;
        }
        ++__i;
      }
      __live = false;
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

  constexpr unique_view(V __v, Eq __q) : __base(micron::move(__v)), __eq(micron::move(__q)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __eq };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

namespace __impl
{

struct __eq_to {
  template<typename A, typename B>
  constexpr bool
  operator()(const A &__a, const B &__b) const
  {
    return __a == __b;
  }
};

};      // namespace __impl

template<typename Eq> struct __unique_by_fn {
  [[no_unique_address]] Eq __q;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return unique_view<micron::remove_cvref_t<decltype(__v)>, Eq>{ micron::move(__v), __q };
  }
};

[[nodiscard]] constexpr auto
unique() noexcept
{
  return __unique_by_fn<__impl::__eq_to>{ {} };
}

template<typename Eq>
[[nodiscard]] constexpr auto
unique(Eq &&__q)
{
  return __unique_by_fn<micron::decay_t<Eq>>{ micron::forward<Eq>(__q) };
}

template<typename V> class nub_view: public micron::view_interface<nub_view<V>>
{
  V __base;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;
  using __T = micron::ranges::range_value_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = __T;

  static constexpr size_kind __kind = degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    fp::__impl::seen_set<__T> __seen{};
    bool __live = false;

    constexpr void
    __seek()
    {
      while ( !(__i == __e) ) {
        if ( __seen.add(static_cast<const __T &>(*__i)) ) {
          __live = true;
          return;
        }
        ++__i;
      }
      __live = false;
    }

  public:
    using value_type = __T;
    using reference = micron::ranges::range_reference_t<V>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    iterator(__ui __ii, __us __ee, usize __hint) : __i(__ii), __e(__ee)
    {
      if ( __hint ) __seen.reserve(__hint);
      __seek();
    }

    constexpr reference
    operator*() const
    {
      return *__i;
    }

    iterator &
    operator++()
    {
      ++__i;
      __seek();
      return *this;
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

  constexpr explicit nub_view(V __v) : __base(micron::move(__v)) { }

  iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), static_cast<usize>(micron::ranges::reserve_hint(__base)) };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

struct __nub_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return nub_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v) };
  }
};

[[nodiscard]] constexpr auto
nub() noexcept
{
  return __nub_fn{};
}

template<typename V, typename Eq> class nub_by_view: public micron::view_interface<nub_by_view<V, Eq>>
{
  V __base;
  [[no_unique_address]] Eq __eq;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;
  using __T = micron::ranges::range_value_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = __T;

  static constexpr size_kind __kind = degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    micron::fvector<__T> __kept{};
    bool __live = false;
    [[no_unique_address]] fn_carrier<Eq> __q{};

    constexpr bool
    __fresh(const __T &__x) const
    {
      for ( usize __k = 0; __k < __kept.size(); ++__k )
        if ( call<Eq>(__q, __kept[__k], __x) ) return false;
      return true;
    }

    constexpr void
    __seek()
    {
      while ( !(__i == __e) ) {
        if ( __fresh(static_cast<const __T &>(*__i)) ) {
          __kept.push_back(static_cast<__T>(*__i));
          __live = true;
          return;
        }
        ++__i;
      }
      __live = false;
    }

  public:
    using value_type = __T;
    using reference = micron::ranges::range_reference_t<V>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    iterator(__ui __ii, __us __ee, const Eq &__qq) : __i(__ii), __e(__ee), __q(hold<Eq>(__qq)) { __seek(); }

    constexpr reference
    operator*() const
    {
      return *__i;
    }

    iterator &
    operator++()
    {
      ++__i;
      __seek();
      return *this;
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

  constexpr nub_by_view(V __v, Eq __q) : __base(micron::move(__v)), __eq(micron::move(__q)) { }

  iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base), __eq };
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

template<typename Eq> struct __nub_by_fn {
  [[no_unique_address]] Eq __q;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return nub_by_view<micron::remove_cvref_t<decltype(__v)>, Eq>{ micron::move(__v), __q };
  }
};

template<typename Eq>
[[nodiscard]] constexpr auto
nub_by(Eq &&__q)
{
  return __nub_by_fn<micron::decay_t<Eq>>{ micron::forward<Eq>(__q) };
}

};      // namespace lz
};      // namespace micron
