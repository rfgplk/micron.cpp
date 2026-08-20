//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// take / drop / take_while / drop_while

namespace micron
{
namespace lz
{
template<typename V> class take_view: public micron::view_interface<take_view<V>>
{
  V __base;
  usize __n;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

  static constexpr bool __collapses = (kind_of<V> == size_kind::exact) && micron::random_access_iterator<__ui>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = (kind_of<V> == size_kind::exact) ? size_kind::exact : size_kind::bounded;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class counting_it
  {

    mutable __ui __i{};
    __us __e{};
    usize __left = 0;

  public:
    using value_type = micron::ranges::range_value_t<V>;
    using reference = micron::ranges::range_reference_t<V>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr counting_it() = default;

    constexpr counting_it(__ui __ii, __us __ee, usize __l) : __i(__ii), __e(__ee), __left(__l) { }

    constexpr reference
    operator*() const
    {
      return *__i;
    }

    constexpr counting_it &
    operator++()
    {

      --__left;
      if ( __left ) ++__i;
      return *this;
    }

    constexpr counting_it
    operator++(int)
    {
      counting_it __t = *this;
      ++*this;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const
    {
      return __left == 0 || __i == __e;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  using iterator = micron::conditional_t<__collapses, __ui, counting_it>;

  constexpr take_view(V __v, usize __cnt) : __base(micron::move(__v)), __n(__cnt) { }

  constexpr usize
  __clamped() const
  {
    if constexpr ( kind_of<V> == size_kind::exact ) {
      const usize __s = static_cast<usize>(micron::ranges::size(__base));
      return __n < __s ? __n : __s;
    } else {
      return __n;
    }
  }

  constexpr iterator
  begin() const
  {
    if constexpr ( __collapses )
      return micron::ranges::begin(__base);
    else
      return counting_it{ micron::ranges::begin(__base), micron::ranges::end(__base), __n };
  }

  constexpr auto
  end() const
  {
    if constexpr ( __collapses )
      return micron::ranges::begin(__base) + static_cast<micron::iter_diff_t<__ui>>(__clamped());
    else
      return sentinel{};
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    return __clamped();
  }

  constexpr usize
  reserve_hint() const
  {
    return __clamped();
  }
};

struct __take_fn {
  usize __n;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return take_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v), __n };
  }
};

[[nodiscard]] constexpr auto
take(usize __n) noexcept
{
  return __take_fn{ __n };
}

template<typename V> class drop_view: public micron::view_interface<drop_view<V>>
{
  V __base;
  usize __n;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

  static constexpr bool __jumps = (kind_of<V> == size_kind::exact) && micron::random_access_iterator<__ui>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = kind_of<V>;
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  constexpr drop_view(V __v, usize __cnt) : __base(micron::move(__v)), __n(__cnt) { }

  constexpr auto
  begin() const
  {
    if constexpr ( __jumps ) {
      const usize __s = static_cast<usize>(micron::ranges::size(__base));
      const usize __k = __n < __s ? __n : __s;
      return micron::ranges::begin(__base) + static_cast<micron::iter_diff_t<__ui>>(__k);
    } else {

      auto __i = micron::ranges::begin(__base);
      const auto __e = micron::ranges::end(__base);
      for ( usize __k = 0; __k < __n && !(__i == __e); ++__k ) ++__i;
      return __i;
    }
  }

  constexpr auto
  end() const
  {
    return micron::ranges::end(__base);
  }

  constexpr usize
  __size_exact() const
    requires(__kind == size_kind::exact)
  {
    const usize __s = static_cast<usize>(micron::ranges::size(__base));
    return __n < __s ? __s - __n : 0u;
  }

  constexpr usize
  reserve_hint() const
  {
    if constexpr ( __kind == size_kind::exact )
      return __size_exact();
    else
      return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

struct __drop_fn {
  usize __n;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return drop_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v), __n };
  }
};

[[nodiscard]] constexpr auto
drop(usize __n) noexcept
{
  return __drop_fn{ __n };
}

template<typename V, typename P> class take_while_view: public micron::view_interface<take_while_view<V, P>>
{
  V __base;
  [[no_unique_address]] P __pred;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = bound_endless(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    [[no_unique_address]] fn_carrier<P> __p{};

  public:
    using value_type = micron::ranges::range_value_t<V>;
    using reference = micron::ranges::range_reference_t<V>;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee, const P &__pp) : __i(__ii), __e(__ee), __p(hold<P>(__pp)) { }

    constexpr reference
    operator*() const
    {
      return *__i;
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

    constexpr bool
    operator==(const sentinel &) const
    {
      return __i == __e || !call<P>(__p, *__i);
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr take_while_view(V __v, P __p) : __base(micron::move(__v)), __pred(micron::move(__p)) { }

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

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

template<typename P> struct __take_while_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return take_while_view<micron::remove_cvref_t<decltype(__v)>, P>{ micron::move(__v), __p };
  }
};

template<typename P>
[[nodiscard]] constexpr auto
take_while(P &&__p)
{
  return __take_while_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename V, typename P> class drop_while_view: public micron::view_interface<drop_while_view<V, P>>
{
  V __base;
  [[no_unique_address]] P __pred;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  constexpr drop_while_view(V __v, P __p) : __base(micron::move(__v)), __pred(micron::move(__p)) { }

  constexpr auto
  begin() const
  {
    auto __i = micron::ranges::begin(__base);
    const auto __e = micron::ranges::end(__base);
    while ( !(__i == __e) && __pred(*__i) ) ++__i;
    return __i;
  }

  constexpr auto
  end() const
  {
    return micron::ranges::end(__base);
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

template<typename P> struct __drop_while_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return drop_while_view<micron::remove_cvref_t<decltype(__v)>, P>{ micron::move(__v), __p };
  }
};

template<typename P>
[[nodiscard]] constexpr auto
drop_while(P &&__p)
{
  return __drop_while_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

[[nodiscard]] constexpr auto
tail() noexcept
{
  return drop(1);
}

template<typename V> class init_view: public micron::view_interface<init_view<V>>
{
  V __base;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::ranges::range_value_t<V>;

  static constexpr size_kind __kind = (kind_of<V> == size_kind::exact) ? size_kind::exact : degrade(kind_of<V>);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    micron::ranges::range_value_t<V> __held{};
    bool __live = false;

  public:
    using value_type = micron::ranges::range_value_t<V>;
    using reference = const value_type &;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee) : __i(__ii), __e(__ee)
    {
      if ( !(__i == __e) ) {
        __held = *__i;
        ++__i;
        __live = !(__i == __e);
      }
    }

    constexpr reference
    operator*() const
    {
      return __held;
    }

    constexpr iterator &
    operator++()
    {
      __held = *__i;
      ++__i;
      __live = !(__i == __e);
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

  constexpr explicit init_view(V __v) : __base(micron::move(__v)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__base), micron::ranges::end(__base) };
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
    return __n ? __n - 1 : 0u;
  }

  constexpr usize
  reserve_hint() const
  {
    const usize __n = static_cast<usize>(micron::ranges::reserve_hint(__base));
    return __n ? __n - 1 : 0u;
  }
};

struct __init_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return init_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v) };
  }
};

[[nodiscard]] constexpr auto
init() noexcept
{
  return __init_fn{};
}

};      // namespace lz
};      // namespace micron
