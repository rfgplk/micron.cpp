//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../../sum.hpp"
#include "../../tuple.hpp"
#include "../fperrors.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zip / zip_with / enumerate / ap

namespace micron
{
namespace lz
{

template<typename V, typename W, typename F> class zip_with_view: public micron::view_interface<zip_with_view<V, W, F>>
{
  V __a;
  W __b;
  [[no_unique_address]] F __fn;

  using __ai = micron::ranges::iterator_t<V>;
  using __as = micron::ranges::sentinel_t<V>;
  using __bi = micron::ranges::iterator_t<W>;
  using __bs = micron::ranges::sentinel_t<W>;

public:
  using __lazy_view_tag = void;
  using __fnref = micron::invoke_result_t<const F &, micron::ranges::range_reference_t<V>, micron::ranges::range_reference_t<W>>;
  using value_type = micron::remove_cvref_t<__fnref>;

  static constexpr size_kind __kind
      = (kind_of<V> == size_kind::exact && kind_of<W> == size_kind::exact)
            ? size_kind::exact
            : ((kind_of<V> == size_kind::endless && kind_of<W> == size_kind::endless) ? size_kind::endless : size_kind::bounded);
  static constexpr usize __static_size = no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ai __ia{};
    __as __ea{};
    mutable __bi __ib{};
    __bs __eb{};
    [[no_unique_address]] fn_carrier<F> __f{};

  public:
    using value_type = zip_with_view::value_type;
    using reference = zip_with_view::__fnref;
    using difference_type = micron::iter_diff_t<__ai>;

    constexpr iterator() = default;

    constexpr iterator(__ai __a0, __as __a1, __bi __b0, __bs __b1, const F &__ff)
        : __ia(__a0), __ea(__a1), __ib(__b0), __eb(__b1), __f(hold<F>(__ff))
    {
    }

    constexpr reference
    operator*() const
    {
      return call<F>(__f, *__ia, *__ib);
    }

    constexpr iterator &
    operator++()
    {
      ++__ia;
      ++__ib;
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
      return __ia == __ea || __ib == __eb;
    }

    constexpr bool
    operator!=(const sentinel &__s) const
    {
      return !(*this == __s);
    }
  };

  constexpr zip_with_view(V __v, W __w, F __f) : __a(micron::move(__v)), __b(micron::move(__w)), __fn(micron::move(__f)) { }

  constexpr iterator
  begin() const
  {
    return iterator{ micron::ranges::begin(__a), micron::ranges::end(__a), micron::ranges::begin(__b), micron::ranges::end(__b), __fn };
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
    const usize __na = static_cast<usize>(micron::ranges::size(__a));
    const usize __nb = static_cast<usize>(micron::ranges::size(__b));
    return __na < __nb ? __na : __nb;
  }

  constexpr usize
  reserve_hint() const
  {
    const usize __na = static_cast<usize>(micron::ranges::reserve_hint(__a));
    const usize __nb = static_cast<usize>(micron::ranges::reserve_hint(__b));
    if ( __na == 0 ) return __nb;
    if ( __nb == 0 ) return __na;
    return __na < __nb ? __na : __nb;
  }
};

template<typename W, typename F> struct __zip_with_fn {
  W __w;
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return zip_with_view<micron::remove_cvref_t<decltype(__v)>, W, F>{ micron::move(__v), __w, __f };
  }
};

template<typename Other, typename F>
[[nodiscard]] constexpr auto
zip_with(Other &&__o, F &&__f)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_with_fn<decltype(__w), micron::decay_t<F>>{ micron::move(__w), micron::forward<F>(__f) };
}

template<typename Other, typename F>
[[nodiscard]] constexpr auto
zip_with_trunc(Other &&__o, F &&__f)
{
  return zip_with(micron::forward<Other>(__o), micron::forward<F>(__f));
}

namespace __impl
{

struct __make_pair_tuple {
  template<typename A, typename B>
  constexpr auto
  operator()(A &&__a, B &&__b) const
  {
    return micron::make_tuple(micron::forward<A>(__a), micron::forward<B>(__b));
  }
};

};      // namespace __impl

template<typename Other>
[[nodiscard]] constexpr auto
zip(Other &&__o)
{
  return zip_with(micron::forward<Other>(__o), __impl::__make_pair_tuple{});
}

template<typename C, typename W, typename F> struct __zip_strict_fn {
  W __w;
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr micron::option<C, fp::bad_zip_error>
  operator()(R &&__r) const
  {
    using O = micron::option<C, fp::bad_zip_error>;
    auto __v = __as_view(micron::forward<R>(__r));

    C __out;
    if constexpr ( requires(C &__c, usize __n) { __c.reserve(__n); } ) {
      const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
      if ( __hint ) __out.reserve(__hint);
    }

    auto __ia = micron::ranges::begin(__v);
    const auto __ea = micron::ranges::end(__v);
    auto __ib = micron::ranges::begin(__w);
    const auto __eb = micron::ranges::end(__w);
    for ( ; !(__ia == __ea) && !(__ib == __eb); ++__ia, ++__ib ) __out.push_back(static_cast<typename C::value_type>(__f(*__ia, *__ib)));
    if ( !(__ia == __ea) || !(__ib == __eb) ) return O{ fp::bad_zip_error{} };
    return O{ micron::move(__out) };
  }
};

template<typename C, typename Other, typename F>
[[nodiscard]] constexpr auto
zip_strict(Other &&__o, F &&__f)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_strict_fn<C, decltype(__w), micron::decay_t<F>>{ micron::move(__w), micron::forward<F>(__f) };
}

template<typename V> class enumerate_view: public micron::view_interface<enumerate_view<V>>
{
  V __base;

  using __ui = micron::ranges::iterator_t<V>;
  using __us = micron::ranges::sentinel_t<V>;

public:
  using __lazy_view_tag = void;
  using value_type = micron::tuple<usize, micron::ranges::range_value_t<V>>;

  static constexpr size_kind __kind = kind_of<V>;
  static constexpr usize __static_size = static_size_of<V>;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    mutable __ui __i{};
    __us __e{};
    usize __k = 0;

  public:
    using value_type = enumerate_view::value_type;
    using reference = value_type;
    using difference_type = micron::iter_diff_t<__ui>;

    constexpr iterator() = default;

    constexpr iterator(__ui __ii, __us __ee) : __i(__ii), __e(__ee) { }

    constexpr value_type
    operator*() const
    {
      return micron::make_tuple(__k, static_cast<micron::ranges::range_value_t<V>>(*__i));
    }

    constexpr iterator &
    operator++()
    {
      ++__i;
      ++__k;
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

  constexpr explicit enumerate_view(V __v) : __base(micron::move(__v)) { }

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
    return static_cast<usize>(micron::ranges::size(__base));
  }

  constexpr usize
  reserve_hint() const
  {
    return static_cast<usize>(micron::ranges::reserve_hint(__base));
  }
};

struct __enumerate_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return enumerate_view<micron::remove_cvref_t<decltype(__v)>>{ micron::move(__v) };
  }
};

[[nodiscard]] constexpr auto
enumerate() noexcept
{
  return __enumerate_fn{};
}

namespace __impl
{

struct __apply_one {
  template<typename Fn, typename X>
  constexpr auto
  operator()(Fn &&__fn, X &&__x) const
  {
    return __fn(micron::forward<X>(__x));
  }
};

};      // namespace __impl

template<typename Vals>
[[nodiscard]] constexpr auto
ap(Vals &&__vals)
{
  return zip_with(micron::forward<Vals>(__vals), __impl::__apply_one{});
}

};      // namespace lz
};      // namespace micron
