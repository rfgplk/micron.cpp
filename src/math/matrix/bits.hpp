//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../__special/initializer_list"
#include "../except.hpp"
#include "../memory/cmemory.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "control.hpp"

namespace micron
{

// NOTE: row major
template <typename B, u32 C, u32 R>
  requires(micron::is_arithmetic_v<B> and ((C * sizeof(C)) * (R * sizeof(R)) <= (4096 / sizeof(byte)))
           && ((((C * R * sizeof(R))) % 64) == 0))
class int_matrix_base_avx
{
  using category_type = type_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef size_t size_type;
  typedef B value_type;
  typedef B &reference;
  typedef B &ref;
  typedef const B &const_reference;
  typedef const B &const_ref;
  typedef B *pointer;
  typedef const B *const_pointer;
  typedef B *iterator;
  typedef const B *const_iterator;

public:
  static constexpr u32 __size = C * R;
  B alignas(64) __mat[__size];     // yes, public for conv.
  ~int_matrix_base_avx(void) = default;

  int_matrix_base_avx(void) : __mat{} { micron::czero<__size>(__mat); }

  int_matrix_base_avx(B n) : __mat{} { micron::ctypeset<__size, B>(__mat, n); }

  template <typename... Args>
    requires(sizeof...(Args) == __size)
  int_matrix_base_avx(Args... args) : __mat{ args... }
  {
  }

  int_matrix_base_avx(const std::initializer_list<B> &lst)
  {
    if ( lst.size() != __size )
      exc<except::library_error>("micron::int8x8 initializer_list out of bounds");
    micron::bytecpy(__mat, lst.data(), __size * sizeof(B));
  }

  int_matrix_base_avx(const int_matrix_base_avx &o) { micron::cmemcpy<__size>(__mat, o.__mat); }

  int_matrix_base_avx(int_matrix_base_avx &&o)
  {
    micron::cmemcpy<__size>(__mat, o.__mat);
    micron::czero<__size>(o.__mat);
  }

  int_matrix_base_avx &
  operator=(const int_matrix_base_avx &o)
  {
    micron::cmemcpy<__size>(__mat, o.__mat);
    return *this;
  }

  int_matrix_base_avx &
  operator=(int_matrix_base_avx &&o)
  {
    micron::cmemcpy<__size>(__mat, o.__mat);
    micron::czero<__size>(o.__mat);
    return *this;
  }

  // start of scalar funcs
  // scalar addition
  int_matrix_base_avx &
  operator=(B sc)
  {
    micron::memset(&__mat[0], sc, __size);
    return *this;
  }

  int_matrix_base_avx &
  operator+=(B sc)
  {
    // NOTE: this will autovectorize or entirely get opt out during ct. ie
    // (mov    $res,%edi)
    for ( size_t i = 0; i < __size; i += 8 ) {
      __mat[i] += sc;
      __mat[i + 1] += sc;
      __mat[i + 2] += sc;
      __mat[i + 3] += sc;
      __mat[i + 4] += sc;
      __mat[i + 5] += sc;
      __mat[i + 6] += sc;
      __mat[i + 7] += sc;
    }
    return *this;
  }

  int_matrix_base_avx &
  operator-=(B sc)
  {
    // NOTE: this will autovectorize or entirely get opt out during ct. ie
    // (mov    $res,%edi)
    for ( size_t i = 0; i < __size; i += 8 ) {
      __mat[i] -= sc;
      __mat[i + 1] -= sc;
      __mat[i + 2] -= sc;
      __mat[i + 3] -= sc;
      __mat[i + 4] -= sc;
      __mat[i + 5] -= sc;
      __mat[i + 6] -= sc;
      __mat[i + 7] -= sc;
    }
    return *this;
  }

  int_matrix_base_avx &
  operator/=(B sc)
  {
    // NOTE: this will autovectorize or entirely get opt out during ct. ie
    // (mov    $res,%edi)
    for ( size_t i = 0; i < __size; i += 8 ) {
      __mat[i] /= sc;
      __mat[i + 1] /= sc;
      __mat[i + 2] /= sc;
      __mat[i + 3] /= sc;
      __mat[i + 4] /= sc;
      __mat[i + 5] /= sc;
      __mat[i + 6] /= sc;
      __mat[i + 7] /= sc;
    }
    return *this;
  }

