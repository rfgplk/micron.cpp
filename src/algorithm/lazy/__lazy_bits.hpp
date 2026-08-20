//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../function.hpp"
#include "../../memory/addr.hpp"
#include "../../range.hpp"
#include "../../tuple.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
namespace lz
{

// %%%%%%%%%%%%%%%%%%%%%%
// pipe
using micron::operator|;

//   exact    n is known before iterating
//   bounded  n is known before iterating, and is an upper bound
//   unknown  nothing is known; grow
//   endless  never terminates on its own; only a take/take_while makes it collectable
enum class size_kind : u8 { exact = 0, bounded = 1, unknown = 2, endless = 3 };

template<typename V> struct __size_kind_of {
  static constexpr size_kind value = size_kind::unknown;
};

template<typename V>
  requires requires { micron::remove_cvref_t<V>::__kind; }
struct __size_kind_of<V> {
  static constexpr size_kind value = micron::remove_cvref_t<V>::__kind;
};

template<typename V> inline constexpr size_kind kind_of = __size_kind_of<V>::value;

// filter's transform
constexpr size_kind
degrade(size_kind __k) noexcept
{
  return __k == size_kind::exact ? size_kind::bounded : __k;
}

// take_while's transform
constexpr size_kind
bound_endless(size_kind __k) noexcept
{
  return __k == size_kind::endless ? size_kind::unknown : degrade(__k);
}

inline constexpr usize no_static_size = 0;

template<typename V> struct __static_size_of {
  static constexpr usize value = no_static_size;
};

template<typename V>
  requires requires { requires micron::remove_cvref_t<V>::__static_size != no_static_size; }
struct __static_size_of<V> {
  static constexpr usize value = micron::remove_cvref_t<V>::__static_size;
};

template<typename V> inline constexpr usize static_size_of = __static_size_of<V>::value;

template<typename C>
concept cheaply_sized = micron::is_contiguous_container<micron::remove_cvref_t<C>> || micron::has_static_size<micron::remove_cvref_t<C>>
                        || requires { typename micron::remove_cvref_t<C>::__cheap_size_tag; };

template<typename V> struct __is_materializing_of {
  static constexpr bool value = false;
};

template<typename V>
  requires requires { micron::remove_cvref_t<V>::__is_materializing; }
struct __is_materializing_of<V> {
  static constexpr bool value = micron::remove_cvref_t<V>::__is_materializing;
};

template<typename V> inline constexpr bool materializes = __is_materializing_of<V>::value;

template<typename V>
concept reversible = micron::same_as<micron::ranges::iterator_t<V>, micron::ranges::sentinel_t<V>>
                     && micron::bidirectional_iterator<micron::ranges::iterator_t<V>>;

template<typename V>
concept flat_exact = (kind_of<V> == size_kind::exact) && micron::contiguous_iterator<micron::ranges::iterator_t<V>>;

namespace __impl
{

template<typename P>
constexpr decltype(auto)
__first_of(P &&__p)
{
  if constexpr ( requires { __p.a; } )
    return (micron::forward<P>(__p).a);
  else if constexpr ( requires { __p.key; } )
    return (micron::forward<P>(__p).key);
  else if constexpr ( requires { __p.first; } )
    return (micron::forward<P>(__p).first);
  else
    return micron::get<0>(micron::forward<P>(__p));
}

template<typename P>
constexpr decltype(auto)
__second_of(P &&__p)
{
  if constexpr ( requires { __p.b; } )
    return (micron::forward<P>(__p).b);
  else if constexpr ( requires { __p.value; } )
    return (micron::forward<P>(__p).value);
  else if constexpr ( requires { __p.second; } )
    return (micron::forward<P>(__p).second);
  else
    return micron::get<1>(micron::forward<P>(__p));
}

template<typename V, typename Fn>
[[gnu::always_inline]] constexpr void
__each(const V &__v, Fn &&__fn)
{
  auto __i = micron::ranges::begin(__v);
  const auto __e = micron::ranges::end(__v);
  for ( ; !(__i == __e); ++__i ) __fn(*__i);
}

template<typename V, typename Fn>
[[gnu::always_inline]] constexpr bool
__each_while(const V &__v, Fn &&__fn)
{
  auto __i = micron::ranges::begin(__v);
  const auto __e = micron::ranges::end(__v);
  for ( ; !(__i == __e); ++__i )
    if ( !__fn(*__i) ) return false;
  return true;
}

};      // namespace __impl

template<typename F> using fn_carrier = micron::conditional_t<micron::is_empty_v<F>, F, const F *>;

template<typename F>
constexpr fn_carrier<F>
hold(const F &__f) noexcept
{
  if constexpr ( micron::is_empty_v<F> )
    return __f;
  else
    return micron::addressof(__f);
}

template<typename F, typename... A>
constexpr decltype(auto)
call(const fn_carrier<F> &__c, A &&...__a)
{
  if constexpr ( micron::is_empty_v<F> )
    return __c(micron::forward<A>(__a)...);
  else
    return (*__c)(micron::forward<A>(__a)...);
}

};      // namespace lz
};      // namespace micron
