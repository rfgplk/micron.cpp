//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"

namespace micron
{
namespace math
{
namespace sparse
{

template<arith_scalar T, micron::integral I = u32> struct sparse_vec {
  using value_type = T;
  using index_type = I;
  using vec_i = micron::vector<I, micron::allocator_serial<>, false>;
  using vec_v = micron::vector<T, micron::allocator_serial<>, false>;

  usize n{ 0 };
  vec_i idx;
  vec_v values;

  sparse_vec() noexcept = default;

  explicit sparse_vec(usize len) : n(len), idx(0), values(0) { }

  sparse_vec(const sparse_vec &) = default;
  sparse_vec(sparse_vec &&) noexcept = default;
  sparse_vec &operator=(const sparse_vec &) = default;
  sparse_vec &operator=(sparse_vec &&) noexcept = default;

  [[nodiscard, gnu::always_inline]] usize
  nnz() const noexcept
  {
    return values.size();
  }
};

};      // namespace sparse
};      // namespace math
};      // namespace micron
