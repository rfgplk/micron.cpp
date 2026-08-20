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

namespace micron
{
namespace lz
{

namespace __impl
{

template<typename C>
constexpr void
__reserve_if(C &__c, usize __n)
{
  if constexpr ( requires(C &__x, usize __k) { __x.reserve(__k); } )
    if ( __n ) __c.reserve(__n);
}

};      // namespace __impl

template<typename C, typename P, typename V>
constexpr micron::tuple<C, C>
partition_into(V &&__v, const P &__p)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::partition: this pipeline never terminates");

  C __yes, __no;
  const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
  __impl::__reserve_if(__yes, __hint);
  __impl::__reserve_if(__no, __hint);

  __impl::__each(__v, [&](auto &&__x) {
    if ( __p(__x) )
      __yes.push_back(static_cast<typename C::value_type>(__x));
    else
      __no.push_back(static_cast<typename C::value_type>(__x));
  });
  return micron::make_tuple(micron::move(__yes), micron::move(__no));
}

template<typename C, typename P> struct __partition_fn {
  [[no_unique_address]] P __p;

  template<typename R>
  constexpr micron::tuple<C, C>
  operator()(R &&__r) const
  {
    return partition_into<C>(__as_view(micron::forward<R>(__r)), __p);
  }
};

template<typename C, typename P>
[[nodiscard]] constexpr auto
partition(P &&__p)
{
  return __partition_fn<C, micron::decay_t<P>>{ micron::forward<P>(__p) };
}

template<typename CA, typename CB, typename V>
constexpr micron::tuple<CA, CB>
unzip_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::unzip: this pipeline never terminates");

  CA __as;
  CB __bs;
  const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
  __impl::__reserve_if(__as, __hint);
  __impl::__reserve_if(__bs, __hint);

  __impl::__each(__v, [&](auto &&__x) {
    __as.push_back(static_cast<typename CA::value_type>(micron::get<0>(__x)));
    __bs.push_back(static_cast<typename CB::value_type>(micron::get<1>(__x)));
  });
  return micron::make_tuple(micron::move(__as), micron::move(__bs));
}

template<typename CA, typename CB> struct __unzip_fn {
  template<typename R>
  constexpr micron::tuple<CA, CB>
  operator()(R &&__r) const
  {
    return unzip_into<CA, CB>(__as_view(micron::forward<R>(__r)));
  }
};

template<typename CA, typename CB>
[[nodiscard]] constexpr auto
unzip() noexcept
{
  return __unzip_fn<CA, CB>{};
}

template<typename C, typename F, typename V>
constexpr auto
traverse_into(V &&__v, const F &__f)
{
  using B = micron::remove_cvref_t<V>;
  using Res = micron::remove_cvref_t<micron::invoke_result_t<const F &, micron::ranges::range_reference_t<B>>>;
  using E = typename Res::second_type;
  using O = micron::option<C, E>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::traverse: this pipeline never terminates");

  C __out;
  __impl::__reserve_if(__out, static_cast<usize>(micron::ranges::reserve_hint(__v)));

  bool __failed = false;
  E __err{};
  __impl::__each_while(__v, [&](auto &&__x) {
    Res __r = __f(micron::forward<decltype(__x)>(__x));
    if ( !__r.is_first() ) {
      __err = __r.template cast<E>();
      __failed = true;
      return false;
    }
    __out.push_back(static_cast<typename C::value_type>(__r.template cast<typename Res::first_type>()));
    return true;
  });

  if ( __failed ) return O{ __err };
  return O{ micron::move(__out) };
}

template<typename C, typename F> struct __traverse_fn {
  [[no_unique_address]] F __f;

  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return traverse_into<C>(__as_view(micron::forward<R>(__r)), __f);
  }
};

template<typename C, typename F>
[[nodiscard]] constexpr auto
traverse(F &&__f)
{
  return __traverse_fn<C, micron::decay_t<F>>{ micron::forward<F>(__f) };
}

template<typename C, typename V>
constexpr auto
sequence_into(V &&__v)
{
  using B = micron::remove_cvref_t<V>;
  using Opt = micron::ranges::range_value_t<B>;
  using E = typename Opt::second_type;
  using O = micron::option<C, E>;
  static_assert(kind_of<B> != size_kind::endless, "micron::lz::sequence: this pipeline never terminates");

  C __out;
  __impl::__reserve_if(__out, static_cast<usize>(micron::ranges::reserve_hint(__v)));

  bool __failed = false;
  E __err{};
  __impl::__each_while(__v, [&](auto &&__o) {
    if ( !__o.is_first() ) {
      __err = __o.template cast<E>();
      __failed = true;
      return false;
    }
    __out.push_back(static_cast<typename C::value_type>(__o.template cast<typename Opt::first_type>()));
    return true;
  });

  if ( __failed ) return O{ __err };
  return O{ micron::move(__out) };
}

template<typename C> struct __sequence_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    return sequence_into<C>(__as_view(micron::forward<R>(__r)));
  }
};

template<typename C>
[[nodiscard]] constexpr auto
sequence() noexcept
{
  return __sequence_fn<C>{};
}

struct __sequence_check_fn {
  template<typename R>
  constexpr bool
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    return __impl::__each_while(__v, [](auto &&__o) { return __o.is_first(); });
  }
};

[[nodiscard]] constexpr auto
sequence_check() noexcept
{
  return __sequence_check_fn{};
}

template<typename C> struct __uncons_fn {
  template<typename R>
  constexpr auto
  operator()(R &&__r) const
  {
    auto __v = __as_view(micron::forward<R>(__r));
    using B = micron::remove_cvref_t<decltype(__v)>;
    using E = micron::ranges::range_value_t<B>;
    using O = micron::option<micron::tuple<E, C>, fp::empty_container_error>;
    static_assert(kind_of<B> != size_kind::endless, "micron::lz::uncons: this pipeline never terminates");

    auto __i = micron::ranges::begin(__v);
    const auto __e = micron::ranges::end(__v);
    if ( __i == __e ) return O{ fp::empty_container_error{} };

    E __head = static_cast<E>(*__i);
    ++__i;
    C __rest;
    const usize __hint = static_cast<usize>(micron::ranges::reserve_hint(__v));
    __impl::__reserve_if(__rest, __hint);
    for ( ; !(__i == __e); ++__i ) __rest.push_back(static_cast<typename C::value_type>(*__i));
    return O{ micron::make_tuple(micron::move(__head), micron::move(__rest)) };
  }
};

template<typename C>
[[nodiscard]] constexpr auto
uncons() noexcept
{
  return __uncons_fn<C>{};
}

};      // namespace lz
};      // namespace micron
