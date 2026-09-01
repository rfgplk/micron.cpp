//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"

#include "../../sum.hpp"
#include "../../vector/fvector.hpp"
#include "../__scan.hpp"
#include "../algorithm.hpp"
#include "../fperrors.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reductions

namespace micron
{
namespace lz
{
template<typename V>
concept indexable_exact = (kind_of<V> == size_kind::exact) && micron::random_access_iterator<micron::ranges::iterator_t<V>>
                          && requires(const V &__v) { __v.__size_exact(); };

namespace __impl
{

template<typename It> struct __idx_adapter {
  It __b;

  [[gnu::always_inline]] constexpr decltype(auto)
  operator[](usize __i) const
  {
    return __b[static_cast<micron::iter_diff_t<It>>(__i)];
  }
};

};      // namespace __impl

template<typename A, typename V, typename F>
constexpr A
fold_into(V &&__v, A __init, const F &__f)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless,
                "micron::lz::fold: this pipeline never terminates -- put a take(n) or take_while(p) before it");
  __impl::__each(__v, [&](auto &&__x) { __init = __f(micron::move(__init), micron::forward<decltype(__x)>(__x)); });
  return __init;
}

template<typename A, typename F> struct __fold_fn {
  A __init;
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr A
  operator()(R &&__r) const
  {
    return fold_into<A>(__as_view(micron::forward<R>(__r)), __init, __f);
  }
};

template<typename A, typename F>
[[nodiscard]] constexpr auto
fold(A __init, F &&__f)
{
  return __fold_fn<A, micron::decay_t<F>>{ micron::move(__init), micron::forward<F>(__f) };
}

template<typename A, typename F>
[[nodiscard]] constexpr auto
foldl(A __init, F &&__f)
{
  return fold(micron::move(__init), micron::forward<F>(__f));
}

template<typename A, typename V, typename F>
constexpr A
foldr_into(V &&__v, const F &__f, A __init)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::foldr: this pipeline never terminates, and foldr starts at the END");

  if constexpr ( reversible<B> ) {
    auto __b = micron::ranges::begin(__v);
    auto __i = micron::ranges::end(__v);
    while ( !(__i == __b) ) {
      --__i;
      __init = __f(*__i, micron::move(__init));
    }
    return __init;
  } else {
    micron::fvector<micron::ranges::range_value_t<B>> __buf;
    const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
    if ( __hint ) __buf.reserve(__hint);
    __impl::__each(__v, [&](auto &&__x) { __buf.push_back(micron::forward<decltype(__x)>(__x)); });
    for ( usize __k = __buf.size(); __k-- > 0; ) __init = __f(__buf[__k], micron::move(__init));
    return __init;
  }
}

template<typename A, typename F> struct __foldr_fn {
  [[no_unique_address]] F __f;
  A __init;

  template<typename R>
  constexpr A
  operator()(R &&__r) const
  {
    return foldr_into<A>(__as_view(micron::forward<R>(__r)), __f, __init);
  }
};

template<typename F, typename A>
[[nodiscard]] constexpr auto
foldr(F &&__f, A __init)
{
  return __foldr_fn<A, micron::decay_t<F>>{ micron::forward<F>(__f), micron::move(__init) };
}

template<typename F> struct __for_each_fn {
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr void
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless,
                  "micron::lz::for_each: this pipeline never terminates -- put a take(n) or take_while(p) before it");
    __impl::__each(__v, [&](auto &&__x) { __f(micron::forward<decltype(__x)>(__x)); });
  }
};

template<typename F>
constexpr auto
for_each(F &&__f)
{
  return __for_each_fn<micron::decay_t<F>>{ micron::forward<F>(__f) };
}

template<typename V>
constexpr usize
count_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::count: this pipeline never terminates");

  if constexpr ( kind_of<B> == size_kind::exact && requires(const B &__b) { __b.__size_exact(); } ) {
    return static_cast<usize>(__v.__size_exact());
  } else {

    usize __n = 0;
    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    for ( ; !(__i == __e); ++__i ) ++__n;
    return __n;
  }
}

