//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../../sum.hpp"
#include "../../tuple.hpp"
#include "../__scan.hpp"
#include "../fperrors.hpp"

namespace micron
{
namespace lz
{

template<typename P> struct __all_of_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr bool
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return __impl::__each_while(__v, [&](auto &&__x) { return static_cast<bool>(__p(micron::forward<decltype(__x)>(__x))); });
  }
};

template<typename P> struct __any_of_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr bool
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));

    return !__impl::__each_while(__v, [&](auto &&__x) { return !static_cast<bool>(__p(micron::forward<decltype(__x)>(__x))); });
  }
};

template<typename P>
[[nodiscard]] constexpr auto
all_of(P &&__p)
{
  return __all_of_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename P>
[[nodiscard]] constexpr auto
any_of(P &&__p)
{
  return __any_of_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename P> struct __none_of_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr bool
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return __impl::__each_while(__v, [&](auto &&__x) { return !static_cast<bool>(__p(micron::forward<decltype(__x)>(__x))); });
  }
};

template<typename P>
[[nodiscard]] constexpr auto
none_of(P &&__p)
{
  return __none_of_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename P>
[[nodiscard]] constexpr auto
all_of_c(P &&__p)
{
  return all_of(micron::forward<P>(__p));
}

template<typename P>
[[nodiscard]] constexpr auto
any_of_c(P &&__p)
{
  return any_of(micron::forward<P>(__p));
}

template<typename P>
[[nodiscard]] constexpr auto
none_of_c(P &&__p)
{
  return none_of(micron::forward<P>(__p));
}

template<typename P> struct __find_first_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using O = micron::option<E, fp::empty_container_error>;

    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    for ( ; !(__i == __e); ++__i ) {

      auto &&__x = *__i;
      if ( __p(__x) ) return O{ static_cast<E>(__x) };
    }
    return O{ fp::empty_container_error{} };
  }
};

template<typename P>
[[nodiscard]] constexpr auto
find_first(P &&__p)
{
  return __find_first_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename P> struct __find_index_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr micron::option<usize, fp::empty_container_error>
  operator()(R &&__r) const
  {
    using O = micron::option<usize, fp::empty_container_error>;
    auto __v = __as_view(micron::forward<R>(__r));
    usize __k = 0;
    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    for ( ; !(__i == __e); ++__i, ++__k )
      if ( __p(*__i) ) return O{ __k };
    return O{ fp::empty_container_error{} };
  }
};

template<typename P>
[[nodiscard]] constexpr auto
find_index(P &&__p)
{
  return __find_index_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename P> struct __find_last_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using O = micron::option<E, fp::empty_container_error>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::find_last: this pipeline never terminates");

    if constexpr ( reversible<B> ) {
      auto __b = micron::ranges::begin(__v);
      auto __i = micron::ranges::end(__v);
      while ( !(__i == __b) ) {
        --__i;
        auto &&__x = *__i;
        if ( __p(__x) ) return O{ static_cast<E>(__x) };
      }
      return O{ fp::empty_container_error{} };
    } else {
      O __hit{ fp::empty_container_error{} };
      __impl::__each(__v, [&](auto &&__x) {
        if ( __p(__x) ) __hit = O{ static_cast<E>(__x) };
      });
      return __hit;
    }
  }
};

template<typename P>
[[nodiscard]] constexpr auto
find_last(P &&__p)
{
  return __find_last_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename T> struct __elem_fn {
  T __val;

  template<typename R>
  constexpr bool
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;

    if constexpr ( flat_exact<B> && micron::__impl::lane_scannable<E> && micron::is_convertible_v<T, E> ) {
      const usize __n = static_cast<usize>(__v.__size_exact());
      return micron::__impl::scan_find(micron::ranges::begin(__v), __n, static_cast<E>(__val)) != __n;
    } else {
      return !__impl::__each_while(__v, [&](auto &&__x) { return !(__x == __val); });
    }
  }
};

template<typename T>
[[nodiscard]] constexpr auto
elem(T __val)
{
  return __elem_fn<T>{ __val };
}

