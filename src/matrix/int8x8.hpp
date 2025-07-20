//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../memory/cmemory.hpp"
#include "../types.hpp"
#include <initializer_list>
#include <type_traits>

// NOTE: these matrix types take a lot of inspiration from ARMs NEON SIMD extensions

namespace micron
{

// NOTE: row major
template <typename B, uint32_t C, uint32_t R>
  requires(std::is_arithmetic_v<B> && ((C * sizeof(C)) * (R * sizeof(R)) <= (1024 / sizeof(byte))))
struct int_matrix_base {
  static constexpr uint32_t __size = C * R;
  B __mat[__size];     // yes, public for conv.
  ~int_matrix_base(void) = default;
  int_matrix_base(void) { micron::czero<__size>(__mat); }
  int_matrix_base(B n) { micron::cmemset<__size>(__mat, n); }
  template <typename... Args>
    requires(sizeof...(Args) == __size)
  int_matrix_base(Args... args) : __mat{ args... }
  {
  }
  int_matrix_base(const std::initializer_list<B> &lst)
  {
    if ( lst.size() != __size )
      throw except::library_error("micron::int8x8 initializer_list out of bounds");
    micron::bytecpy(__mat, lst.data(), __size * sizeof(B));
  }
  int_matrix_base(const int_matrix_base &o) { micron::cmemcpy<__size>(__mat, o.__mat); }
  int_matrix_base(int_matrix_base &&o)
  {
    micron::cmemcpy<__size>(__mat, o.__mat);
    micron::czero<__size>(o.__mat);
  }
  int_matrix_base &
  operator=(const int_matrix_base &o)
  {
    micron::cmemcpy<__size>(__mat, o.__mat);
    return *this;
  }
  int_matrix_base &
  operator=(int_matrix_base &&o)
  {
    micron::cmemcpy<__size>(__mat, o.__mat);
    micron::czero<__size>(o.__mat);
    return *this;
  }
  B &
  operator[](uint32_t r, uint32_t c)
  {
    return __mat[r * C + c];
  }
  int_matrix_base &
  operator+=(const int_matrix_base &o)
  {
    for ( size_t i = 0; i < __size; i++ )
      __mat[i] += o.__mat[i];
    return *this;
  }
  int_matrix_base &
  operator-=(const int_matrix_base &o)
  {
    for ( size_t i = 0; i < __size; i++ )
      __mat[i] -= o.__mat[i];
    return *this;
  }

  int_matrix_base
  operator*(const int_matrix_base &o)
  {
    int_matrix_base result;
    for ( B i = 0; i < R; i++ )
    {
      for(B j = 0; j < C; j++)
        for(B k = 0; k < C; k++)
          result[i, j] += __mat[i * C + k] * o.__mat[k * C + j];
    }
    return result;
  }

};

};