struct __count_fn {
  template<typename R>
  constexpr usize
  operator()(R &&__r) const
  {
    return count_into(__as_view(micron::forward<R>(__r)));
  }
};

[[nodiscard]] constexpr auto
count() noexcept
{
  return __count_fn{};
}

template<typename P> struct __count_if_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr usize
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::count_if: this pipeline never terminates");
    usize __n = 0;
    __impl::__each(__v, [&](auto &&__x) {
      if ( __p(micron::forward<decltype(__x)>(__x)) ) ++__n;
    });
    return __n;
  }
};

template<typename P>
[[nodiscard]] constexpr auto
count_if(P &&__p)
{
  return __count_if_fn<micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename T> struct __count_of_fn {
  T __val;

  template<typename R>
  constexpr usize
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::count_of: this pipeline never terminates");

    if constexpr ( flat_exact<B> && micron::__impl::lane_scannable<E> && micron::is_convertible_v<T, E> ) {
      return static_cast<usize>(
          micron::__impl::scan_count(micron::ranges::begin(__v), static_cast<usize>(__v.__size_exact()), static_cast<E>(__val)));
    } else {
      usize __n = 0;
      __impl::__each(__v, [&](auto &&__x) {
        if ( __x == __val ) ++__n;
      });
      return __n;
    }
  }
};

template<typename T>
[[nodiscard]] constexpr auto
count_of(T __val)
{
  return __count_of_fn<T>{ __val };
}

__micron_push_options
__micron_optimize_no_fast_math
namespace __impl
{

template<typename V>
constexpr f64
__sum_lanes_stream(const V &__v) noexcept
{
  f64 __s[4] = { 0, 0, 0, 0 };
  f64 __c[4] = { 0, 0, 0, 0 };
  f64 __stage[4];
  usize __have = 0;

  __each(__v, [&](auto &&__x) {
    __stage[__have++] = static_cast<f64>(__x);
    if ( __have == 4 ) {
      micron::__impl::__neumaier_add(__s[0], __c[0], __stage[0]);
      micron::__impl::__neumaier_add(__s[1], __c[1], __stage[1]);
      micron::__impl::__neumaier_add(__s[2], __c[2], __stage[2]);
      micron::__impl::__neumaier_add(__s[3], __c[3], __stage[3]);
      __have = 0;
    }
  });
  for ( usize __k = 0; __k < __have; ++__k ) micron::__impl::__neumaier_add(__s[0], __c[0], __stage[__k]);

  f64 __acc = 0, __comp = 0;
  for ( usize __k = 0; __k < 4; ++__k ) {
    micron::__impl::__neumaier_add(__acc, __comp, __s[__k]);
    __comp += __c[__k];
  }
  return __acc + __comp;
}

template<typename V>
constexpr f128
__sum_wide_stream(const V &__v) noexcept
{
  f128 __sm = 0;
  __each(__v, [&](auto &&__x) { __sm += static_cast<f128>(__x); });
  return __sm;
}

};      // namespace __impl

template<typename V>
constexpr f128
sum_fp_into(V &&__v) noexcept
{
  using B = micron::remove_cvref_t<V>;

  if constexpr ( flat_exact<B> ) {
    return micron::__impl::__sum_fp(micron::ranges::begin(__v), static_cast<usize>(__v.__size_exact()));
  } else if constexpr ( indexable_exact<B> ) {

    return micron::__impl::__sum_fp(__impl::__idx_adapter<micron::ranges::iterator_t<B>>{ micron::ranges::begin(__v) },
                                    static_cast<usize>(__v.__size_exact()));
  } else {
    const f64 __fast = __impl::__sum_lanes_stream(__v);
    if ( micron::__impl::__f64_finite(__fast) ) return static_cast<f128>(__fast);
    return __impl::__sum_wide_stream(__v);
  }
}

