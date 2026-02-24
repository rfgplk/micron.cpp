// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "../../../array/arrays.hpp"
#include "config.hpp"

namespace abc
{
constexpr static const u64 __max_cached = 32;

class alloc_predictor
{
  // Keep sum and div maintained incrementally instead of recomputing each time
  micron::carray<size_t, __max_cached> allocs;
  u64 running_sum;
  u64 running_div;     // count of non-zero entries
  u32 index;

  static constexpr size_t
  align_to_page(const size_t n) noexcept
  {
    return (n + __system_pagesize - 1) & ~(size_t(__system_pagesize) - 1);
  }

  [[nodiscard]] inline u64
  of_class(const size_t sz) const noexcept
  {
    return (sz < __class_small)    ? __class_precise
           : (sz < __class_medium) ? __class_small
           : (sz < __class_large)  ? __class_medium
           : (sz < __class_1mb)    ? __class_large
           : (sz < __class_gb)     ? __class_1mb
                                   : __class_gb;
  }

public:
  ~alloc_predictor() = default;

  alloc_predictor() noexcept : allocs{}, running_sum(0), running_div(0), index(0) {}

  alloc_predictor(const alloc_predictor &) = default;
  alloc_predictor(alloc_predictor &&) = default;
  alloc_predictor &operator=(const alloc_predictor &) = default;
  alloc_predictor &operator=(alloc_predictor &&) = default;

  inline alloc_predictor &
  operator+=(const size_t n) noexcept
  {
    const u32 i = index;

    const size_t old = allocs[i];
    running_sum -= old;
    running_div -= (old != 0);

    allocs[i] = n;
    running_sum += n;
    running_div += (n != 0);

    index = (i + 1) & (__max_cached - 1);

    return *this;
  }

  [[nodiscard]] inline size_t
  predict_size(const size_t n) const noexcept
  {
    if ( __builtin_expect(running_div == 0, 0) )
      return align_to_page(n);

    const size_t mean_alloc = running_sum / running_div;

    if ( mean_alloc == 0 || n > mean_alloc * 3 || of_class(n) != of_class(mean_alloc) ) {
      return align_to_page(n);
    }

    return align_to_page(mean_alloc > n ? mean_alloc : n);
  }
};
};     // namespace abc
