//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
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
