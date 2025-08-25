//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/mem.hpp"
#include "types.hpp"
#include "type_traits.hpp"
#include <initializer_list>

namespace micron
{
template <typename T, typename F> struct pair {
  T a;     // or first
  F b;     // or second
  pair() : a(T()), b(F()) {}
  template <typename C>
    requires((micron::is_same_v<T, C>) or (micron::is_same_v<F, C>)
             or ((micron::is_convertible_v<T, C>) or micron::is_convertible_v<F, C>))
  pair(std::initializer_list<C> &&lst)
  {
    size_t i = 0;
    for ( C val : lst ) {
      if ( (i++) == 0 )
        a = static_cast<T>(micron::move(val));
      if ( (i++) == 1 )
        b = static_cast<F>(micron::move(val));
      if ( (i++) > 1 )
        break;
    }
  }
  template <typename C>
    requires((micron::is_same_v<T, C>) or (micron::is_same_v<F, C>)
             or ((micron::is_convertible_v<T, C>) or micron::is_convertible_v<F, C>))
  pair &
  operator=(std::initializer_list<C> &&lst)
  {
    size_t i = 0;
    for ( C val : lst ) {
      if ( (i++) == 0 )
        a = static_cast<T>(micron::move(val));
      if ( (i++) == 1 )
        b = static_cast<F>(micron::move(val));
      if ( (i++) > 1 )
        break;
    }
  }
  template <typename K, typename L>
    requires(micron::is_convertible_v<T, K> and micron::is_convertible_v<F, L>)
  pair(const K &x, const L &y) : a(static_cast<T>(x)), b(static_cast<F>(y))
  {
  }
  template <typename K, typename L>
    requires((micron::is_convertible_v<T, K> and micron::is_convertible_v<F, L>)
             and (!micron::is_same_v<T, K> and !micron::is_same_v<F, L>))
  pair(K &&x, L &&y) : a(micron::move(x)), b(micron::move(y))
  {
  }
  pair(T &&x, F &&y) : a(micron::move(x)), b(micron::move(y)) {}
  pair(const T &x, const F &y) : a(x), b(y) {}
  template <typename K, typename L> pair(K &&x, L &&y) : a(micron::move(x)), b(micron::move(y)) {}
  pair(const pair &o) : a(o.a), b(o.b) {}
  template <typename K, typename L> pair(const pair<K, L> &o) : a(static_cast<K>(o.a)), b(static_cast<L>(o.b)) {}
  pair(pair &&o) : a(micron::move(o.a)), b(micron::move(o.b))
  {
    if constexpr ( micron::is_class_v<T> )
      o.a.~T();
    else
      o.a = 0x0;
    if constexpr ( micron::is_class_v<F> )
      o.b.~F();
    else
      o.b = 0x0;
  }
  template <typename K, typename L> pair(pair<K, L> &&o) : a(micron::move(o.a)), b(micron::move(o.b))
  {
    if constexpr ( micron::is_class_v<T> )
      o.a.~T();
    else
      o.a = 0x0;
    if constexpr ( micron::is_class_v<F> )
      o.b.~F();
    else
      o.b = 0x0;
  }
  pair &
  operator=(const pair &o)
  {
    a = o.a;
    b = o.b;
    return *this;
  }
  template <typename K, typename L>
  pair &
  operator=(const pair<K, L> &o)
  {
    a = o.a;
    b = o.b;
    return *this;
  }
  pair &
  operator=(pair &&o)
  {
    a = micron::move(o.a);
    b = micron::move(o.b);
    return *this;
  }
  template <typename K, typename L>
  pair &
  operator=(pair<K, L> &&o)
  {
    a = micron::move(o.a);
    b = micron::move(o.b);
    return *this;
  }
  template <typename K = T>
    requires(micron::is_convertible_v<T, K>)
  pair &
  operator=(const K &x)
  {
    a = static_cast<T>(x);
    return *this;
  }

  template <typename L = F>
    requires(micron::is_convertible_v<F, L>)
  pair &
  operator=(const L &y)
  {
    b = static_cast<F>(y);
    return *this;
  }
  pair &
  operator=(T &&x)
  {
    a = micron::move(x);
    return *this;
  }
  template <typename L = F>
    requires(!micron::same_as<T, F>)
  pair &
  operator=(L &&y)
  {
    b = micron::move(y);
    return *this;
  }
  pair
  get(void) const
  {
    return pair(*this);
  }

  ~pair() = default;
};
template <typename C>
micron::pair<C, C>
tie(std::initializer_list<C> &&lst)
{
  micron::pair<C, C> c(micron::move(lst));
  return c;
}
template <typename C, typename D>
micron::pair<C, D>
tie(const C &c, const D &d)
{
  micron::pair<C, D> p(c, d);
  return p;
}
template <typename C, typename D>
micron::pair<C, D>
tie(C &&c, D &&d)
{
  micron::pair<C, D> p(micron::move(c), micron::move(d));
  return p;
}
};
