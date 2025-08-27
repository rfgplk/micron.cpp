//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../control.hpp"
#include "../../except.hpp"
#include "../../types.hpp"
#include "../linux/allocate_map.hpp"
#include "config.hpp"
#include "free_list.hpp"
#include "hooks.hpp"

namespace abc
{

// calling it a sheet to avoid conf. with system pages
template <u64 Sz> class sheet
{
  constexpr static const u64 __size_class = Sz;     // what class of data to hold
  using stack_page_list = __buddy_list<micron::__chunk<byte>, __size_class, 64>;
  micron::__chunk<byte> __kernel_memory;
  stack_page_list __book;
  // when a new malloc happens insert it into the book

  inline __attribute__((always_inline)) void
  __impl_release(void)
  {
    if ( !__kernel_memory.zero() ) {
      if ( micron::munmap(__kernel_memory.ptr, __kernel_memory.len) == -1 )
        throw micron::except::memory_error("abcmalloc ~sheet(): failed to unmap memory");
      __kernel_memory.ptr = nullptr;
      __kernel_memory.len = 0;
    }
  }

public:
  ~sheet() { __impl_release(); };
  sheet(void) = delete;
  sheet(const micron::__chunk<byte> &mem) : __kernel_memory(mem), __book(mem) {}
  sheet(const sheet &) = delete;
  sheet(sheet &&o) : __kernel_memory(micron::move(o.__kernel_memory)), __book(micron::move(o.__book)) {}
  sheet &operator=(const sheet &) = delete;
  sheet &
  operator=(sheet &&o)
  {
    __kernel_memory = micron::move(o.__kernel_memory);
    __book = micron::move(o.__book);
    return *this;
  }
  void
  release(void)
  {
    __impl_release();
  }
  bool
  empty(void) const noexcept
  {
    return __kernel_memory.zero();
  }
  void
  resize(micron::__chunk<byte> &&n_mem)
  {
  }
  // request to allocate mem of sz, fail silently (return nullptr)
  micron::__chunk<byte>
  mark(size_t mem_sz)
  {
    if ( empty() )
      return { nullptr, 0 };
    micron::__chunk<byte> _p = __book.allocate(mem_sz);
    if ( _p.zero() )
      return { nullptr, 0 };
    return _p;
  }

  // request to allocate mem of sz, fail loudly, force quote
  micron::__chunk<byte>
  try_mark(size_t mem_sz)
  {
    if ( empty() )
      micron::abort();
    micron::__chunk<byte> _p = __book.allocate(mem_sz);
    // max order
    if ( _p.ptr == (byte *)0xFE and _p.len == 0xFE ) {
      micron::abort();
    }
    if ( _p.zero() )
      micron::abort();
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
  size_t
  available() const
  {
    if ( empty() )
      return 0;
    return __book.available();
  }
};

template <u64 Sz>
sheet<Sz>
make_sheet(size_t req_size)
{
  return sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(req_size));
}

};
