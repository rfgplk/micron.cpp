//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// total memory allocator, will allocate all system available memory reduced by offset
template <is_policy P = huge_allocation_policy> class allocator_total : private map_allocator<byte, page_size>
{     // uses mmap_allocator as baseline allocator
  size_t ram_size;

public:
  // anything else untouched
  allocator_total()
  {
    resources sz;
    ram_size = static_cast<size_t>(sz.free_memory * 0.90);     // leave some
  }     // init blank without args
  allocator_total(const allocator_total &o) {};
  allocator_total(allocator_total &&o) {}
  allocator_total &
  operator=(const allocator_total &o)
  {
    return *this;
  }
  allocator_total &
  operator=(allocator_total &&o)
  {
    return *this;
  }
  ~allocator_total() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  inline __attribute__((always_inline)) size_t
  auto_size()
  {
    return ram_size;
  }
  chunk<byte>
  create(size_t n)
  {
    if ( n < ram_size )
      n = ram_size;
    n = to_page<page_size>(n);
    return chunk<byte>{ allocate(n), n };     // create the block, the handler is responsible for calling destroy
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    throw except::memory_error("The total memory allocator cannot grow");
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  byte *share(void) = delete;
  i16
  get_grow() const
  {
    return P::on_grow;
  }
};

};