template<typename T> struct __find_of_fn {
  T __val;

  template<typename R>
  constexpr micron::option<usize, fp::empty_container_error>
  operator()(R &&__r) const
  {
    using O = micron::option<usize, fp::empty_container_error>;
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;

    if constexpr ( flat_exact<B> && micron::__impl::lane_scannable<E> && micron::is_convertible_v<T, E> ) {
      const usize __n = static_cast<usize>(__v.__size_exact());
      const usize __i = micron::__impl::scan_find(micron::ranges::begin(__v), __n, static_cast<E>(__val));
      return __i == __n ? O{ fp::empty_container_error{} } : O{ __i };
    } else {
      usize __k = 0;
      auto __i = micron::ranges::begin(__v);
      const auto __e = micron::ranges::end(__v);
      for ( ; !(__i == __e); ++__i, ++__k )
        if ( *__i == __val ) return O{ __k };
      return O{ fp::empty_container_error{} };
    }
  }
};

template<typename T>
[[nodiscard]] constexpr auto
find_of(T __val)
{
  return __find_of_fn<T>{ __val };
}

struct __at_fn {
  usize __n;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using O = micron::option<E, fp::index_out_of_bounds_error>;

    usize __k = 0;
    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    for ( ; !(__i == __e); ++__i, ++__k )
      if ( __k == __n ) return O{ static_cast<E>(*__i) };
    return O{ fp::index_out_of_bounds_error{} };
  }
};

[[nodiscard]] constexpr auto
at(usize __n) noexcept
{
  return __at_fn{ __n };
}

[[nodiscard]] constexpr auto
nth(usize __n) noexcept
{
  return __at_fn{ __n };
}

struct __head_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using O = micron::option<E, fp::empty_container_error>;

    auto __i = micron::ranges::begin(__v);
    if ( __i == micron::ranges::end(__v) ) return O{ fp::empty_container_error{} };
    return O{ static_cast<E>(*__i) };
  }
};

[[nodiscard]] constexpr auto
head() noexcept
{
  return __head_fn{};
}

struct __last_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using O = micron::option<E, fp::empty_container_error>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::last: this pipeline never terminates");

    if constexpr ( reversible<B> ) {
      auto __b = micron::ranges::begin(__v);
      auto __i = micron::ranges::end(__v);
      if ( __i == __b ) return O{ fp::empty_container_error{} };
      --__i;
      return O{ static_cast<E>(*__i) };
    } else {
      O __seen{ fp::empty_container_error{} };
      __impl::__each(__v, [&](auto &&__x) { __seen = O{ static_cast<E>(__x) }; });
      return __seen;
    }
  }
};

[[nodiscard]] constexpr auto
last() noexcept
{
  return __last_fn{};
}

template<typename C, bool Break, typename P, typename V>
constexpr micron::tuple<C, C>
__split_at_into(V &&__v, const P &__p)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::span_at/sbreak: this pipeline never terminates");

  C __a, __b;
  const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
  if constexpr ( requires(C &__c, usize __k) { __c.reserve(__k); } )
    if ( __hint ) {
      __a.reserve(__hint);
      __b.reserve(__hint);
    }

  bool __past = false;
  __impl::__each(__v, [&](auto &&__x) {
    if ( !__past ) {
      const bool __hit = static_cast<bool>(__p(__x));
      if ( Break ? __hit : !__hit )
        __past = true;
      else {
        __a.push_back(static_cast<typename C::value_type>(__x));
        return;
      }
    }
    __b.push_back(static_cast<typename C::value_type>(__x));
  });
  return micron::make_tuple(micron::move(__a), micron::move(__b));
}

template<typename C, typename P> struct __span_at_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr micron::tuple<C, C>
  operator()(R &&__r) const
  {
    return __split_at_into<C, false>(__as_view(micron::forward<R>(__r)), __p);
  }
};

template<typename C, typename P> struct __sbreak_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr micron::tuple<C, C>
  operator()(R &&__r) const
  {
    return __split_at_into<C, true>(__as_view(micron::forward<R>(__r)), __p);
  }
};

template<typename C, typename P>
[[nodiscard]] constexpr auto
span_at(P &&__p)
{
  return __span_at_fn<C, micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename C, typename P>
[[nodiscard]] constexpr auto
sbreak(P &&__p)
{
  return __sbreak_fn<C, micron::decay_t<P>>{ micron::forward<P>(__p) };
}

};      // namespace lz
};      // namespace micron
