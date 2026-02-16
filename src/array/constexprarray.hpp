//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../concepts.hpp"
namespace micron
{

template <is_constexpr_valid T, size_t N = 64>
  requires(N > 0)
struct constexpr_array {
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  alignas(64) T stack[N];

  constexpr constexpr_array() = default;

  constexpr constexpr_array(const T &val)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = val;
  }

  constexpr constexpr_array(const std::initializer_list<T> &lst)
  {
    size_t i = 0;
    for ( auto &v : lst )
      stack[i++] = v;
  }

  constexpr constexpr_array(const constexpr_array &o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = o.stack[i];
  }

  constexpr constexpr_array(constexpr_array &&o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = micron::move(o.stack[i]);
  }

  constexpr constexpr_array &
  operator=(const constexpr_array &o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = o.stack[i];
    return *this;
  }

  constexpr constexpr_array &
  operator=(constexpr_array &&o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = micron::move(o.stack[i]);
    return *this;
  }
  constexpr const_iterator
  begin(void) const noexcept
  {
    return micron::real_addr_as<T>(stack[0]);
  }
  constexpr const_iterator
  end(void) const noexcept
  {
    return micron::real_addr_as<T>(stack[N]);
  }
  constexpr iterator
  begin(void) noexcept
  {
    return micron::real_addr_as<T>(stack[0]);
  }
  constexpr iterator
  end(void) noexcept
  {
    return micron::real_addr_as<T>(stack[N]);
  }
  constexpr T &
  operator[](size_t i)
  {
    return stack[i];
  }
  constexpr const T &
  operator[](size_t i) const
  {
    return stack[i];
  }

  constexpr size_t
  size() const
  {
    return N;
  }

  constexpr T *
  data()
  {
    return stack;
  }
  constexpr const T *
  data() const
  {
    return stack;
  }

  template <typename F>
    requires micron::is_arithmetic_v<F>
  constexpr constexpr_array &
  fill(F val)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = val;
    return *this;
  }

  constexpr constexpr_array &
  operator+=(const T &o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] += o;
    return *this;
  }

  constexpr constexpr_array &
  operator-=(const T &o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] -= o;
    return *this;
  }

  constexpr constexpr_array &
  operator*=(const T &o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] *= o;
    return *this;
  }

  constexpr constexpr_array &
  operator/=(const T &o)
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] /= o;
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array &
  operator+=(const constexpr_array<T, M> &o)
  {
    for ( size_t i = 0; i < M; ++i )
      stack[i] += o.stack[i];
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array &
  operator-=(const constexpr_array<T, M> &o)
  {
    for ( size_t i = 0; i < M; ++i )
      stack[i] -= o.stack[i];
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array &
  operator*=(const constexpr_array<T, M> &o)
  {
    for ( size_t i = 0; i < M; ++i )
      stack[i] *= o.stack[i];
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array &
  operator/=(const constexpr_array<T, M> &o)
  {
    for ( size_t i = 0; i < M; ++i )
      stack[i] /= o.stack[i];
    return *this;
  }

  constexpr T
  sum() const
  {
    T sm{};
    for ( size_t i = 0; i < N; ++i )
      sm += stack[i];
    return sm;
  }

  constexpr T
  mul() const
  {
    T prod = stack[0];
    for ( size_t i = 1; i < N; ++i )
      prod *= stack[i];
    return prod;
  }

  constexpr bool
  all(const T &val) const
  {
    for ( size_t i = 0; i < N; ++i )
      if ( stack[i] != val )
        return false;
    return true;
  }

  constexpr bool
  any(const T &val) const
  {
    for ( size_t i = 0; i < N; ++i )
      if ( stack[i] == val )
        return true;
    return false;
  }

  constexpr constexpr_array &
  sqrt()
  {
    for ( size_t i = 0; i < N; ++i )
      stack[i] = static_cast<T>(math::sqrt(static_cast<double>(stack[i])));
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array
  operator+(const constexpr_array<T, M> &o) const
  {
    constexpr_array res(*this);
    res += o;
    return res;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array
  operator-(const constexpr_array<T, M> &o) const
  {
    constexpr_array res(*this);
    res -= o;
    return res;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array
  operator*(const constexpr_array<T, M> &o) const
  {
    constexpr_array res(*this);
    res *= o;
    return res;
  }

  template <size_t M>
    requires(M <= N)
  constexpr constexpr_array
  operator/(const constexpr_array<T, M> &o) const
  {
    constexpr_array res(*this);
    res /= o;
    return res;
  }
  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }
};

template <class T, size_t N = 64> using constarray = constexpr_array<T, N>;

};