__micron_pop_options
namespace __impl
{

template<typename V>
constexpr umax_t
__sum_int_stream(const V &__v) noexcept
{
  umax_t __w = 0, __x = 0, __y = 0, __z = 0;
  auto __i = micron::ranges::begin(__v);
  const auto __e = micron::ranges::end(__v);
  for ( ;; ) {
    if ( __i == __e ) break;
    __w += static_cast<umax_t>(*__i);
    ++__i;
    if ( __i == __e ) break;
    __x += static_cast<umax_t>(*__i);
    ++__i;
    if ( __i == __e ) break;
    __y += static_cast<umax_t>(*__i);
    ++__i;
    if ( __i == __e ) break;
    __z += static_cast<umax_t>(*__i);
    ++__i;
  }
  return (__w + __x) + (__y + __z);
}

};      // namespace __impl

template<typename V>
constexpr umax_t
sum_int_into(V &&__v) noexcept
{
  using B = micron::remove_cvref_t<V>;
  if constexpr ( flat_exact<B> )
    return micron::__impl::__sum_int(micron::ranges::begin(__v), static_cast<usize>(__v.__size_exact()));
  else if constexpr ( indexable_exact<B> )
    return micron::__impl::__sum_int(__impl::__idx_adapter<micron::ranges::iterator_t<B>>{ micron::ranges::begin(__v) },
                                     static_cast<usize>(__v.__size_exact()));
  else
    return __impl::__sum_int_stream(__v);
}

struct __sum_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::sum: this pipeline never terminates");
    static_assert(micron::is_floating_point_v<E> || micron::is_integral_v<E>,
                  "micron::lz::sum: the element type is neither integral nor floating point -- use fold()");
    if constexpr ( micron::is_floating_point_v<E> )
      return sum_fp_into(__v);
    else
      return sum_int_into(__v);
  }
};

[[nodiscard]] constexpr auto
sum() noexcept
{
  return __sum_fn{};
}

template<typename E> using __sum_result_t = micron::conditional_t<micron::is_floating_point_v<E>, f128, umax_t>;

struct __safe_sum_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using S = __sum_result_t<E>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::safe_sum: this pipeline never terminates");

    if ( micron::ranges::begin(__v) == micron::ranges::end(__v) )
      return micron::option<S, fp::empty_container_error>{ fp::empty_container_error{} };
    if constexpr ( micron::is_floating_point_v<E> )
      return micron::option<S, fp::empty_container_error>{ sum_fp_into(__v) };
    else
      return micron::option<S, fp::empty_container_error>{ sum_int_into(__v) };
  }
};

[[nodiscard]] constexpr auto
safe_sum() noexcept
{
  return __safe_sum_fn{};
}

template<bool Max, typename V>
constexpr auto
__extreme_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  using E = micron::ranges::range_value_t<B>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::min/max: this pipeline never terminates");

  auto __i = micron::ranges::begin(__v);
  const auto __e = micron::ranges::end(__v);
  E __best = *__i;
  ++__i;
  for ( ; !(__i == __e); ++__i ) {
    auto &&__x = *__i;
    if constexpr ( Max ) {
      if ( __best < __x ) __best = static_cast<E>(__x);
    } else {
      if ( __x < __best ) __best = static_cast<E>(__x);
    }
  }
  return __best;
}

template<bool Max, typename V>
constexpr auto
__safe_extreme_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  using E = micron::ranges::range_value_t<B>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::safe_min/safe_max: this pipeline never terminates");

  if ( micron::ranges::begin(__v) == micron::ranges::end(__v) )
    return micron::option<E, fp::empty_container_error>{ fp::empty_container_error{} };
  return micron::option<E, fp::empty_container_error>{ __extreme_into<Max>(__v) };
}

struct __min_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return __extreme_into<false>(__as_view(micron::forward<R>(__r)));
  }
};

struct __max_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return __extreme_into<true>(__as_view(micron::forward<R>(__r)));
  }
};

struct __safe_min_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return __safe_extreme_into<false>(__as_view(micron::forward<R>(__r)));
  }
};

struct __safe_max_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return __safe_extreme_into<true>(__as_view(micron::forward<R>(__r)));
  }
};

[[nodiscard]] constexpr auto
min() noexcept
{
  return __min_fn{};
}

[[nodiscard]] constexpr auto
max() noexcept
{
  return __max_fn{};
}

