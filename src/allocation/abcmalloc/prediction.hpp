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

#include "../../array/arrays.hpp"
#include "config.hpp"

namespace abc
{
constexpr static const u64 __max_cached = 32;
class alloc_predictor
{
  micron::carray<size_t, __max_cached> allocs;
  size_t index;

  u64
  of_class(const size_t sz) const
  {
    if ( sz < __class_small )
      return __class_precise;
    else if ( sz < __class_medium and sz >= __class_small )
      return __class_small;
    else if ( sz < __class_large and sz >= __class_medium )
      return __class_medium;
    else if ( sz < __class_huge and sz >= __class_large )
      return __class_large;
    else if ( sz < __class_gb and sz >= __class_1mb )
      return __class_1mb;
    else
      return __class_gb;
  }

public:
  ~alloc_predictor() = default;
  alloc_predictor(void) = default;
  alloc_predictor(const alloc_predictor &) = default;
  alloc_predictor(alloc_predictor &&) = default;
  alloc_predictor &
  operator+=(const size_t n)
  {
    allocs[index++] = n;
    if ( index >= __max_cached ) [[unlikely]]
      index = 0;
    return *this;
  }

  inline size_t
  predict_size(const size_t n) const
  {
    u64 sum = 0;
    u64 div = 0;
    for ( size_t i = 0; i < __max_cached; i += 4 ) {
      sum += allocs[i];
      sum += allocs[i + 1];
      sum += allocs[i + 2];
      sum += allocs[i + 3];
      if ( allocs[i] )
        ++div;
      if ( allocs[i + 1] )
        ++div;
      if ( allocs[i + 2] )
        ++div;
      if ( allocs[i + 3] )
        ++div;
    }
    size_t mean_alloc = sum / div;

    if ( mean_alloc == 0 )
      return n;
    if ( n > mean_alloc * 3 )
      return n;
    if ( of_class(n) != of_class(mean_alloc) ) {
      return n;
    }
    return mean_alloc;
  }
};
};