  int_matrix_base_avx &
  operator*=(B sc)
  {
    // NOTE: this will autovectorize or entirely get opt out during ct. ie
    // (mov    $res,%edi)
    for ( size_t i = 0; i < __size; i += 8 ) {
      __mat[i] *= sc;
      __mat[i + 1] *= sc;
      __mat[i + 2] *= sc;
      __mat[i + 3] *= sc;
      __mat[i + 4] *= sc;
      __mat[i + 5] *= sc;
      __mat[i + 6] *= sc;
      __mat[i + 7] *= sc;
    }
    return *this;
  }

  // end of scalar funcs
  B &
  col(u32 c)
  {
    return __mat[c];
  }

  B &
  row(u32 r)
  {
    return __mat[r * C];
  }

  B &
  operator[](u32 r)
  {
    return __mat[r * C];
  }

  B &
  operator[](u32 r, u32 c)
  {
    return __mat[r * C + c];
  }

  const B &
  col(u32 c) const
  {
    return __mat[c];
  }

  const B &
  row(u32 r) const
  {
    return __mat[r * C];
  }

  const B &
  operator[](u32 r) const
  {
    return __mat[r * C];
  }

  const B &
  operator[](u32 r, u32 c) const
  {
    return __mat[r * C + c];
  }

  int_matrix_base_avx &
  operator+=(const int_matrix_base_avx &o)
  {
    for ( size_t i = 0; i < __size; i++ )
      __mat[i] += o.__mat[i];
    return *this;
  }

  int_matrix_base_avx &
  operator-=(const int_matrix_base_avx &o)
  {
    for ( size_t i = 0; i < __size; i++ )
      __mat[i] -= o.__mat[i];
    return *this;
  }

  int_matrix_base_avx &
  operator*=(const int_matrix_base_avx &o)
  {
    for ( size_t i = 0; i < __size; i++ )
      __mat[i] *= o.__mat[i];
    return *this;
  }

  int_matrix_base_avx &
  operator/=(const int_matrix_base_avx &o)
  {
    for ( size_t i = 0; i < __size; i++ )
      __mat[i] /= o.__mat[i];
    return *this;
  }

  int_matrix_base_avx
  div_scalar(B sc) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__mat[i] = __mat[i] / sc;
    return result;
  }

  int_matrix_base_avx
  transpose() const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < R; ++i )
      for ( size_t j = 0; j < C; ++j )
        result[j, i] = (*this)[i, j];
    return result;
  }

  int_matrix_base_avx
  add_scalar(B sc) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__mat[i] = __mat[i] + sc;
    return result;
  }

  int_matrix_base_avx
  sub_scalar(B sc) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__mat[i] = __mat[i] - sc;
    return result;
  }

  int_matrix_base_avx
  scale(B sc) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__mat[i] = __mat[i] * sc;
    return result;
  }

  int_matrix_base_avx
  mul(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < R; i++ ) {
      for ( size_t j = 0; j < C; j++ )
        for ( size_t k = 0; k < C; k++ )
          result[i, j] += __mat[i * C + k] * o.__mat[k * C + j];
    }
    return result;
  }

  // element wise
  int_matrix_base_avx
  operator*(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < R; ++i )
      for ( size_t j = 0; j < C; ++j )
        result[i, j] = (*this)[i, j] * o[i, j];
    return result;
  }

  int_matrix_base_avx
  operator+(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < R; ++i )
      for ( size_t j = 0; j < C; ++j )
        result[i, j] = (*this)[i, j] + o[i, j];
    return result;
  }

  int_matrix_base_avx
  operator-(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < R; ++i )
      for ( size_t j = 0; j < C; ++j )
        result[i, j] = (*this)[i, j] - o[i, j];
    return result;
  }

  int_matrix_base_avx
  operator/(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( size_t i = 0; i < R; ++i )
      for ( size_t j = 0; j < C; ++j )
        result[i, j] = (*this)[i, j] / o[i, j];
    return result;
  }
};

template <typename B, u32 C, u32 R> using int_matrix_base = int_matrix_base_avx<B, C, R>;

};     // namespace micron
