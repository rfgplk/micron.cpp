//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// huge standard allocator, cannot be mempooled, quad growth policy
// NOTE: make absolutely sure that a) the kernel has support for huge pages b) the executing user has all the appropriate
// huge pages permissions c) huge pages have been enabled otherwise this will error out with ENOMEM set
// /proc/sys/vm/nr_hugepages && nr_hugepages_policy on a lot of distros this is disabled by default
template <is_policy P = huge_allocation_policy> class allocator_huge : private map_allocator<byte, large_page_size>
{     // uses mmap_allocator as baseline allocator

public:
  // anything else untouched
  allocator_huge() {}     // init blank without args
  allocator_huge(const allocator_huge &o) {};
  allocator_huge(allocator_huge &&o) {}
  allocator_huge &
  operator=(const allocator_huge &o)
  {
    return *this;
  }
  allocator_huge &
  operator=(allocator_huge &&o)
  {
    return *this;
  }
  ~allocator_huge() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return alloc_auto_large_sz;
  }
  chunk<byte>
  create(size_t n)
  {
    n = to_page<large_page_size>(n);
    return chunk<byte>{ allocate(n), n };     // create the block, the handler is responsible for calling destroy
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    n = to_page<large_page_size>(n);
    n = n / old < P::on_grow ? P::on_grow * old : (n / old) * old;
    chunk<byte> mem = { allocate(n), n };
    micron::memcpy(mem.ptr, ptr, old);
    destroy({ ptr, old });
    return mem;
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  byte *share(void) = delete;
  int16_t
  get_grow() const
  {
    return P::on_grow;
  }
};

};
