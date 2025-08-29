//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../math/generic.hpp"
#include "../../types.hpp"
#include "__std.hpp"
#include "config.hpp"

namespace abc
{
// note u64 and size_t are the same type
inline size_t
__calculate_desired_space(size_t sz)
{
  // x^2/log(x)
  u64 t = static_cast<u64>((float)(sz * sz) / micron::math::log10f32((float)sz));
  float t_2 = (float)t / 4096;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((size_t)t_2) < 96 ? 96 : (size_t)t_2) * 4096;
  return sz;
}

void *
__get_kernel_memory(u64 sz)
{
  return micron::std_allocator<byte>::alloc(sz);
}

template <typename T>
T
__get_kernel_chunk(u64 sz)
{
  return { micron::std_allocator<byte>::alloc(sz), sz };
}

template <typename T>
void
__release_kernel_chunk(const T &mem)
{
  micron::std_allocator<byte>::dealloc(mem.ptr, mem.len);
}
};
