//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// serial standard allocator, cannot be mempooled, default *tripling* policy
template <is_policy P = serial_allocation_policy> class allocator_serial : private abc_allocator<byte>
{

public:
  ~allocator_serial() = default;
  allocator_serial() = default;
  allocator_serial(const allocator_serial &o) = default;
  allocator_serial(allocator_serial &&o) = default;
  allocator_serial &operator=(const allocator_serial &o) = default;
  allocator_serial &operator=(allocator_serial &&o) = default;

  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return P::granularity;
  }

  static chunk<byte>
  create(size_t n)
  {
    n = to_granularity<P::granularity>(n);
    return allocate(n);     // create the block, the handler is responsible for calling destroy
  }

  static chunk<byte>
  grow(chunk<byte> memory, size_t n)
  {
    // NOTE: in case we somehow provide zerod out mem
    if ( memory.len == 0 and memory.ptr == nullptr ) {
      n = to_page(n);
      n = static_cast<size_t>(static_cast<f32>(n) * static_cast<f32>(P::on_grow));

      chunk<byte> mem = allocate(n);
      return mem;
    }
    n = to_granularity<P::granularity>(n);
    f32 n_f = static_cast<f32>(n);
    f32 len_f = static_cast<f32>(memory.len);
    f32 result_f = (n_f / len_f < P::on_grow) ? P::on_grow * len_f : (n_f / len_f) * len_f;
    n = static_cast<size_t>(result_f);
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