[[nodiscard]] constexpr auto
safe_min() noexcept
{
  return __safe_min_fn{};
}

[[nodiscard]] constexpr auto
safe_max() noexcept
{
  return __safe_max_fn{};
}

template<typename R, typename V>
constexpr R
mean_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  using E = micron::ranges::range_value_t<B>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::mean: this pipeline never terminates");

  if constexpr ( kind_of<B> == size_kind::exact && requires(const B &__b) { __b.__size_exact(); } ) {
    const usize __n = static_cast<usize>(__v.__size_exact());
    if constexpr ( micron::is_floating_point_v<E> )
      return static_cast<R>(sum_fp_into(__v)) / static_cast<R>(__n);
    else
      return static_cast<R>(sum_int_into(__v)) / static_cast<R>(__n);
  } else {
    R __acc = R{};
    usize __n = 0;
    __impl::__each(__v, [&](auto &&__x) {
      __acc += static_cast<R>(__x);
      ++__n;
    });
    return __acc / static_cast<R>(__n);
  }
}

template<typename R> struct __mean_fn {
  template<typename Rn>
  constexpr R
  operator()(Rn &&__r) const
  {
    return mean_into<R>(__as_view(micron::forward<Rn>(__r)));
  }
};

template<typename R> struct __safe_mean_fn {
  template<typename Rn>
  constexpr micron::option<R, fp::empty_container_error>
  operator()(Rn &&__r) const
  {
    auto __v = __as_view(micron::forward<Rn>(__r));
    if ( micron::ranges::begin(__v) == micron::ranges::end(__v) )
      return micron::option<R, fp::empty_container_error>{ fp::empty_container_error{} };
    return micron::option<R, fp::empty_container_error>{ mean_into<R>(__v) };
  }
};

template<typename R = f64>
[[nodiscard]] constexpr auto
mean() noexcept
{
  return __mean_fn<R>{};
}

template<typename R = f64>
[[nodiscard]] constexpr auto
safe_mean() noexcept
{
  return __safe_mean_fn<R>{};
}

template<typename R, typename V>
constexpr R
geomean_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::geomean: this pipeline never terminates");
  R __prod = R(1);
  usize __n = 0;
  __impl::__each(__v, [&](auto &&__x) {
    __prod *= static_cast<R>(__x);
    ++__n;
  });
  return static_cast<R>(math::powerflong(__prod, static_cast<R>(R(1) / static_cast<R>(__n))));
}

template<typename R, typename V>
constexpr R
harmonicmean_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::harmonicmean: this pipeline never terminates");
  R __rec = R{};
  usize __n = 0;
  __impl::__each(__v, [&](auto &&__x) {
    __rec += (R(1) / static_cast<R>(__x));
    ++__n;
  });
  return static_cast<R>(__n) / __rec;
}

template<typename R> struct __geomean_fn {
  template<typename Rn>
  constexpr R
  operator()(Rn &&__r) const
  {
    return geomean_into<R>(__as_view(micron::forward<Rn>(__r)));
  }
};

template<typename R> struct __harmonicmean_fn {
  template<typename Rn>
  constexpr R
  operator()(Rn &&__r) const
  {
    return harmonicmean_into<R>(__as_view(micron::forward<Rn>(__r)));
  }
};

template<typename R> struct __safe_geomean_fn {
  template<typename Rn>
  constexpr micron::option<R, fp::empty_container_error>
  operator()(Rn &&__r) const
  {
    auto __v = __as_view(micron::forward<Rn>(__r));
    if ( micron::ranges::begin(__v) == micron::ranges::end(__v) )
      return micron::option<R, fp::empty_container_error>{ fp::empty_container_error{} };
    return micron::option<R, fp::empty_container_error>{ geomean_into<R>(__v) };
  }
};

template<typename R> struct __safe_harmonicmean_fn {
  template<typename Rn>
  constexpr micron::option<R, fp::empty_container_error>
  operator()(Rn &&__r) const
  {
    auto __v = __as_view(micron::forward<Rn>(__r));
    if ( micron::ranges::begin(__v) == micron::ranges::end(__v) )
      return micron::option<R, fp::empty_container_error>{ fp::empty_container_error{} };
    return micron::option<R, fp::empty_container_error>{ harmonicmean_into<R>(__v) };
  }
};

