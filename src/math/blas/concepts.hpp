//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"

namespace micron
{
namespace math
{
namespace blas
{

template <typename T>
concept blas_real = ieee754_floating<T>;

template <typename T>
concept blas_scalar = micron::is_arithmetic_v<T>;

template <typename V>
concept vec_view_like = requires(V v, V::value_type *p) {
  typename V::value_type;
  { v.data } -> micron::convertible_to<typename V::value_type *>;
  { v.n } -> micron::convertible_to<usize>;
  { v.inc } -> micron::convertible_to<ssize_t>;
};

template <typename M>
concept row_view_like = requires(M m) {
  typename M::value_type;
  typename M::row_major_tag;
  { m.data } -> micron::convertible_to<typename M::value_type *>;
  { m.rows } -> micron::convertible_to<usize>;
  { m.cols } -> micron::convertible_to<usize>;
  { m.ld } -> micron::convertible_to<usize>;
};

template <typename M>
concept col_view_like = requires(M m) {
  typename M::value_type;
  typename M::col_major_tag;
  { m.data } -> micron::convertible_to<typename M::value_type *>;
  { m.rows } -> micron::convertible_to<usize>;
  { m.cols } -> micron::convertible_to<usize>;
  { m.ld } -> micron::convertible_to<usize>;
};

template <typename M>
concept mat_view_like = row_view_like<M> or col_view_like<M>;

};     // namespace blas
};     // namespace math
};     // namespace micron
