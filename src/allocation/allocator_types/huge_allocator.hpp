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
template <is_policy P = huge_allocation_policy> class allocator_huge : private abc_allocator<byte>
{

public:
  ~allocator_huge() = default;
  allocator_huge() = default;
  allocator_huge(const allocator_huge &o) = default;
  allocator_huge(allocator_huge &&o) = default;
  allocator_huge &operator=(const allocator_huge &o) = default;
  allocator_huge &operator=(allocator_huge &&o) = default;

  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return alloc_auto_sz;
  }
  static chunk<byte>
  create(size_t n)
  {
    n = to_page(n);
    return allocate(n);     // create the block, the handler is responsible for calling destroy
  }
  static chunk<byte>
  grow(chunk<byte> memory, size_t n)
  {
    // NOTE: in case we somehow provide zerod out mem
    if ( memory.len == 0 and memory.ptr == nullptr ) {
      n = to_page(n);
      n *= P::on_grow;
      chunk<byte> mem = allocate(n);
      return mem;
    }
    n = to_page(n);
    n = n / memory.len < P::on_grow ? P::on_grow * memory.len : (n / memory.len) * memory.len;
    chunk<byte> mem = allocate(n);
    micron::memcpy(mem.ptr, memory.ptr, memory.len);
    destroy(memory);
    return mem;
  }
  static void
  destroy(const chunk<byte> mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  byte *share(void) = delete;
  static i16
  get_grow()
  {
    return P::on_grow;
  }
};

};