template<typename R = flong>
[[nodiscard]] constexpr auto
geomean() noexcept
{
  return __geomean_fn<R>{};
}

template<typename R = flong>
[[nodiscard]] constexpr auto
harmonicmean() noexcept
{
  return __harmonicmean_fn<R>{};
}

template<typename R = flong>
[[nodiscard]] constexpr auto
safe_geomean() noexcept
{
  return __safe_geomean_fn<R>{};
}

template<typename R = flong>
[[nodiscard]] constexpr auto
safe_harmonicmean() noexcept
{
  return __safe_harmonicmean_fn<R>{};
}

template<typename R, typename V, typename W>
constexpr R
inner_product_into(const V &__v, const W &__w, R __init)
{
  static_assert(kind_of<micron::remove_cvref_t<V>> != size_kind::endless || kind_of<micron::remove_cvref_t<W>> != size_kind::endless,
                "micron::lz::inner_product: both operands are endless");

  if constexpr ( flat_exact<micron::remove_cvref_t<V>> && flat_exact<micron::remove_cvref_t<W>> ) {
    const auto *__restrict __pa = micron::ranges::begin(__v);
    const auto *__restrict __pb = micron::ranges::begin(__w);
    const usize __na = static_cast<usize>(__v.__size_exact());
    const usize __nb = static_cast<usize>(__w.__size_exact());
    const usize __n = __na < __nb ? __na : __nb;
    R __acc = __init;
    for ( usize __i = 0; __i < __n; ++__i ) __acc = __acc + static_cast<R>(__pa[__i]) * static_cast<R>(__pb[__i]);
    return __acc;
  } else {
    auto __a = micron::ranges::begin(__v);
    const auto __ea = micron::ranges::end(__v);
    auto __b = micron::ranges::begin(__w);
    const auto __eb = micron::ranges::end(__w);
    for ( ; !(__a == __ea) && !(__b == __eb); ++__a, ++__b ) __init = __init + static_cast<R>(*__a) * static_cast<R>(*__b);
    return __init;
  }
}

template<typename R, typename W> struct __inner_product_fn {
  W __w;
  R __init;

  template<typename Rn>
  constexpr R
  operator()(Rn &&__r) const
  {
    return inner_product_into<R>(__as_view(micron::forward<Rn>(__r)), __w, __init);
  }
};

template<typename R, typename W> struct __safe_inner_product_fn {
  W __w;
  R __init;

  template<typename Rn>
  constexpr micron::option<R, fp::bad_zip_error>
  operator()(Rn &&__r) const
  {
    auto __v = __as_view(micron::forward<Rn>(__r));
    auto __a = micron::ranges::begin(__v);
    const auto __ea = micron::ranges::end(__v);
    auto __b = micron::ranges::begin(__w);
    const auto __eb = micron::ranges::end(__w);
    R __acc = __init;
    for ( ; !(__a == __ea) && !(__b == __eb); ++__a, ++__b ) __acc = __acc + static_cast<R>(*__a) * static_cast<R>(*__b);

    if ( !(__a == __ea) || !(__b == __eb) ) return micron::option<R, fp::bad_zip_error>{ fp::bad_zip_error{} };
    return micron::option<R, fp::bad_zip_error>{ __acc };
  }
};

template<typename R = f64, typename Other>
[[nodiscard]] constexpr auto
inner_product(Other &&__o, R __init = R{})
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __inner_product_fn<R, decltype(__w)>{ micron::move(__w), __init };
}

template<typename R = f64, typename Other>
[[nodiscard]] constexpr auto
safe_inner_product(Other &&__o, R __init = R{})
{
  auto __w = __as_view(micron::forward<Other>(__o));
  return __safe_inner_product_fn<R, decltype(__w)>{ micron::move(__w), __init };
}

};      // namespace lz
};      // namespace micron
