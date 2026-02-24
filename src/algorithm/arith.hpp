//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../math/generic.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

// container agnostic functions for arith. operations on contiguous data
// T* sig. func. are blind iterators
// T& sig. func. take in a container which requires .begin and .end funcs

namespace micron
{

template <is_iterable_container T, typename Y>
  requires micron::is_floating_point_v<Y>
void
pow(T &cont, const Y y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = math::powerf(*first, y);     // for clarity
}

template <is_iterable_container T, typename Y>
  requires(micron::is_floating_point_v<typename T::value_type> and micron::is_integral_v<Y>)
void
pow(T &cont, const Y y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = math::powerf(*first, y);     // for clarity
}

template <is_iterable_container T, typename Y>
  requires(micron::is_integral_v<typename T::value_type> and micron::is_integral_v<Y>)
void
pow(T &cont, const Y y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = math::power(*first, y);     // for clarity
}

template <typename T, typename Y>
  requires micron::is_floating_point_v<Y>
void
pow(T *__restrict first, T *__restrict end, const Y y) noexcept
{
  for ( ; first != end; ++first )
    *first = math::powerf(*first, y);     // for clarity
}

template <typename T, typename Y>
  requires micron::is_integral_v<Y>
void
pow(T *__restrict first, T *__restrict end, const Y y) noexcept
{
  for ( ; first != end; ++first )
    *first = math::power(*first, y);     // for clarity
}

template <is_iterable_container T>
void
add(T &cont, const typename T::value_type y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first + y;     // for clarity
}

template <is_iterable_container T>
auto
multiply(T &cont) noexcept
{
  typename T::value_type r = 1;
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    r *= *first;     // for clarity
  return r;
}

template <is_iterable_container T>
void
multiply(T &cont, const typename T::value_type y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first * y;     // for clarity
}

template <is_iterable_container T>
auto
mul(T &cont) noexcept
{
  return multiply(cont);
}

template <is_iterable_container T>
void
mul(T &cont, const typename T::value_type y) noexcept
{
  multiply(cont, y);
}

template <is_iterable_container T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
divide(T &cont, const Y y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first / y;     // for clarity
}

template <is_iterable_container T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
subtract(T &cont, const Y y) noexcept
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first - y;     // for clarity
}

template <is_iterable_container T, typename... Args>
void
add(T &cont, const Args *__restrict... args) noexcept
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) + (... + (*(args + i)));
}

template <is_iterable_container T, typename... Args>
void
multiply(T &cont, const Args *__restrict... args) noexcept
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) * (... + (*(args + i)));
}

template <is_iterable_container T, typename... Args>
void
divide(T &cont, const Args *__restrict... args) noexcept
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) / (... + (*(args + i)));
}

template <is_iterable_container T, typename... Args>
void
subtract(T &cont, const Args *__restrict... args) noexcept
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) - (... + (*(args + i)));
}

template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
add(T *first, T *end, const Y y) noexcept
{
  for ( ; first != end; ++first )
    *first = *first + y;
}

template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
multiply(T *first, T *end, const Y y) noexcept
{
  for ( ; first != end; ++first )
    *first = *first * y;
}

template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
divide(T *first, T *end, const Y y) noexcept
{
  for ( ; first != end; ++first )
    *first = *first / y;
}

template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
subtract(T *first, T *end, const Y y) noexcept
{
  for ( ; first != end; ++first )
    *first = *first - y;
}

template <class T, typename... Args>
void
add(size_t n, T *__restrict first, const Args *__restrict... args) noexcept
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) + (... + (*(args + i)));
}

template <class T, typename... Args>
void
multiply(size_t n, T *__restrict first, const Args *__restrict... args) noexcept
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) * (... + (*(args + i)));
}

template <class T, typename... Args>
void
divide(size_t n, T *__restrict first, const Args *__restrict... args) noexcept
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) / (... + (*(args + i)));
}

template <class T, typename... Args>
void
subtract(size_t n, T *__restrict first, const Args *__restrict... args) noexcept
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) - (... + (*(args + i)));
}
};     // namespace micron
