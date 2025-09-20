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

#include "book.hpp"
#include "config.hpp"
#include "harden.hpp"
#include "hooks.hpp"
#include "stats.hpp"

namespace abc
{

struct cache {
  micron::__chunk<byte>
  __heap_grow(const size_t sz)
  {
    return { reinterpret_cast<byte *>(micron::sbrk(sz)), sz };
  }
  void
  __heap_shrink(const ssize_t sz)
  {
    micron::sbrk((-1) * sz);
  }

  micron::__chunk<byte>
  grow(const size_t sz)
  {
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);
    micron::__chunk<byte> memory;
    if ( memory = __heap_grow(sz); !memory.zero() ) {
      zero_on_alloc(memory.ptr, memory.len);
      sanitize_on_alloc(memory.ptr, memory.len);
      collect_stats<stat_type::total_memory_throughput>(memory.len);
      return memory;
    }
    return { (byte *)-1, micron::numeric_limits<size_t>::max() };
  }

  void
  shrink(const ssize_t sz)
  {
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(sz);
    __heap_shrink(sz);
  }
};

};
