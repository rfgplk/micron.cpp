//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../__special/initializer_list"
#include "../../except.hpp"
#include "../../memory/cmemory.hpp"
#include "../../tags.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../control.hpp"

namespace micron
{

template <typename B, usize C, usize R>
inline constexpr usize int_matrix_align_v = (C * R * sizeof(B) <= 16)   ? 16
                                            : (C * R * sizeof(B) <= 32) ? 32
                                                                        : 64;

// NOTE: row major
template <typename B, usize C, usize R>
  requires(micron::is_arithmetic_v<B> && C >= 1 && R >= 1 && (C * R * sizeof(B) <= 4096))
class alignas(int_matrix_align_v<B, C, R>) int_matrix_base_avx
{
  using category_type = type_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef usize size_type;
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
  static constexpr usize __size = C * R;
  B data[__size];     // public for conv.  Class alignas carries through.
  ~int_matrix_base_avx(void) = default;

  int_matrix_base_avx(void) : data{} { micron::czero<__size>(data); }

  int_matrix_base_avx(B n) : data{} { micron::ctypeset<__size, B>(data, n); }

  template <typename... Args>
    requires(sizeof...(Args) == __size)
  int_matrix_base_avx(Args... args) : data{ args... }
  {
  }

  int_matrix_base_avx(const std::initializer_list<B> &lst)
  {
    if ( lst.size() != __size ) exc<except::library_error>("micron::int_matrix_base_avx initializer_list out of bounds");
    micron::bytecpy(data, lst.begin(), __size * sizeof(B));
  }

  int_matrix_base_avx(const int_matrix_base_avx &o) { micron::cmemcpy<__size>(data, o.data); }

  int_matrix_base_avx(int_matrix_base_avx &&o)
  {
    micron::cmemcpy<__size>(data, o.data);
    micron::czero<__size>(o.data);
  }

  int_matrix_base_avx &
  operator=(const int_matrix_base_avx &o)
  {
    micron::cmemcpy<__size>(data, o.data);
    return *this;
  }

  int_matrix_base_avx &
  operator=(int_matrix_base_avx &&o)
  {
    micron::cmemcpy<__size>(data, o.data);
    micron::czero<__size>(o.data);
    return *this;
  }

  int_matrix_base_avx &
  operator=(B sc)
  {
    micron::typeset(&data[0], sc, __size);
    return *this;
  }

  int_matrix_base_avx &
  operator+=(B sc)
  {
    // dropping the hand unrolling so this works for any size
    for ( usize i = 0; i < __size; ++i ) data[i] += sc;
    return *this;
  }

  int_matrix_base_avx &
  operator-=(B sc)
  {
    for ( usize i = 0; i < __size; ++i ) data[i] -= sc;
    return *this;
  }

  int_matrix_base_avx &
  operator/=(B sc)
  {
    for ( usize i = 0; i < __size; ++i ) data[i] /= sc;
    return *this;
  }

  int_matrix_base_avx &
  operator*=(B sc)
  {
    for ( usize i = 0; i < __size; ++i ) data[i] *= sc;
    return *this;
  }

  // end of scalar funcs
  B &
  col(usize c)
  {
    return data[c];
  }

  B &
  row(usize r)
  {
    return data[r * C];
  }

  B &
  operator[](usize r)
  {
    return data[r * C];
  }

  B &
  operator[](usize r, usize c)
  {
    return data[r * C + c];
  }

  const B &
  col(usize c) const
  {
    return data[c];
  }

  const B &
  row(usize r) const
  {
    return data[r * C];
  }

  const B &
  operator[](usize r) const
  {
    return data[r * C];
  }

  const B &
  operator[](usize r, usize c) const
  {
    return data[r * C + c];
  }

  int_matrix_base_avx &
  operator+=(const int_matrix_base_avx &o)
  {
    for ( usize i = 0; i < __size; i++ ) data[i] += o.data[i];
    return *this;
  }

  int_matrix_base_avx &
  operator-=(const int_matrix_base_avx &o)
  {
    for ( usize i = 0; i < __size; i++ ) data[i] -= o.data[i];
    return *this;
  }

  int_matrix_base_avx &
  operator*=(const int_matrix_base_avx &o)
  {
    for ( usize i = 0; i < __size; i++ ) data[i] *= o.data[i];
    return *this;
  }

  int_matrix_base_avx &
  operator/=(const int_matrix_base_avx &o)
  {
    for ( usize i = 0; i < __size; i++ ) data[i] /= o.data[i];
    return *this;
  }

  int_matrix_base_avx
  div_scalar(B sc) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < __size; ++i ) result.data[i] = data[i] / sc;
    return result;
  }

  int_matrix_base_avx
  transpose() const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) result[j, i] = (*this)[i, j];
    return result;
  }

  int_matrix_base_avx
  add_scalar(B sc) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < __size; ++i ) result.data[i] = data[i] + sc;
    return result;
  }

  int_matrix_base_avx
  sub_scalar(B sc) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < __size; ++i ) result.data[i] = data[i] - sc;
    return result;
  }

  int_matrix_base_avx
  scale(B sc) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < __size; ++i ) result.data[i] = data[i] * sc;
    return result;
  }

  int_matrix_base_avx
  mul(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < R; i++ ) {
      for ( usize j = 0; j < C; j++ )
        for ( usize k = 0; k < C; k++ ) result[i, j] += data[i * C + k] * o.data[k * C + j];
    }
    return result;
  }

  // element wise
  int_matrix_base_avx
  operator*(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) result[i, j] = (*this)[i, j] * o[i, j];
    return result;
  }

  int_matrix_base_avx
  operator+(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) result[i, j] = (*this)[i, j] + o[i, j];
    return result;
  }

  int_matrix_base_avx
  operator-(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) result[i, j] = (*this)[i, j] - o[i, j];
    return result;
  }

  int_matrix_base_avx
  operator/(const int_matrix_base_avx &o) const
  {
    int_matrix_base_avx result;
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) result[i, j] = (*this)[i, j] / o[i, j];
    return result;
  }
};

template <typename B, usize C, usize R> using int_matrix_base = int_matrix_base_avx<B, C, R>;

};     // namespace micron
