//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// serial standard allocator, cannot be mempooled, default doubling policy
template <is_policy P = serial_allocation_policy> class allocator_serial : private abc_allocator<byte>
{

public:
  ~allocator_serial() = default;
  allocator_serial() = default;
  allocator_serial(const allocator_serial &o) = default;
  allocator_serial(allocator_serial &&o) = default;
  allocator_serial &
  operator=(const allocator_serial &o) = default;
  allocator_serial &
  operator=(allocator_serial &&o) = default;



  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return alloc_auto_sz;
  }
  chunk<byte>
  create(size_t n)
  {
    n = to_page(n);
    return chunk<byte>{ allocate(n), n };     // create the block, the handler is responsible for calling destroy
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    n = to_page(n);
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
