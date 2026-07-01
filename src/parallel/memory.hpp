//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

#include "../algorithm/memory.hpp"

namespace micron
{
namespace parallel
{

// dst[i] = src[i]
template<class T, class F>
[[nodiscard]] micron::task<void>
copy_n(const T *__src, F *__dst, usize __n)
{
  auto __leaf = [__src, __dst](const T *__a, const T *__b) {
    const usize __off = static_cast<usize>(__a - __src);
    micron::copy_n(__a, __dst + __off, static_cast<usize>(__b - __a));
  };
  return __pmap<const T *, decltype(__leaf)>(__src, __src + __n, __leaf, __grain_for(__n));
}

// dst[i] = move(src[i])
template<class T, class F>
[[nodiscard]] micron::task<void>
move_n(T *__src, F *__dst, usize __n)
{
  auto __leaf = [__src, __dst](T *__a, T *__b) {
    const usize __off = static_cast<usize>(__a - __src);
    micron::move_n(__a, __dst + __off, static_cast<usize>(__b - __a));
  };
  return __pmap<T *, decltype(__leaf)>(__src, __src + __n, __leaf, __grain_for(__n));
}

// count is in bytes
template<class T>
[[nodiscard]] micron::task<void>
set_n(T *__dst, unsigned char __val, usize __nbytes)
{
  using BP = unsigned char *;
  BP __base = reinterpret_cast<BP>(__dst);
  auto __leaf = [__val](BP __a, BP __b) { micron::set_n(__a, __val, static_cast<usize>(__b - __a)); };
  return __pmap<BP, decltype(__leaf)>(__base, __base + __nbytes, __leaf, __grain_for(__nbytes));
}

template<class T>
[[nodiscard]] micron::task<void>
zero_n(T *__dst, usize __nbytes)
{
  using BP = unsigned char *;
  BP __base = reinterpret_cast<BP>(__dst);
  auto __leaf = [](BP __a, BP __b) { micron::zero_n(__a, static_cast<usize>(__b - __a)); };
  return __pmap<BP, decltype(__leaf)>(__base, __base + __nbytes, __leaf, __grain_for(__nbytes));
}

template<class T>
[[nodiscard]] micron::task<void>
swap_n(T *__a0, T *__b0, usize __nbytes)
{
  using BP = unsigned char *;
  BP __abase = reinterpret_cast<BP>(__a0);
  BP __bbase = reinterpret_cast<BP>(__b0);
  auto __leaf = [__abase, __bbase](BP __a, BP __b) {
    const usize __off = static_cast<usize>(__a - __abase);
    micron::swap_n(__a, __bbase + __off, static_cast<usize>(__b - __a));
  };
  return __pmap<BP, decltype(__leaf)>(__abase, __abase + __nbytes, __leaf, __grain_for(__nbytes));
}

};      // namespace parallel
};      // namespace micron
