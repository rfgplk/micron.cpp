//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../control.hpp"
#include "../../except.hpp"
#include "../../memory/addr.hpp"
#include "../../types.hpp"
#include "../linux/kmemory.hpp"
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
      if ( micron::munmap(micron::real_addr(__kernel_memory.ptr), __kernel_memory.len) == -1 ) {
        micron::abort();
      }
      // throw micron::except::memory_error("abcmalloc ~sheet(): failed to unmap memory");
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
  bool
  freeze(void)
  {
    if ( micron::mprotect(__kernel_memory.ptr, __kernel_memory.len, micron::PROT_READ) != 0 ) {
      return false;
    }
    return true;
  }
  bool
  freeze(int prot)
  {
    if ( micron::mprotect(__kernel_memory.ptr, __kernel_memory.len, prot) != 0 ) {
      return false;
    }
    return true;
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
  // request to allocate mem of sz, fail silently (return nullptr)
  micron::__chunk<byte>
  mark(size_t mem_sz)
  {
    if ( empty() )
      return { nullptr, 0 };
    micron::__chunk<byte> _p = __book.allocate(mem_sz);
    if ( _p.zero() or _p.invalid() )
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

    if ( _p.zero() or _p.invalid() )
      return { (micron::numeric_limits<byte *>::max() - 1), 0xFF };
    return _p;
  }

  // request to deallocate mem of sz
  bool
  try_unmark(micron::__chunk<byte> _p)
  {
    if ( empty() )
      micron::abort();
    if ( _p.zero() )
      micron::abort();
    auto r = __book.deallocate(_p);
    if ( r == __flag_out_of_space )
      return false;
    if ( r == __flag_invalid or r == __flag_failure )
      return false;
    return true;
  }
  bool
  try_tombstone(micron::__chunk<byte> _p)
  {
    if ( empty() )
      micron::abort();
    if ( _p.zero() )
      micron::abort();
    auto r = __book.tombstone(_p);
    if ( r == __flag_out_of_space )
      return false;
    if ( r == __flag_invalid or r == __flag_failure )
      return false;
    return true;
  }
  bool
  try_unmark_no_size(byte *_p)
  {
    if ( empty() )
      micron::abort();
    if ( _p == nullptr )
      micron::abort();
    __book.deallocate(_p);
    return true;
  }
  // deprecated technically
  bool
  try_tombstone_no_size(byte *_p)
  {
    if ( empty() )
      micron::abort();
    if ( _p == nullptr )
      micron::abort();
    __book.tombstone(_p);
    return true;
  }
  size_t
  available() const
  {
    if ( empty() )
      return 0;
    return __book.available();
  }
  size_t
  used() const
  {
    return __book.used();
  }
  size_t
  allocated() const
  {
    return __kernel_memory.len;
  }
  byte *
  addr() const
  {
    return __kernel_memory.ptr;
  }
  byte *
  addr_end() const
  {
    return __kernel_memory.ptr + __kernel_memory.len;
  }
  bool
  is_at(byte *_addr) const
  {
    if ( _addr >= addr() and _addr < addr_end() )
      return true;
    return false;
  }

  void
  reset(void)
  {
    __impl_release();
  }
};

template <u64 Sz>
sheet<Sz>
make_sheet(size_t req_size)
{
  return sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(req_size));
}

};
