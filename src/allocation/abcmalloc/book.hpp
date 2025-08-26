//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"
#include "../linux/allocate_map.hpp"
#include "config.hpp"
#include "free_list.hpp"

namespace abc
{

// calling it a sheet to avoid conf. with system pages
template <u64 Sz, u64 Pg = __default_page_mul> class sheet
{
  constexpr static const u64 __class = Sz;        // what class of data to hold
  constexpr static const u64 page_count = Pg;     // how many default OS pages per sheet
  using stack_free_list = __free_list<micron::__chunk<byte>, (__system_pagesize * page_count), __class>;
  //using stack_free_list = __free_list<micron::__chunk<byte>, (4096), 16>;
  void *__kernel_memory;
  stack_free_list __book;
  size_t sz = page_count * __system_pagesize;
  // when a new malloc happens insert it into the book

  inline __attribute__((always_inline)) void
  __impl_reserve()
  {
    if ( __kernel_memory == nullptr ) {
      __kernel_memory = micron::map_normal(nullptr, sz);
    }
  }

  inline __attribute__((always_inline)) void
  __impl_release(void)
  {
    if ( __kernel_memory ) {
      if ( micron::munmap(__kernel_memory, sz) == -1 )
        throw micron::except::memory_error("abcmalloc ~sheet(): failed to unmap memory");
      __kernel_memory = nullptr;
    }
  }

public:
  ~sheet() { __impl_release(); };
  sheet(void) : __kernel_memory(nullptr), __book(), sz(page_count * __system_pagesize) { __impl_reserve(); }
  sheet(void *__s, size_t _sz) : __kernel_memory(__s), __book(), sz(_sz) {}
  sheet(const sheet &) = delete;
  sheet(sheet &&o) : __kernel_memory(o.__kernel_memory), __book(micron::move(o.__book)), sz(o.sz)
  {
    o.__kernel_memory = nullptr;
    o.sz = 0;
  }
  sheet &operator=(const sheet &) = delete;
  sheet &
  operator=(sheet &&o)
  {
    __kernel_memory = o.__kernel_memory;
    sz = o.sz;
    o.__kernel_memory = nullptr;
    o.sz = 0;
    __book = micron::move(o.__book);
    return *this;
  }
  bool
  empty(void) const noexcept
  {
    return __kernel_memory == nullptr;
  }

  // request to allocate mem of sz
  micron::__chunk<byte>
  try_mark(size_t mem_sz)
  {
    if ( empty() )
      return { nullptr, 0 };
    micron::__chunk<byte> _p = __book.allocate(mem_sz);
    if ( _p.zero() )
      return { nullptr, 0 };
    return _p;
  }

  // request to deallocate mem of sz
  bool
  try_unmark(micron::__chunk<byte> _p)
  {
    if ( empty() )
      return false;
    if ( _p.zero() )
      return false;
    __book.deallocate(_p);
    return true;
  }
};

};
