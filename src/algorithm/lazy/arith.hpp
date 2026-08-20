//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"
#include "transform.hpp"

#include "../../math/generic.hpp"
#include "../../sum.hpp"
#include "../fperrors.hpp"

namespace micron
{
namespace lz
{

namespace __impl
{

template<typename Y> struct __add_by {
  Y __y;

  template<typename T>
  constexpr auto
  operator()(const T &__x) const
  {
    return __x + __y;
  }
};

template<typename Y> struct __sub_by {
  Y __y;

  template<typename T>
  constexpr auto
  operator()(const T &__x) const
  {
    return __x - __y;
  }
};

template<typename Y> struct __mul_by {
  Y __y;

  template<typename T>
  constexpr auto
  operator()(const T &__x) const
  {
    return __x * __y;
  }
};

template<typename Y> struct __div_by {
  Y __y;

  template<typename T>
  constexpr auto
  operator()(const T &__x) const
  {
    return __x / __y;
  }
};

template<typename Y> struct __pow_by {
  Y __y;

  template<typename T>
  constexpr auto
  operator()(const T &__x) const
  {
    if constexpr ( micron::is_floating_point_v<T> || micron::is_floating_point_v<Y> )
      return math::powerf(__x, __y);
    else
      return math::power(__x, __y);
  }
};

struct __negate_one {
  template<typename T>
  constexpr T
  operator()(const T &__x) const
  {
    return -__x;
  }
};

struct __abs_one {
  template<typename T>
  constexpr T
  operator()(const T &__x) const
  {
    return __x < T{} ? static_cast<T>(-__x) : __x;
  }
};

template<typename T> struct __clamp_one {
  T __lo;
  T __hi;

  constexpr T
  operator()(const T &__x) const
  {
    return __x < __lo ? __lo : (__hi < __x ? __hi : __x);
  }
};

};      // namespace __impl

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
add(Y __y)
{
  return fmap(__impl::__add_by<Y>{ __y });
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
subtract(Y __y)
{
  return fmap(__impl::__sub_by<Y>{ __y });
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
multiply(Y __y)
{
  return fmap(__impl::__mul_by<Y>{ __y });
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
divide(Y __y)
{
  return fmap(__impl::__div_by<Y>{ __y });
}

template<typename Y>
[[nodiscard]] constexpr auto
pow(Y __y)
{
  return fmap(__impl::__pow_by<Y>{ __y });
}

template<typename R, typename Y>
  requires micron::is_arithmetic_v<Y> && micron::ranges::range<micron::remove_reference_t<R>>
[[nodiscard]] constexpr auto
add(R &&__r, Y __y)
{
  return add(__y)(micron::forward<R>(__r));
}

template<typename R, typename Y>
  requires micron::is_arithmetic_v<Y> && micron::ranges::range<micron::remove_reference_t<R>>
[[nodiscard]] constexpr auto
subtract(R &&__r, Y __y)
{
  return subtract(__y)(micron::forward<R>(__r));
}

template<typename R, typename Y>
  requires micron::is_arithmetic_v<Y> && micron::ranges::range<micron::remove_reference_t<R>>
[[nodiscard]] constexpr auto
multiply(R &&__r, Y __y)
{
  return multiply(__y)(micron::forward<R>(__r));
}

template<typename R, typename Y>
  requires micron::is_arithmetic_v<Y> && micron::ranges::range<micron::remove_reference_t<R>>
[[nodiscard]] constexpr auto
divide(R &&__r, Y __y)
{
  return divide(__y)(micron::forward<R>(__r));
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
add_c(Y __y)
{
  return add(__y);
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
subtract_c(Y __y)
{
  return subtract(__y);
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
multiply_c(Y __y)
{
  return multiply(__y);
}

template<typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
divide_c(Y __y)
{
  return divide(__y);
}

template<typename Y>
[[nodiscard]] constexpr auto
pow_c(Y __y)
{
  return pow(__y);
}

[[nodiscard]] constexpr auto
negate()
{
  return fmap(__impl::__negate_one{});
}

[[nodiscard]] constexpr auto
abs()
{
  return fmap(__impl::__abs_one{});
}

template<typename T>
[[nodiscard]] constexpr auto
clamp_each(T __lo, T __hi)
{
  return fmap(__impl::__clamp_one<T>{ __lo, __hi });
}

template<typename C, typename Y> struct __safe_divide_fn {
  Y __y;

  template<typename R>
  constexpr micron::option<C, fp::division_by_zero_error>
  operator()(R &&__r) const
  {
    using O = micron::option<C, fp::division_by_zero_error>;
    if ( __y == Y{} ) return O{ fp::division_by_zero_error{} };

    auto __v = __as_view(micron::forward<R>(__r));
    C __out;
    if constexpr ( requires(C &__c, usize __n) { __c.reserve(__n); } ) {
      const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
      if ( __hint ) __out.reserve(__hint);
    }
    __impl::__each(__v, [&](auto &&__x) { __out.push_back(static_cast<typename C::value_type>(__x / __y)); });
    return O{ micron::move(__out) };
  }
};

template<typename C, typename Y>
  requires micron::is_arithmetic_v<Y>
[[nodiscard]] constexpr auto
safe_divide(Y __y)
{
  return __safe_divide_fn<C, Y>{ __y };
}

template<typename V, typename W, typename F> class zip_arith_view: public micron::view_interface<zip_arith_view<V, W, F>>
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
  using value_type = micron::remove_cvref_t<
      micron::invoke_result_t<const F &, micron::ranges::range_reference_t<V>, micron::ranges::range_reference_t<W>>>;

  static constexpr size_kind __kind
      = (kind_of<V> == size_kind::exact && kind_of<W> == size_kind::exact) ? size_kind::exact : size_kind::bounded;
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
    using value_type = zip_arith_view::value_type;
    using reference = micron::invoke_result_t<const F &, micron::ranges::range_reference_t<V>, micron::ranges::range_reference_t<W>>;
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

  constexpr zip_arith_view(V __v, W __w, F __f) : __a(micron::move(__v)), __b(micron::move(__w)), __fn(micron::move(__f)) { }

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

namespace __impl
{

struct __zadd {
  template<typename A, typename B>
  constexpr auto
  operator()(const A &__a, const B &__b) const
  {
    return __a + __b;
  }
};

struct __zsub {
  template<typename A, typename B>
  constexpr auto
  operator()(const A &__a, const B &__b) const
  {
    return __a - __b;
  }
};

struct __zmul {
  template<typename A, typename B>
  constexpr auto
  operator()(const A &__a, const B &__b) const
  {
    return __a * __b;
  }
};

struct __zdiv {
  template<typename A, typename B>
  constexpr auto
  operator()(const A &__a, const B &__b) const
  {
    return __a / __b;
  }
};

};      // namespace __impl

template<typename W, typename F> struct __zip_arith_fn {
  W __w;
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return zip_arith_view<micron::remove_cvref_t<decltype(__v)>, W, F>{ micron::move(__v), __w, __f };
  }
};

template<typename Other>
[[nodiscard]] constexpr auto
add_zip(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_arith_fn<decltype(__w), __impl::__zadd>{ micron::move(__w), {} };
}

template<typename Other>
[[nodiscard]] constexpr auto
subtract_zip(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_arith_fn<decltype(__w), __impl::__zsub>{ micron::move(__w), {} };
}

template<typename Other>
[[nodiscard]] constexpr auto
multiply_zip(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_arith_fn<decltype(__w), __impl::__zmul>{ micron::move(__w), {} };
}

template<typename Other>
[[nodiscard]] constexpr auto
divide_zip(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_arith_fn<decltype(__w), __impl::__zdiv>{ micron::move(__w), {} };
}

template<typename C, typename W> struct __safe_divide_zip_fn {
  W __w;

  template<typename R>
  constexpr micron::option<C, fp::division_by_zero_error>
  operator()(R &&__r) const
  {
    using O = micron::option<C, fp::division_by_zero_error>;
    using E = typename C::value_type;

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
    for ( ; !(__ia == __ea) && !(__ib == __eb); ++__ia, ++__ib ) {
      if ( *__ib == micron::ranges::range_value_t<W>{} ) return O{ fp::division_by_zero_error{} };
      __out.push_back(static_cast<E>(*__ia / *__ib));
    }
    return O{ micron::move(__out) };
  }
};

template<typename C, typename Other>
[[nodiscard]] constexpr auto
safe_divide_zip(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __safe_divide_zip_fn<C, decltype(__w)>{ micron::move(__w) };
}

template<typename A, typename B> struct __div_each {
  using R = decltype(micron::declval<A>() / micron::declval<B>());

  constexpr micron::option<R, fp::division_by_zero_error>
  operator()(const A &__a, const B &__b) const
  {
    if ( __b == B{} ) return micron::option<R, fp::division_by_zero_error>{ fp::division_by_zero_error{} };
    return micron::option<R, fp::division_by_zero_error>{ static_cast<R>(__a / __b) };
  }
};

template<typename A, typename B, typename Other>
[[nodiscard]] constexpr auto
divide_zip_each(Other &&__o)
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __zip_arith_fn<decltype(__w), __div_each<A, B>>{ micron::move(__w), {} };
}

};      // namespace lz
};      // namespace micron
