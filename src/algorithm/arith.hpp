//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../math/generic.hpp"
#include "../types.hpp"
#include "../type_traits.hpp"

// container agnostic functions for arith. operations on contiguous data
// T* sig. func. are blind iterators
// T& sig. func. take in a container which requires .begin and .end funcs

namespace micron
{

template <typename T>
concept isContainer = requires(T t) {
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
};

template <isContainer T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
pow(T &cont, const Y y)
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = math::power(*first, y);     // for clarity
}
template <typename T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
pow(T *__restrict first, T *__restrict end, const Y y)
{
  for ( ; first != end; ++first )
    *first = math::power(*first, y);     // for clarity
}

template <isContainer T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
add(T &cont, const Y y)
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first + y;     // for clarity
}

template <isContainer T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
multiply(T &cont, const Y y)
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first * y;     // for clarity
}

template <isContainer T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
divide(T &cont, const Y y)
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first / y;     // for clarity
}

template <isContainer T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
subtract(T &cont, const Y y)
{
  auto *first = cont.begin();
  auto *end = cont.end();
  for ( ; first != end; ++first )
    *first = *first - y;     // for clarity
}

template <isContainer T, typename... Args>
void
add(T &cont, const Args *__restrict... args)
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) + (... + (*(args + i)));
}
template <isContainer T, typename... Args>
void
multiply(T &cont, const Args *__restrict... args)
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) * (... + (*(args + i)));
}
template <isContainer T, typename... Args>
void
divide(T &cont, const Args *__restrict... args)
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) / (... + (*(args + i)));
}
template <isContainer T, typename... Args>
void
subtract(T &cont, const Args *__restrict... args)
{
  auto *first = cont.begin();
  auto n = cont.size();
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) - (... + (*(args + i)));
}

template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
add(T *first, T *end, const Y y)
{
  for ( ; first != end; ++first )
    *first = *first + y;
}
template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
multiply(T *first, T *end, const Y y)
{
  for ( ; first != end; ++first )
    *first = *first * y;
}
template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
divide(T *first, T *end, const Y y)
{
  for ( ; first != end; ++first )
    *first = *first / y;
}
template <class T, typename Y>
  requires micron::is_arithmetic_v<Y>
void
subtract(T *first, T *end, const Y y)
{
  for ( ; first != end; ++first )
    *first = *first - y;
}

template <class T, typename... Args>
void
add(size_t n, T *__restrict first, const Args *__restrict... args)
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) + (... + (*(args + i)));
}
template <class T, typename... Args>
void
multiply(size_t n, T *__restrict first, const Args *__restrict... args)
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) * (... + (*(args + i)));
}
template <class T, typename... Args>
void
divide(size_t n, T *__restrict first, const Args *__restrict... args)
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) / (... + (*(args + i)));
}
template <class T, typename... Args>
void
subtract(size_t n, T *__restrict first, const Args *__restrict... args)
{
  for ( size_t i = 0; i < n; i++ )
    (*(first + i)) = (*(first + i)) - (... + (*(args + i)));
}
};     // namespace micron
